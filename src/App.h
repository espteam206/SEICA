#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

class App {
public:
    static App* Init();

    void Update(float dt);
    void Shutdown();
private:
    App();
    ~App();

    static App* s_Instance;
private:
    void WindowDockSpace();
    void WindowInput();
    void WindowGraph();
    void WindowCalcs();
    void PopupMixtures();
    void PopupCalcs();

    void WindowDemoGraph();

    void HelpWidget(const char*);
    
    void LoadData(const std::filesystem::path& file);

    void Calculate();
private:
    enum ContributorType {
        // Materials
        Cement,
        SCM,
        Admixture,
        Aggregate,

        Transport,

        Water,

        CONTRIBUTOR_TYPE_CNT
    };

    // Window titles for help popups of respective contributor types
    static constexpr const char* s_ContribPopupTitles[CONTRIBUTOR_TYPE_CNT] = {
        "Cements",
        "SCMs",
        "Admixtures",
        "Aggregates",
        "Transport Methods",
        "Water",
    };

    struct MixtureVal {
        float Value;
        float Accuracy;
        std::string Name;
        std::string Location;
        std::string Source;
    };

    std::array<std::vector<MixtureVal>, CONTRIBUTOR_TYPE_CNT> m_MixVals;

    // TODO: rename 'Name' to 'ID'
    struct InputVal {
        int32_t Name; // Index into m_MixVals[*type*]
        float Value;
        int32_t Trans; // Index into m_MixVals[ContributorType::Transport]
        float Dist;

        InputVal() : Name(-1), Value(0.0f), Trans(-1), Dist(0.0f) {}
    };

    float m_TotalVolume = 1.0f; // m^3 of cement
    float m_CementitiousMass;   // mass of all cementitious materials

    // Exclude the Transport Method
    std::array<std::vector<InputVal>, CONTRIBUTOR_TYPE_CNT> m_InputVals;

    std::vector<const char*> m_GraphLabels;
    std::vector<float> m_GraphValues;
    std::vector<float> m_MassValues;
    float m_TotalCO2 = 0.0f;

    // Window toggles
    bool m_ShowInput = true;
    bool m_ShowGraph = true;
    bool m_ShowCalcs = true;
    bool m_ShowDemoGraph = false;
    bool m_ShowImGuiDemo = false;
    // bool m_ShowMixturesVals   = false;
};
