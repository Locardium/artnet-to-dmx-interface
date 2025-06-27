#include "application.h"

#include "imgui.h"

#include "artnet.h"
#include "interface.h"

#include "opensans-regular.h"
#include "opensans-bold.h"

#include <thread>
#include <chrono>

static bool receiving = false;
static vector<uint8_t> dmx;
static int universe = 0;

static vector<string> networkInterfaces = ArtNet::listInterfaces();
static int currentIndex = 0;

static int refreshRate = 25;

static ImFont* Default;
static ImFont* SubPage;

namespace Application {
    void renderDMXGrid(const std::vector<uint8_t>& dmxValues) {
        const int numCols = 20;
        const int numRows = 26;
        const int startAddr = 1;

        if (ImGui::BeginTable("dmx-table", numCols + 1, ImGuiTableFlags_Borders)) {
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            ImGui::TableNextColumn();
            for (int c = 1; c <= numCols; c++) {
                ImGui::TableNextColumn();
                ImGui::Text("%d", c);
            }

            for (int r = 0; r < numRows; r++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                int baseAddr = startAddr + r * numCols;
                ImGui::Text("%d", baseAddr);

                for (int c = 0; c < numCols; c++) {
                    ImGui::TableNextColumn();
                    int channelIndex = baseAddr - 1 + c;
                    if (channelIndex < (int)dmxValues.size()) {
                        int val = dmxValues[channelIndex];
                        if (val > 0) {
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY());
                            ImGui::Text("%d", val);

                            // BG
                            ImVec2 cellMin = ImGui::GetItemRectMin();
                            ImVec2 cellMax = ImGui::GetItemRectMax();
                            ImGui::GetWindowDrawList()->AddRectFilled(
                                ImVec2(cellMin.x, cellMin.y),
                                ImVec2(cellMax.x, cellMax.y),
                                IM_COL32(0, 200, 0, 100)
                            );

                        }
                        else {
                            ImGui::TextUnformatted("0");
                        }
                    }
                    else {
                        ImGui::TextUnformatted("-");
                    }
                }
            }

            ImGui::EndTable();
        }
    }
    int messageBox(const char* id, const char* title, int type, const char* button1 = "Yes", const char* button2 = "No") {
        ImGui::OpenPopup(id);
        ImGui::BeginPopupModal(id, NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetWindowSize(ImVec2(300, 150));

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        ImGui::SetCursorPosX((300 - ImGui::CalcTextSize(title).x) * 0.5f);
        ImGui::Text(title);

        ImGui::SetCursorPosY(100);
        ImGui::Separator();

        if (type == 2)
        {
            ImGui::SetCursorPosX((300 - 120) * 0.1f);
            if (ImGui::Button(button1, ImVec2(120, 0)))
            {
                ImGui::EndPopup();
                return 1;
            }

            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();

            ImGui::SetCursorPosX((300 - 120) * 0.9f);
            if (ImGui::Button(button2, ImVec2(120, 0)))
            {
                ImGui::EndPopup();
                return 2;
            }
        }
        else if (type == 1)
        {
            ImGui::SetItemDefaultFocus();
            ImGui::SetCursorPosX((300 - 120) / 2);
            if (ImGui::Button(button1, ImVec2(120, 0)))
            {
                ImGui::EndPopup();
                return 1;
            }
        }


        ImGui::EndPopup();

        return 0;
    }
    ImVec4 RGBAToVec4(float R, float G, float B, float A) {
        return ImVec4(R / 255, G / 255, B / 255, A / 255);
    }

    void onLoad() {
        // Load ip
        string loadedIp = Config::read("ip");
        if (loadedIp != "")
        {
            int found = 0;
            for (size_t i = 0; i < networkInterfaces.size(); i++)
            {
                const string& ip = networkInterfaces[i];
                if (ip == loadedIp)
                {
                    ArtNet::selectInterface(ip);
                    found = i;
                    break;
                }
            }

            currentIndex = found;
        }

        // Load universe
        string loadedUniverse = Config::read("universe");
        if (loadedUniverse != "")
        {
            try {
                universe = std::stoi(loadedUniverse);
            }
            catch (const std::invalid_argument&) {
                std::cerr << "[APP-Config] Error 'universe' is not a valid number.\n";
            }
            catch (const std::out_of_range&) {
                std::cerr << "[APP-Config] Error 'universe' is out of range of int.\n";
            }
        }

        // Load refresh rate
        string loadedRefreshRate = Config::read("refreshRate");
        if (loadedRefreshRate != "")
        {
            try {
                refreshRate = std::stoi(loadedRefreshRate);
                Interface::setRefreshRate(refreshRate);
            }
            catch (const std::invalid_argument&) {
                std::cerr << "[APP-Config] Error 'refresh rate' is not a valid number.\n";
            }
            catch (const std::out_of_range&) {
                std::cerr << "[APP-Config] Error 'refresh rate' is out of range of int.\n";
            }
        }
    }

    void style() {
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowPadding = ImVec2(15, 15);
        style.WindowRounding = 5.0f;
        style.WindowBorderSize = 0.0f;
        style.WindowTitleAlign = ImVec2(0.02f, 0.5f);
        style.FramePadding = ImVec2(5, 5);
        style.FrameRounding = 4.0f;
        style.FrameBorderSize = 1;
        style.ItemSpacing = ImVec2(12, 8);
        style.ItemInnerSpacing = ImVec2(8, 6);
        style.IndentSpacing = 25.0f;
        style.ScrollbarSize = 15.0f;
        style.ScrollbarRounding = 9.0f;
        style.GrabMinSize = 5.0f;
        style.GrabRounding = 3.0f;

        style.Colors[ImGuiCol_WindowBg] = ImColor(9, 9, 9, 255);
        style.Colors[ImGuiCol_ChildBg] = ImColor(11, 11, 11, 255);
        style.Colors[ImGuiCol_PopupBg] = ImColor(9, 9, 9, 255);
        style.Colors[ImGuiCol_FrameBg] = ImColor(11, 11, 11, 255);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

        style.Colors[ImGuiCol_TitleBgActive] = ImColor(11, 11, 11, 255);

        style.Colors[ImGuiCol_Separator] = ImColor(110, 110, 128, 30);
        style.Colors[ImGuiCol_SeparatorHovered] = ImColor(110, 110, 128, 30);
        style.Colors[ImGuiCol_SeparatorActive] = ImColor(110, 110, 128, 30);

        style.Colors[ImGuiCol_Border] = ImColor(110, 110, 128, 30);

        style.Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.12f, 0.12f, 0.5f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);

        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.12f, 0.12f, 0.12f, 0.5f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.IniFilename = nullptr;

        ImFontConfig font_config;
        static const ImWchar ranges[] =
        {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
                0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
                0x2DE0, 0x2DFF, // Cyrillic Extended-A
                0xA640, 0xA69F, // Cyrillic Extended-B
                0xE000, 0xE226, // icons
                0,
        };
        font_config.FontDataOwnedByAtlas = false;

        Default = io.Fonts->AddFontFromMemoryTTF(OpenSans_Bold, sizeof(OpenSans_Bold), 17.0f, &font_config, ranges);
        SubPage = io.Fonts->AddFontFromMemoryTTF(OpenSans_Regular, sizeof(OpenSans_Regular), 17.0f, &font_config, ranges);
    }

    void renderUI()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        {
            ImVec2 tabSize = ImVec2(viewport->Size.x - 20, 220);
            //ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
            ImGui::SetCursorPosX(10);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
            ImGui::BeginChild("Tab1Child", tabSize);
            {
                // Status
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
                ImGui::Text("Status");
                ImGui::SameLine();

                ImVec4 ColorChildBgStatus;
                ImVec4 ColorTextStatus;
                string TextStatus;

                int connectStatus = Interface::connect(0);

                if (connectStatus == 1)
                {
                    TextStatus = "Connected  ";
                    ColorChildBgStatus = RGBAToVec4(32, 100, 64, 255);
                    ColorTextStatus = RGBAToVec4(9, 230, 140, 255);
                }
                else if (connectStatus == 2) 
                {
                    TextStatus = "Error   ";
                    ColorChildBgStatus = RGBAToVec4(100, 24, 24, 255);
                    ColorTextStatus = RGBAToVec4(230, 49, 49, 255);
                }
                else
                {
                    TextStatus = "Waiting for connection";
                    ColorChildBgStatus = RGBAToVec4(120, 100, 24, 255);
                    ColorTextStatus = RGBAToVec4(255, 230, 0, 255);
                }

                ImVec2 childSize = ImVec2(ImGui::CalcTextSize(TextStatus.c_str()).x + 20, 25);

                ImGui::SetCursorPosX(tabSize.x - childSize.x - 10);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.5);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ColorChildBgStatus);
                ImGui::PushStyleColor(ImGuiCol_Text, ColorTextStatus);
                ImGui::BeginChild("##StatusText", childSize, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 17);
                    ImGui::SetCursorPosY(2.6);
                    ImGui::PushFont(SubPage);
                    ImGui::Text(TextStatus.c_str());
                    ImGui::PopFont();
                    ImGui::PopStyleColor(2);
                    ImGui::EndChild();
                }

                // Interface selector
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
                ImGui::Text("Network interface");
                ImGui::SameLine();
                ImGui::SetCursorPosX(tabSize.x - 120 - 10);
                if (!networkInterfaces.empty())
                {
                    static std::vector<const char*> labels;
                    if (labels.empty()) for (auto& s : networkInterfaces) labels.push_back(s.c_str());

                    ImGui::PushItemWidth(120);
                    if (ImGui::Combo("##network-interface-combo", &currentIndex, labels.data(), (int)labels.size()))
                    {
                        string ip = networkInterfaces[currentIndex];

                        ArtNet::selectInterface(ip);
                        Config::write("ip", ip);

                        if (receiving)
                        {
                            ArtNet::stopReceiving();
                            ArtNet::startReceiving();
                        }
                    }
                    ImGui::PopItemWidth();
                }
                else
                {
                    if (messageBox("##artnet-error", "No IPv4 interfaces found", 1, "Close")) PostQuitMessage(0);
                }
                
                // Universe selector
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
                ImGui::Text("Universe");
                ImGui::SameLine();
                ImGui::SetCursorPosX(tabSize.x - 100 - 10);
                ImGui::PushItemWidth(100);
                if (universe < 10)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(11, 5));
                }
                else if (universe >= 10 && universe < 100)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 5));
                }
                else if (universe >= 100)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 5));
                }
                if (ImGui::InputInt("##universe-input", &universe))
                {
                    Config::write("universe", universe);
                }
                ImGui::PopStyleVar();
                ImGui::PopItemWidth();
                if (universe < 1) universe = 1;
                if (universe > 100) universe = 100;

                // Refresh rate selector
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
                ImGui::Text("Refresh Rate (ms)");
                ImGui::SameLine();
                ImGui::SetCursorPosX(tabSize.x - 100 - 10);
                ImGui::PushItemWidth(100);
                if (refreshRate < 10)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(11, 5));
                }
                else if (refreshRate >= 10 && refreshRate < 100)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 5));
                }
                else if (refreshRate >= 100)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 5));
                }
                if (ImGui::InputInt("##refresh-rate-input", &refreshRate))
                {
                    Interface::setRefreshRate(refreshRate);
                    Config::write("refreshRate", refreshRate);
                }
                ImGui::PopStyleVar();
                ImGui::PopItemWidth();
                if (refreshRate < 1) refreshRate = 1;

                // Debug mode
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
                ImGui::Text("Show debug console");
                ImGui::SameLine();
                ImGui::SetCursorPosX(tabSize.x - 27 - 10);
                if (ImGui::Checkbox("##show-debug-console", &showConsole))
                {
                    if (showConsole)
                    {
                        AllocConsole();
                        freopen("CONOUT$", "w", stdout);
                        freopen("CONOUT$", "w", stderr);
                        freopen("CONIN$", "r", stdin);
                    }
                    else
                    {
                        fclose(stdin);
                        fclose(stdout);
                        fclose(stderr);
                        FreeConsole();
                    }
                }

                ImGui::EndChild();
            }
            ImGui::PopStyleVar(1);
        }

        {
            ImVec2 tabSize = ImVec2(viewport->Size.x - 20, 200);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
            ImGui::SetCursorPosX(10);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
            ImGui::BeginChild("Tab2Child", tabSize);
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);

                auto dmx = ArtNet::getDMX(universe);
                renderDMXGrid(dmx);
                Interface::setDMX(dmx);

                ImGui::EndChild();
            }
            ImGui::PopStyleVar(1);
        }

        // Start/stop
        {
            ImVec2 tabSize = ImVec2(viewport->Size.x - 20, 45);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
            ImGui::SetCursorPosX(10);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
            ImGui::BeginChild("Tab3Child", tabSize);
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);

                // Start/stop
                ImGui::Text("Action");
                ImGui::SameLine();
                ImGui::SetCursorPosX(tabSize.x - 80 - 10);
                if (!receiving) {
                    if (ImGui::Button("Start", ImVec2(80, 25))) {
                        if (ArtNet::startReceiving()) {
                            receiving = true;
                            Interface::start();
                        }
                        else
                        {
                            if (messageBox("##artnet-error", "Error starting artnet", 1, "Close")) PostQuitMessage(0);
                        }
                    }
                }
                else {
                    if (ImGui::Button("Stop", ImVec2(80, 25))) {
                        ArtNet::stopReceiving();
                        Interface::stop();
                        receiving = false;
                        this_thread::sleep_for(chrono::seconds(1));
                    }
                }

                ImGui::EndChild();
            }
            ImGui::PopStyleVar(1);
        }
    }    
}
