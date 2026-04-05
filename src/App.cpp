#include "App.h"
#include "CSV.h"

#include <imgui.h>
#include <implot.h>

#include <cstdio>

App* App::s_Instance;

App* App::Init() {
    if (!s_Instance)
        return new App;
    else
        return nullptr;
}

App::App() {
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Load font
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    io.FontDefault = io.Fonts->AddFontFromFileTTF("res/fonts/Roboto/roboto_regular.ttf", 24.0f, &fontConfig);

    // Styling
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.GrabRounding = 4.0f;
    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.WindowMinSize = { 200.0f, 200.0f };

    LoadData("res/databases/Test.csv");

    // static const ImU32 MyColors[3] = {IM_COL32(255,0,0,255), IM_COL32(0,255,0,255), IM_COL32(0,0,255,255)};
    // ImPlotColormap myColormap = ImPlot::AddColormap("DFSLJKSFDLJKDSFLKJ", MyColors, 3);
}

App::~App() {
    // stuff
}

void App::WindowInput() {
    if (!m_ShowInput)
        return;

    static constexpr const char* s_InputValueUnits[CONTRIBUTOR_TYPE_CNT] = {
        "% mass",
        "% mass",
        "kg/m^3 cement",
        "kg/m^3 cement",
        "kg/(tonne*km)",
        "ratio",
    };

    ImGui::Begin("Mixture parameters", &m_ShowInput);

    ImGui::InputFloat("kg cementitious materials / m^3 mixture", &m_CementitiousMass, 0.0f, 0.0f, "%.2f");
    ImGui::InputFloat("Total mixture volume", &m_TotalVolume, 0.0f, 0.0f, "%.2f");
    ImGui::Separator();

    for (int32_t type = 0; type < CONTRIBUTOR_TYPE_CNT; ++type) {
        if (type == ContributorType::Transport)
            continue;

        ImGui::PushID(type);

        ImGui::Text("%s", s_ContribPopupTitles[type]);
        ImGui::SameLine();
        if (ImGui::Button(" ? "))
            ImGui::OpenPopup(s_ContribPopupTitles[type]);
        PopupMixtures();

        ImGui::Separator();

        std::vector<InputVal>& vals = m_InputVals[type];

        for (int i = 0; i < vals.size(); ++i) {
            ImGui::PushID(i);

            ImGui::BeginTable("inputs", 3, ImGuiTableFlags_SizingFixedFit);
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            // The delete button
            // TODO: Make it square
            ImGui::PushItemWidth(100);
            if (ImGui::Button("-")) {
                vals.erase(std::next(vals.begin(), i));
                ImGui::EndTable();
                ImGui::PopID();
                continue;
            }
            ImGui::PopItemWidth();

            ImGui::TableNextColumn();
            // ImGui::SetNextItemWidth(TEXT_BASE_WIDTH * 30);
            ImGui::SetNextItemWidth(150);
            std::vector<MixtureVal>& values = m_MixVals[type];
            const char* comboStr = vals[i].Name != -1 ? values[vals[i].Name].Name.c_str() : "";
            if (ImGui::BeginCombo("Type", comboStr)) {
                for (uint32_t j = 0; j < values.size(); ++j)
                    if (ImGui::Selectable(values[j].Name.c_str()))
                        vals[i].Name = j;

                ImGui::EndCombo();
            }

            ImGui::TableNextColumn();
            const char* unit = s_InputValueUnits[type];
            ImGui::InputFloat(unit, &vals[i].Value, 0.0f, 0.0f, "%.3f");

            ImGui::TableNextRow();
            ImGui::TableNextColumn(); // Skip the column with the "minus" button

            if (type == ContributorType::Water) {
                // Assume water is provided at the source
                ImGui::EndTable();
                ImGui::PopID();
                continue;
            }

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(150);
            std::vector<MixtureVal>& transVals = m_MixVals[ContributorType::Transport];
            comboStr = vals[i].Trans != -1 ? transVals[vals[i].Trans].Name.c_str() : "";
            if (ImGui::BeginCombo("Type##Transport", comboStr)) {
                for (uint32_t j = 0; j < transVals.size(); ++j)
                    if (ImGui::Selectable(transVals[j].Name.c_str()))
                        vals[i].Trans = j;

                ImGui::EndCombo();
            }

            ImGui::TableNextColumn();
            ImGui::InputFloat("km", &vals[i].Dist, 0.0f, 0.0f, "%.1f");

            ImGui::Dummy(ImVec2(0.0f, 20.0f));
            ImGui::EndTable();

            ImGui::PopID();
        }

        if (ImGui::Button("Add"))
            vals.emplace_back();

        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        ImGui::PopID();
    }

    ImGui::PushItemWidth(ImGui::GetWindowSize().x);
    if (ImGui::Button("Calculate!")) {
        Calculate();
    }

    ImGui::End();
}

void App::WindowGraph() {
    if (!m_ShowGraph)
        return;

    ImGui::Begin("Carbon Footprint", &m_ShowGraph);

    ImPlotPieChartFlags flags = 0;
    flags |= ImPlotPieChartFlags_Exploding;
    flags |= ImPlotPieChartFlags_Normalize;
    flags |= ImPlotPieChartFlags_IgnoreHidden;

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    float width = std::min(viewportSize.x, viewportSize.y);

    // Centre the graph
    float off = (viewportSize.x - width) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

    if (ImPlot::BeginPlot("##Footprint composition", { width, width })) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 1, 0, 1);
        ImPlot::PushStyleColor(ImPlotCol_Fill, (ImVec4){ 0.0f, 1.0f, 1.0f, 1.0f });
        ImPlot::PushStyleColor(ImPlotCol_LegendBg, (ImVec4){ 0.0f, 1.0f, 1.0f, 1.0f });
        // Use the custom colormap
        // ImPlot::PushColormap("DFSLJKSFDLJKDSFLKJ");
        ImPlot::PlotPieChart(m_GraphLabels.data(), m_GraphValues.data(), m_GraphValues.size(), 0.5, 0.5, 0.4, "%.2f", 90, flags);
        ImPlot::PopStyleColor(2);
        // ImPlot::PopColormap();

        ImPlot::EndPlot();
    }

    ImGui::End();
}

void App::WindowCalcs() {
    if (!m_ShowCalcs)
        return;

    ImGui::Begin("Carbon Footprint Details", &m_ShowCalcs);

    ImGui::Text("CO2 emitted per m^3 (kg): %.2f", m_TotalCO2);
    ImGui::Text("Total CO2 emitted (kg): %.2f", m_TotalVolume * m_TotalCO2);

    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders
        | ImGuiTableFlags_ScrollX
        | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##Carbon Footprint Details", 4, tableFlags)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Material kg per m^3");
        ImGui::TableSetupColumn("CO2 kg per m^3");
        ImGui::TableSetupColumn("Percentage Contribution");
        ImGui::TableHeadersRow();
        for (uint32_t i = 0; i < m_GraphLabels.size(); ++i) {
            ImGui::PushID(i);
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("%s", m_GraphLabels[i]);

            ImGui::TableNextColumn();
            if (m_MassValues[i] == 0.0f)
                ImGui::Text("N/A");
            else
                ImGui::Text("%.2f", m_MassValues[i]);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", m_GraphValues[i]);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f%%", m_GraphValues[i] * 100.0f / m_TotalCO2);

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void App::PopupMixtures() {
    static constexpr const char* s_ContribValueUnits[CONTRIBUTOR_TYPE_CNT] = {
        "kg/kg",
        "kg/kg",
        "kg/kg",
        "kg/kg",
        "kg/tonne*km",
        "kg/m^3",
    };

    for (int32_t type = 0; type < CONTRIBUTOR_TYPE_CNT; ++type) {
        static char valueHeader[10];
        sprintf(valueHeader, "Value (%s)", s_ContribValueUnits[type]);

        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(s_ContribPopupTitles[type], NULL, ImGuiWindowFlags_HorizontalScrollbar)) {
            ImGuiTableFlags tableFlags =
                ImGuiTableFlags_Borders
                | ImGuiTableFlags_ScrollX
                | ImGuiTableFlags_ScrollY;
            ImVec2 outerSize = ImVec2(ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 7 + 30));
            ImGui::BeginTable("Mixture info", 6, tableFlags, outerSize);
            ImGui::TableSetupColumn("#",         ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Name",      ImGuiTableColumnFlags_WidthFixed, 300.f);
            ImGui::TableSetupColumn(valueHeader, ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Accuracy",  ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Location",  ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Source",    ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            for (const MixtureVal& val : m_MixVals[type]) {
                ImGui::PushID(&val);
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("N");  // TODO: INSERT NUMBER HERE

                ImGui::TableNextColumn();
                ImGui::Text("%s", val.Name.c_str());
                // Since long names are cut off, provide
                // a tooltip with the full name if hovered over
                if (ImGui::BeginItemTooltip()) {
                    ImGui::Text("%s", val.Name.c_str());
                    ImGui::EndTooltip();
                }

                ImGui::TableNextColumn();
                if (val.Value < 0.1f)
                    ImGui::Text("%.2e", val.Value);
                else
                    ImGui::Text("%.3f", val.Value);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f%%", val.Accuracy * 100);
                ImGui::TableNextColumn();
                ImGui::Text("%s", val.Location.c_str());
                ImGui::TableNextColumn();
                if (val.Source.find("https://") == 0)
                    ImGui::TextLinkOpenURL(val.Source.c_str());
                else
                    ImGui::Text("%s", val.Source.c_str());

                ImGui::PopID();
            }

            ImGui::EndTable();

            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
}

void App::WindowDockSpace() {
    // Fullscreen stuff
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar();

    ImGui::PopStyleVar(2);

    // Submit the DockSpace
    static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiID dockspaceID = ImGui::GetID("DockSpace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New");

            ImGui::Separator();

            if (ImGui::MenuItem("Quit")) {}

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Graph"))               { m_ShowGraph = true; }
            if (ImGui::MenuItem("Mixture Input"))       { m_ShowInput = true; }
            if (ImGui::MenuItem("Caclulation details")) { m_ShowCalcs = true; }

            ImGui::Separator();

            if (ImGui::MenuItem("Demo Graph"))      { m_ShowDemoGraph = true; }
            if (ImGui::MenuItem("Show ImGui Demo")) { m_ShowImGuiDemo = true; }

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("About"))
            ImGui::OpenPopup("About##Popup");

        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("About##Popup")) {
            ImGui::Text("Concrete carbon calculator\n");
            ImGui::Text(R"(Authors:
Jeffrey Chen
Ruth Cheng
Zoey Fong
Raphael Larroquette
Justin Ma
Julian Poon)");
            ImGui::TextLinkOpenURL("Learn more", "https://github.com/espteam206/desktop");
            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void App::HelpWidget(const char* text) {
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(text);
        ImGui::TextLinkOpenURL("Learn more", "https://github.com/espteam206/desktop");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void App::Update(float dt) {
    if (m_ShowImGuiDemo)
        ImGui::ShowDemoWindow(&m_ShowImGuiDemo);

    WindowDockSpace();
    WindowInput();
    WindowGraph();
    WindowCalcs();
    WindowDemoGraph();
}

void App::Shutdown() {
    delete s_Instance;
    s_Instance = nullptr;
}

// Since this is just for development, leave it last
void App::WindowDemoGraph() {
    if (!m_ShowDemoGraph)
        return;

    ImGui::Begin("Demo graph", &m_ShowDemoGraph);

    static const char* labels1[]    = {"Frogs","Hogs","Dogs","Logs"};
    static float data1[]            = {0.15f,  0.30f,  0.2f, 0.05f};
    static ImPlotPieChartFlags flags = 0;
    ImGui::SetNextItemWidth(250);
    ImGui::DragFloat4("Values", data1, 0.01f, 0, 1);

    if (ImPlot::BeginPlot("##Pie1", ImVec2(ImGui::GetTextLineHeight()*16,ImGui::GetTextLineHeight()*16), ImPlotFlags_Equal | ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 1, 0, 1);
        ImPlot::PlotPieChart(labels1, data1, 4, 0.5, 0.5, 0.4, "%.2f", 90, flags);
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    static const char* labels2[]   = {"A","B","C","D","E"};
    static int data2[]             = {1,1,2,3,5};

    ImPlot::PushColormap(ImPlotColormap_Pastel);
    if (ImPlot::BeginPlot("##Pie2", ImVec2(ImGui::GetTextLineHeight()*16,ImGui::GetTextLineHeight()*16), ImPlotFlags_Equal | ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 1, 0, 1);
        ImPlot::PlotPieChart(labels2, data2, 5, 0.5, 0.5, 0.4, "%.0f", 180, flags);
        ImPlot::EndPlot();
    }
    ImPlot::PopColormap();

    ImGui::End();
}

void App::LoadData(const std::filesystem::path& path) {
    CSV csv(path);

    for (std::vector<MixtureVal>& m : m_MixVals)
        m.reserve(csv.GetLineCount() / CONTRIBUTOR_TYPE_CNT);

    while (csv.NextLine()) {
        try {
            std::string contributor = csv.Next<std::string>().value();
            std::string category    = csv.Next<std::string>().value();
            std::string name        = csv.Next<std::string>().value();
            std::string location    = csv.Next<std::string>().value();
            float value             = csv.Next<float>().value();
            float accuracy          = csv.Next<float>().value();
            std::string source      = csv.Next<std::string>().value();

            ContributorType type;
            if (contributor == "Admixtures")
                type = ContributorType::Admixture;
            else if (contributor == "Aggregates")
                type = ContributorType::Aggregate;
            else if (contributor == "Cement")
                type = ContributorType::Cement;
            else if (contributor == "Transport")
                type = ContributorType::Transport;
            else if (contributor == "SCM")
                type = ContributorType::SCM;
            else if (contributor == "Water")
                type = ContributorType::Water;
            else
                throw std::invalid_argument("Invalid contributor type");

            m_MixVals[type].emplace_back(
                value,
                accuracy,
                std::move(name),
                std::move(location),
                std::move(source)
            );
        } catch (std::bad_optional_access& e) {
            printf("Error reading csv file!\n");
            printf("Row: %d column: %d\n", csv.GetRow(), csv.GetColumn());
            continue;
        } catch (std::invalid_argument& e) {
            printf("Error reading csv file! %s\n", e.what());
            printf("Row: %d column: %d\n", csv.GetRow(), csv.GetColumn());
            continue;
        }
    }
}
