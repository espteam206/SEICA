#include "App.h"

#include <iostream>

void App::Calculate() {
    // Calculates kg of CO2 emitted for every m^3 of mixture

    std::cout << "Beginning calculation..\n";

    if (m_CementitiousMass <= 0.0f) {
        std::cout << "Mass of cementitious materials must be greater than 0.\n";
        return;
    }

    m_TotalCO2 = 0.0f;
    float mixtureKg = 0.0f;

    static constexpr std::array<ContributorType, 4> MATERIAL_TYPES = {
        ContributorType::Cement,
        ContributorType::SCM,
        ContributorType::Admixture,
        ContributorType::Aggregate,
    };

    // Multiply by two since each material also has a transport method
    m_GraphLabels.clear();
    m_GraphValues.clear();
    m_MassValues.clear();
    m_GraphLabels.reserve(m_InputVals.size() * 2);
    m_GraphValues.reserve(m_InputVals.size() * 2);

    // Calculate cementitious materials contribution
    for (ContributorType type : MATERIAL_TYPES) {
        for (const InputVal& material : m_InputVals[type]) {
            // TODO: Notify of errors
            if (material.Name == -1 || material.Trans == -1) {
                std::cout << "A value in ";
                std::cout << s_ContribPopupTitles[type];
                std::cout << " category was not set!\n";
                continue;
            }

            if (material.Value < 0.0f || material.Dist < 0.0f) {
                std::cout << "Material value cannot be negative!";
                continue;
            }

            // Find the mass of the material
            // Cementitious materials: % mass of cementitious materials
            // Admixtures + Aggregates: kg/m^3 cement
            float mass = material.Value;
            if (type == ContributorType::Cement || type == ContributorType::SCM)
                mass = material.Value * m_CementitiousMass * 0.01f;

            mixtureKg += mass;

            // mass  is kg of material
            // perKg in kg CO2 / kg material
            float CO2perKg = m_MixVals[type][material.Name].Value;
            float matCO2 = mass * CO2perKg;

            // TODO: append numbers to label
            m_GraphLabels.push_back(m_MixVals[type][material.Name].Name.c_str());
            m_GraphValues.push_back(matCO2);
            m_MassValues.push_back(mass);

            // material.Dist in CO2 per (km * 1000kg) of transportation
            float perKm = m_MixVals[ContributorType::Transport][material.Trans].Value * 0.001;
            float transCO2 = mass * material.Dist * perKm;

            m_GraphLabels.push_back(m_MixVals[ContributorType::Transport][material.Trans].Name.c_str());
            m_GraphValues.push_back(transCO2);
            m_MassValues.push_back(0.0f);

            std::cout << "Material name: " << m_MixVals[type][material.Name].Name << "\n";
            std::cout << "  Material carbon contribution: " << matCO2 << " kg of CO2\n";
            std::cout << " Transportation name: " << m_MixVals[ContributorType::Transport][material.Trans].Name << "\n";
            std::cout << "  Transportation carbon contribution: " << transCO2 << " kg of CO2\n";

            m_TotalCO2 += matCO2 + transCO2;
        }
    }

    // Calculate water
    if (!m_InputVals[ContributorType::Water].empty()) {
        // Realistically, there's only one water value
        // but just for safety...
        for (const InputVal& water : m_InputVals[ContributorType::Water]) {
            if (water.Name == -1) {
                std::cout << "A value in ";
                std::cout << s_ContribPopupTitles[ContributorType::Water];
                std::cout << " category was not set!\n";
                continue;
            }

            if (water.Value < 0.0f || water.Dist < 0.0f) {
                std::cout << "Material value cannot be negative!";
                continue;
            }

            float waterKg = water.Value * 0.01f * mixtureKg;
            float waterVol = waterKg * 0.001f;
            float CO2perKg = m_MixVals[ContributorType::Water][water.Name].Value * waterVol;
            float waterCO2 = waterKg * CO2perKg;
            m_GraphLabels.push_back(m_MixVals[ContributorType::Water][water.Name].Name.c_str());
            m_GraphValues.push_back(waterCO2);
            m_MassValues.push_back(waterKg);
            m_TotalCO2 += waterCO2;
        }
    }

    std::cout << "Calculated!\n";
}
