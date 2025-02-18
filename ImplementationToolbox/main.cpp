// Dear ImGui: standalone example application for DirectX 9

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

/*
*****Author: Alex Towne
*****Date: Began on 2/1/24
*****Last Updated: 1/30/2025
*****Purpose: This program is built in the ImGui framework to add UI wrapping to C++ code.
* The plan is to continually update with tools/utilities that assist the Professional services department with rapid completion of installs/tasks
* Will be expanded in the future to assist the tech team with migrations by integrating directly with file share systems and SQL servers via ODBC connections to automate file moving and SQL migration 

*/

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include "genericexport_window.h"
#include "oneButtonRefresh_window.h"
#include "sqlQueryBuilder.h"
#include "systemFunctions.h"
#include <filesystem>
#include <iostream>
#include <string>
#include "xmlParser.h"
#include "Sql.h"

// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static bool                     g_DeviceLost = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Main code
int main(int, char**)
{
    // Change both version nums at the same time, haven't found a way to convert from wchar_t to char* yet.
    const wchar_t* versionNum = L"Implementation Toolbox v0.5.0";
    const char* currVersion = "Implementation Toolbox v0.5.0";
    const char* lastUpdate = "2/10/25";

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Implementation Toolbox", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, versionNum, WS_OVERLAPPEDWINDOW, 100, 100, 1280, 900, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    // ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    // Darken the input text boxes
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.7f); // Darker gray for input box background
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // Slightly darker gray when hovered
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // Even darker gray when active

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    static float font_size = 14.0f;

    // Try to load an Arial font from windows directory
    if (io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", font_size));
    // If we can't find it use the default one
    else (io.Fonts->AddFontDefault());

   

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;

    // My boolean states
    bool show_generic_export_window = false;
    bool show_one_button_refresh_window = false;
    bool show_sql_query_builder_window = false;
    bool show_xml_parser_window = false;
    bool show_sql_conn_window = false;                  // Is SQL connection configuration window open or closed
    bool show_modules = true;
    
    // Window open state
    static bool open_getting_started = true;
    static bool open_recent_updates = true;
    static bool open_health_check = true;
    static bool open_bgcolor_picker = false;

    // Background colors
    static ImVec4 bg_color = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    static ImVec4 cst_orange = ImVec4(0.843f, 0.251f, 0.035f, 1.0f); // #d74009 hex color
    static ImVec4 cst_purple = ImVec4(0.161f, 0.020f, 0.282f, 1.0f); // #290548 hex color

    // Load image file for CST logo
    int my_image_width = 0;
    int my_image_height = 0;
    PDIRECT3DTEXTURE9 my_texture = NULL;
    bool ret = displayCentralSquareLogo(g_pd3dDevice, "Images/cstlogo.png", &my_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle lost D3D9 device
        if (g_DeviceLost)
        {
            HRESULT hr = g_pd3dDevice->TestCooperativeLevel();
            if (hr == D3DERR_DEVICELOST)
            {
                ::Sleep(10);
                continue;
            }
            if (hr == D3DERR_DEVICENOTRESET)
                ResetDevice();
            g_DeviceLost = false;
        }

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            ResetDevice();
        }

        

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Get main viewport
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_center = ImGui::GetMainViewport()->GetCenter();

        // Window sizes for each main screen window
        float
            // Getting started window size
            // Using this window as the main anchor to position all other windows from
            getting_started_x = 250,
            getting_started_y = 100,
            getting_started_posx = 0,
            getting_started_posy = 20,

            // Logo window size & pos
            cst_logo_x = 550,
            cst_logo_y = getting_started_y,
            cst_logo_posx = getting_started_x,
            cst_logo_posy = getting_started_posy,

            // Health Check size & pos
            health_check_x = viewport->Size.x - (cst_logo_x + getting_started_x),
            health_check_y = getting_started_y,
            health_check_posx = getting_started_x + cst_logo_x,
            health_check_posy = 20,

            // Module button window size & pos
            module_buttons_x = getting_started_x,
            module_buttons_y = 400,
            module_buttons_posx = getting_started_posx,
            module_buttons_posy = getting_started_y + getting_started_posy,

            // Recent update window size & pos
            recent_update_x = getting_started_x,
            recent_update_y = viewport->Size.y - (module_buttons_posy + module_buttons_y),
            recent_update_posx = 0,
            recent_update_posy = module_buttons_posy + module_buttons_y,

            // Module Window size & pos
            module_window_x = viewport->Size.x - getting_started_x,
            module_window_y = viewport->Size.y - getting_started_y - getting_started_posy,
            module_window_posx = getting_started_x,
            module_window_posy = getting_started_y + getting_started_posy,


            // Module Button size
            module_button_size_x = module_buttons_x,
            module_button_size_y = module_buttons_y / 3.3333;        // Divide by 3 for 3 buttons so they should take up the whole window for each button        

        // Containers for ImGui window sizes
        ImVec2
            gettingStartedSize = ImVec2(getting_started_x, getting_started_y),
            gettingStartedPos = ImVec2(getting_started_posx, getting_started_posy),
            healthCheckSize = ImVec2(health_check_x, health_check_y),
            healthCheckPos = ImVec2(health_check_posx, health_check_posy),
            cstLogoSize = ImVec2(cst_logo_x, cst_logo_y),
            cstLogoPos = ImVec2(cst_logo_posx, cst_logo_posy),
            recentUpdateSize = ImVec2(recent_update_x, recent_update_y),
            recentUpdatePos = ImVec2(recent_update_posx, recent_update_posy),
            moduleSize = ImVec2(module_window_x, module_window_y),
            modulePos = ImVec2(module_window_posx, module_window_posy),
            // Window Size and Position for module buttons
            moduleButtonSize = ImVec2(module_buttons_x, module_buttons_y),
            moduleButtonPos = ImVec2(module_buttons_posx, module_buttons_posy),
            // Size of module buttons to select from
            moduleSelectionSize = ImVec2(module_button_size_x, module_button_size_y);


        static std::string directory_path = "C:\\ImplementationToolbox\\";

        // Display the CST logo at the top of the screen
        ImGui::SetNextWindowPos(cstLogoPos);
        ImGui::SetNextWindowSize(cstLogoSize);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, cst_purple);
        ImGui::Begin("##cstlogo", NULL, ImGuiWindowFlags_NoDecoration);
        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Image((ImTextureID)(intptr_t)my_texture, ImVec2(my_image_width, my_image_height));
        // End logo image window
        ImGui::End();

        // End orange color
        ImGui::PopStyleColor();

        // SQL Connection string variables for connection test
        static char serverNameBuffer[256] = { }, databaseNameBuffer[256] = { }, usernameBuffer[256] = { }, passwordBuffer[256] = { };
        static std::string sqlServerName, databaseName, sqlUsername, sqlPassword;
        static bool integratedSecurity = false;
        static bool connection_attempted = false;   // Have we tried to connect yet? - Use this to determine what to display in the health check window
        static bool connection_success = false;     // Did we succeed in our attempt to connect to SQL?

        // Open the SQL configuration window if the menu item is selected
        if (show_sql_conn_window)
            ImGui::OpenPopup("SQL Connection Settings");

        // Create a static sql class to store connection info in so we can pass to different functions that may need the SQL connectivity
        static Sql sql;

        // Create window to configure connection string with
        static float fieldLen = 101;
        //ImGui::SetNextWindowSize(ImVec2(260, 210));
        ImGui::SetNextWindowPos(ImVec2(window_center.x - (250 / 2), window_center.y - (200 / 2)));
        if (ImGui::BeginPopupModal("SQL Connection Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::BeginTable("SQL Connection String", 2, ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableNextColumn();
                ImGui::Text("Enter SQL Server name:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##server", serverNameBuffer, IM_ARRAYSIZE(serverNameBuffer));
                ImGui::TableNextColumn();
                ImGui::Text("Enter database name:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##database", databaseNameBuffer, IM_ARRAYSIZE(databaseNameBuffer));
                ImGui::TableNextColumn();
                ImGui::Text("Enter SQL Username:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##user", usernameBuffer, IM_ARRAYSIZE(usernameBuffer));
                ImGui::TableNextColumn();
                ImGui::Text("Enter SQL Password:"); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(fieldLen); ImGui::InputText("##password", passwordBuffer, IM_ARRAYSIZE(passwordBuffer));

                // End SQL entry table
                ImGui::EndTable();
            }

            // Copy char array buffers to strings
            sql.SetSource(std::string(serverNameBuffer));
            sql.SetDatabase(std::string(databaseNameBuffer));
            sql.SetUsername(std::string(usernameBuffer));
            sql.SetPassword(std::string(passwordBuffer));

            // Check that field have been filled out
            bool requiredInfo = sql.requiredInfo(sql.GetSource(), sql.GetDatabase(), sql.GetUsername(), sql.GetPassword());

            if (!sql.GetConnected() && requiredInfo) {
                if (ImGui::Button("Test Connection", ImVec2(120, 60))) {
                    sql.connect();
                }
            }
            else if (!sql.GetConnected() && !requiredInfo) {
                ImGui::BeginDisabled();
                ImGui::Button("Test Connection", ImVec2(120, 60));
                ImGui::EndDisabled();
            }
            else {
                ImGui::BeginDisabled();
                ImGui::Button("SQL Connected!", ImVec2(120, 60));
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            if (ImGui::Button("Close", ImVec2(120, 60))) {
                show_sql_conn_window = false;
                ImGui::CloseCurrentPopup();
            }

            // End SQL Connection popup modal window
            ImGui::EndPopup();
        }

        // Create a program health window to check for required setup at the start.
        static bool directoryFound = createDirectory(directory_path);
        static bool fieldsLoaded = genericFieldCheck();
        if (open_health_check) {
            ImGui::SetNextWindowSize(healthCheckSize);
            ImGui::SetNextWindowPos(healthCheckPos/*, ImGuiCond_Always, ImVec2(1.0f, 0.0f)*/ );

            // Begin Health check window
            ImGui::Begin("Health Check", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove );
            if (ImGui::BeginTable("HealthCheck", 2, ImGuiTableFlags_SizingFixedFit)) {
                // Check for the directory and display it in green if found, red text if not
                ImGui::TableNextColumn();
                ImGui::Text("Main Directory:"); 
                ImGui::TableNextColumn();
                if (directoryFound) {
                    DisplayColoredText(("\tDirectory exists at " + directory_path).c_str(), true);
                }
                else {
                    ImGui::Text(("\tUnable to find/create directory at " + directory_path).c_str(), false);
                }
                ImGui::TableNextColumn();
                // Check generic export field and display it in green text if loaded, red if not.
                ImGui::Text("Files:"); 
                ImGui::TableNextColumn();
                if (fieldsLoaded) {
                    DisplayColoredText("\tGeneric Export fields loaded successfully!", true);
                }
                else {
                    DisplayColoredText("\tUnable to load genericfieldlist.txt! Check it is in the same folder as the exe.", false);
                }
                ImGui::TableNextColumn();
                // Show SQL connection status
                ImGui::Text("SQL Connection Status:"); 
                ImGui::TableNextColumn();
                if (!sql.GetConnectionAttempt()) {
                    DisplayColoredText("\tNo SQL Connection attempted.", false);
                }
                else if (!sql.GetConnected()) {
                    DisplayColoredText("\tSQL Connection failed!", false);
                }
                else {
                    DisplayColoredText("\tSQL Connection Suceeded!", true);
                }

                // End health check table
                ImGui::EndTable();
            }
                

            // End program Health window
            ImGui::End();
        }

        ImGui::SetNextWindowSize(gettingStartedSize);
        ImGui::SetNextWindowPos(gettingStartedPos);
        if (open_getting_started) {
            ImGui::Begin("Getting Started", &open_getting_started, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
            ImGui::TextWrapped("To get started, click on modules from the menu bar and select the module you'd like to use.");

            // End getting started
            ImGui::End();
        }

        // Module Buttons
        ImGui::SetNextWindowSize(moduleButtonSize);
        ImGui::SetNextWindowPos(moduleButtonPos);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, (ImVec2(0, 4.5)));
        ImGui::Begin("Module Selection", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

        // Show generic export generator button 
        if(!show_generic_export_window)
            if (ImGui::Button("Generic Export Generator", moduleSelectionSize)) {
                show_generic_export_window = true;
            }
        if (show_generic_export_window) {
            ImGui::BeginDisabled();
            if(ImGui::Button("Generic Export Generator", moduleSelectionSize));
            ImGui::EndDisabled();
        }
        // Show one button refresh button 
        if (!show_one_button_refresh_window)
            if (ImGui::Button("One Button Database Refresh", moduleSelectionSize)) {
                show_one_button_refresh_window = true;
            }
        if (show_one_button_refresh_window) {
            ImGui::BeginDisabled();
            if (ImGui::Button("One Button Database Refresh", moduleSelectionSize));
            ImGui::EndDisabled();
        }
        // Show sql query builder button 
        if (!show_sql_query_builder_window)
            if (ImGui::Button("SQL Query Builder", moduleSelectionSize)) {
                show_sql_query_builder_window = true;
            }
        if (show_sql_query_builder_window) {
            ImGui::BeginDisabled();
            if (ImGui::Button("SQL Query Builder", moduleSelectionSize));
            ImGui::EndDisabled();
        }

        // End Module Selection window
        ImGui::End();

        // End no window padding
        ImGui::PopStyleVar();

        // Recent Updates window
        ImGui::SetNextWindowSize(recentUpdateSize);
        ImGui::SetNextWindowPos(recentUpdatePos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 0));

        if (open_recent_updates) {
            ImGui::Begin(currVersion, &open_recent_updates, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
            displayUpdates();

            // End recent updates
            ImGui::End();
        }

        // End window padding style
        ImGui::PopStyleVar();

        // Set modules in designated area with tabs
        // Future idea: Make buttons that open modules as tabs and make tabs able to be closed as needed
        ImGui::SetNextWindowPos(modulePos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(moduleSize);
        ImGui::Begin("Modules", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        if (!show_generic_export_window && !show_one_button_refresh_window && !show_sql_query_builder_window) {
            ImGui::Text("Click a button to open that module here. You can have multiple modules open at once.");
        }
        else {
            if (ImGui::BeginTabBar("Modules"), ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs) {
                if (ImGui::BeginTabItem("Generic Export Generator", &show_generic_export_window, ImGuiTabItemFlags_None)) {
                    showGenericExportWindow(&show_generic_export_window);

                    // End Generic export generator tab
                    ImGui::EndTabItem();
                }
                // Add tab for one button refresh window
                if (ImGui::BeginTabItem("OneButton RMS Refresh", &show_one_button_refresh_window, ImGuiTabItemFlags_None)) {
                    showOneButtonRefreshWindow(&show_one_button_refresh_window);

                    // End Refresh tab
                    ImGui::EndTabItem();
                }
                // Add tab for sql generator
                if (ImGui::BeginTabItem("SQL Query Builder - WIP", &show_sql_query_builder_window, ImGuiTabItemFlags_None)) {
                    showSqlQueryBuilderWindow(&show_sql_query_builder_window);

                    // End sql query builder tab
                    ImGui::EndTabItem();
                }
                // Tab for XML parser module
                if (ImGui::BeginTabItem("XML Parser", &show_xml_parser_window, ImGuiTabItemFlags_None)) {
                    xmlParser(directory_path);

                    // End XML Parser
                    ImGui::EndTabItem();
                }

                // End Modules tab
                ImGui::EndTabBar();
            }
        }

        // End modules child window
        ImGui::End();
        
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Close Application")) {
                    return 0;
                }
                // End File menu
                ImGui::EndMenu();
            }
            // Old method of display - replaced with tab view
            //if (ImGui::BeginMenu("Modules")) {
            //    ImGui::SeparatorText("RMS/JMS");
            //    if (ImGui::MenuItem("Generic Export Generator", NULL, false, (fieldsLoaded || directoryFound)))             // Disable module if the field list isn't found or directory isn't created.
            //        show_generic_export_window = true;
            //    if (ImGui::MenuItem("One Button RMS->RMSTRN refresh", NULL, false, directoryFound))                         // Disable module if directory can't be created
            //        show_one_button_refresh_window = true;
            //    ImGui::SeparatorText("SQL Query Wizard");
            //    if (ImGui::MenuItem("SQL Query Builder", NULL, false))
            //        show_sql_query_builder_window = true;

            //    // End Module menu
            //    ImGui::EndMenu();
            //    }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Getting Started",NULL, &open_getting_started));
                if (ImGui::MenuItem("Recent Updates", NULL, &open_recent_updates));
                if (ImGui::MenuItem("Health Check", NULL, &open_health_check));
                ImGui::BeginDisabled(); if (ImGui::MenuItem("XML Parser", NULL, &show_xml_parser_window)); ImGui::EndDisabled();
                if (ImGui::MenuItem("Demo Window", NULL, &show_demo_window));

                // End View menu
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings")) {
                // Wasn't as simple as I hoped, need to look into this in the future
                ImGui::BeginDisabled();
                if (ImGui::BeginMenu("Font Size")) {
                    if (ImGui::MenuItem("12 pt"))
                        font_size = 12.0f;
                    if (ImGui::MenuItem("14 pt"), NULL, true)
                        font_size = 14.0f;
                    if (ImGui::MenuItem("16 pt"))
                        font_size = 16.0f;
                    // End Font Size menu
                    ImGui::EndMenu();
                }
                ImGui::EndDisabled();

                if (ImGui::BeginMenu("SQL Connection Settings")) {
                    if (ImGui::MenuItem("Open SQL Configuration Window", NULL, &show_sql_conn_window));

                    //End SQL Settings menu
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Display")) {
                    if (ImGui::BeginMenu("Style")) {
                        if (ImGui::MenuItem("Light")) {
                            ImGui::StyleColorsLight();
                        }
                        if (ImGui::MenuItem("Dark")) {
                            ImGui::StyleColorsDark();
                        }
                        // End Style Menu
                        ImGui::EndMenu();
                    }

                    // End Display Menu
                    ImGui::EndMenu();
                }
                
                // End settings menu
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        
        // Legacy way of showing each module in it's own window. Have since moved to tabs in a fixed area
        // Begin GenericExport Window
        /*if(show_generic_export_window)            
            showGenericExportWindow(&show_generic_export_window);*/
        // Begin One Button Refresh Window
        //if (show_one_button_refresh_window)
        //    showOneButtonRefreshWindow(&show_one_button_refresh_window);
        //// Begin SQL Query Builder window
        //if (show_sql_query_builder_window) {
        //    ImGui::SetNextWindowSize(ImVec2(350, 200));
        //    showSqlQueryBuilderWindow(&show_sql_query_builder_window);
        //}

        // Set the background color
        // ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = bg_color;

        /*if (open_bgcolor_picker) {
            colorPickerWithBackgroundChange(bg_color);
        }*/

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        //{
        //    static float f = 0.0f;
        //    static int counter = 0;

        //    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        //    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        //    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        //    ImGui::Checkbox("Another Window", &show_another_window);

        //    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        //    ImGui::ColorEdit3("clear color", (float*)&bg_color); // Edit 3 floats representing a color

        //    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        //        counter++;
        //    ImGui::SameLine();
        //    ImGui::Text("counter = %d", counter);

        //    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        //    ImGui::End();
        //}

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(bg_color.x*bg_color.w*255.0f), (int)(bg_color.y*bg_color.w*255.0f), (int)(bg_color.z*bg_color.w*255.0f), (int)(bg_color.w*255.0f));
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (result == D3DERR_DEVICELOST)
            g_DeviceLost = true;
    }

    // Cleanup
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
