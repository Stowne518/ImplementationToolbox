// Dear ImGui: standalone example application for DirectX 9

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

/*
*****Author: Alex Towne
*****Date: Began on 2/1/24
*****Last Updated: 4/09/25
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
#include "Units.h"
#include "unitImport.h"
#include "popups.h"
#include "unitbuilder.h"
#include "mapImport.h"
#include "servlogViewer.h"
#include "UserSettings.h"
#include <Windows.h>
#include "genericDataImport.h"
#include "AppLog.h"



// Get center of monitor from Windows API
static POINT getScreenCenter() {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    POINT center;
    center.x = screenWidth / 2;
    center.y = screenHeight / 2;
    return center;
}

static POINT getScreenSize()
{
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    POINT size;
    size.x = screenWidth;
    size.y = screenHeight;
    return size;
}

// Get window position from Windows
static POINT getWindowPos(HWND hwnd) {
	RECT rect;
	GetWindowRect(hwnd, &rect);
	POINT pos;
	pos.x = rect.left;
	pos.y = rect.top;
	return pos;
}

// Get window size from Windows
static POINT getWindowSize(HWND hwnd) {
	RECT rect;
	GetWindowRect(hwnd, &rect);
	POINT size;
	size.x = rect.right - rect.left;
	size.y = rect.bottom - rect.top;
	return size;
}

constexpr auto settings_filename = "ImplementationToolbox.ini";
static bool isDarkMode = false;

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
	static float w_width, w_height, wx_pos, wy_pos;
    static int w_darkmode;
	static bool getting_started, recent_update, health_check, debug_log, modules;
    std::string conn_str;
    
    // Initialize AppLog
    static AppLog log;

    // Create a static sql class to store connection info in so we can pass to different functions that may need the SQL connectivity
    static Sql sql;

    // Load initial settings
    UserSettings usrsettings;
	usrsettings.loadSettings(settings_filename);                        // Load all settings/ create default file
	isDarkMode = usrsettings.getDarkMode();                             // Load dark mode setting from file
    const int WIDTH = usrsettings.getWindowWidth();                     // Initial window width definition
    const int HEIGHT = usrsettings.getWindowHeight();                   // Initial window height definition
    const auto START_X = usrsettings.getWindowPosX();                   // Get center screen x coordinate - based on window size
    const auto START_Y = usrsettings.getWindowPosY();                   // Get center screen y coordinate - based on window size
    // Window open state
    static bool open_getting_started = usrsettings.getGettingStarted();
    static bool open_recent_updates = usrsettings.getRecentUpdates();
    static bool open_health_check = usrsettings.getHealthCheck();
    static bool show_log = usrsettings.getDebugLog();
    static bool show_modules = usrsettings.getModules();

    // Change both version nums at the same time, haven't found a way to convert from wchar_t to char* yet.
    const wchar_t* versionNum = L"Implementation Toolbox v0.6.7";
    const char* currVersion = "Implementation Toolbox v0.6.7";
    const std::string version_number = "v0.6.7";
    const char* lastUpdate = "5/6/25";

    // Button labels
    static char genExprtLabel[] = "Generic Export Generator";
    static char oneBttnLabel[] = "One Button Database Refresh";
    static char sqlQryLabel[] = "SQL Query Builder";
    // static char unitImportLabel[] = "Unit Bulk Import";              -- DEPRECATED
    // static char mapDataImportLabel[] = "Map Data Import";            -- DEPRECATED
    static char servlogLabel[] = "Servlog Viewer";
    static char genericDataImportLabel[] = "Generic Data Import";

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, versionNum, nullptr };

    // Load the icons
    wc.hIcon = (HICON)LoadImage(NULL, _T("Images/CST_LOGO_icon_64x64.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
    wc.hIconSm = (HICON)LoadImage(NULL, _T("Images/CST_LOGO_icon_32x32.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
    
    // Create window
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, versionNum, WS_OVERLAPPEDWINDOW, START_X, START_Y, WIDTH, HEIGHT, nullptr, nullptr, wc.hInstance, nullptr);

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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;        // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    ImGuiViewport* windowPos = ImGui::GetMainViewport();
    ImVec2 windowPosCoords = windowPos->Pos;
    static double windowPosX = windowPosCoords.x;
    static double windowPosY = windowPosCoords.y;

    if(isDarkMode)
    {
        ImGui::StyleColorsDark();
    }
    else
    {
        ImGui::StyleColorsLight();
    }

    // Darken the input text boxes -- MADE CHANGES TO StyleColorsLight(); directly
    static ImGuiStyle& style = ImGui::GetStyle();
    //style.Colors[ImGuiCol_FrameBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.7f); // Darker gray for input box background
    //style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // Slightly darker gray when hovered
    //style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // Even darker gray when active

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

    static float font_size = 18.0f;
    ImFontConfig config;
    // Try to load an Segoe UI or Arial font from windows directory
    try
    {
        if (io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", font_size, &config, nullptr));
    }
    catch (const std::exception&)
    {
        log.AddLog("[ERROR] Failed to load segoeui.ttf from windows. Trying next font.\n");
    }
    try
    {
        if (io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", font_size));
    }
    catch (const std::exception&)
    {
        log.AddLog("[ERROR] Failed to load arial.ttf from windows. Trying next font.\n");
    }
    // If we can't find it use the default one
    try
    {
        if (io.Fonts->AddFontDefault());
    }
    catch (const std::exception&)
    {
        log.AddLog("[ERROR] Failed to load default font Closing application.\n");
        std::cerr << "[ERROR] Failed to load default font Closing application.";
        return 1;
    }
    
    io.Fonts->Build();
    ImFontAtlas* font_atlas = io.Fonts;

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;

    // My boolean states
    bool show_generic_export_window = false;
    bool show_one_button_refresh_window = false;
    bool show_sql_query_builder_window = false;
    bool show_xml_parser_window = false;
    // bool show_unit_import_window = false;        -- DEPRECATED
    // bool show_map_import_window = false;         -- DEPRECATED
    bool show_servlog_viewer = false;
	bool show_generic_import_data_window = false;
    bool show_sql_conn_window = false;

    // Popup window states
    bool gen_export_info = false;
    
    

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

        // Check and log state changes
        log.logStateChange("show_demo_window", show_demo_window);
        log.logStateChange("show_another_window", show_another_window);
        log.logStateChange("show_generic_export_window", show_generic_export_window);
        log.logStateChange("show_one_button_refresh_window", show_one_button_refresh_window);
        log.logStateChange("show_sql_query_builder_window", show_sql_query_builder_window);
        log.logStateChange("show_xml_parser_window", show_xml_parser_window);
        log.logStateChange("show_servlog_viewer", show_servlog_viewer);
        log.logStateChange("show_generic_import_data_window", show_generic_import_data_window);
        log.logStateChange("show_sql_conn_window", show_sql_conn_window);
        log.logStateChange("show_modules", show_modules);
        log.logStateChange("gen_export_info", gen_export_info);
        log.logStateChange("open_getting_started", open_getting_started);
        log.logStateChange("open_recent_updates", open_recent_updates);
        log.logStateChange("open_health_check", open_health_check);
        log.logStateChange("isDarkMode", isDarkMode);
		log.logStateChange("show_log", show_log);
        log.logStateChange("show_modules", show_modules);

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

		w_darkmode = usrsettings.getDarkMode();                 // Load dark mode setting for comparison
		w_width = usrsettings.getWindowWidth();                 // Load window width for comparison
		w_height = usrsettings.getWindowHeight();               // Load window height for comparison
		wx_pos = usrsettings.getWindowPosX();                   // Load window x position for comparison
		wy_pos = usrsettings.getWindowPosY();                   // Load window y position for comparison
		getting_started = usrsettings.getGettingStarted();      // Load getting started window state for comparison
		recent_update = usrsettings.getRecentUpdates();         // Load recent updates window state for comparison
		health_check = usrsettings.getHealthCheck();            // Load health check window state for comparison
		debug_log = usrsettings.getDebugLog();                  // Load debug log window state for comparison
        modules = usrsettings.getModules();                     // Load module window state

		// Check if any settings have changed that we save to the file
		if (
            w_width != getWindowSize(hwnd).x 
            || w_height != getWindowSize(hwnd).y 
            || wx_pos != getWindowPos(hwnd).x 
            || wy_pos != getWindowPos(hwnd).y 
            || w_darkmode != isDarkMode 
            || getting_started != open_getting_started 
            || recent_update != open_recent_updates 
            || health_check != open_health_check 
            || debug_log != show_log
            || modules != show_modules
            )
		{
			// Save the new window size and position
			usrsettings.setWindowHeight(getWindowSize(hwnd).y);
			usrsettings.setWindowWidth(getWindowSize(hwnd).x);
			usrsettings.setWindowPosX(getWindowPos(hwnd).x);
			usrsettings.setWindowPosY(getWindowPos(hwnd).y);
			usrsettings.setDarkMode(isDarkMode);
            if (open_getting_started) usrsettings.setGettingStarted('Y'); else usrsettings.setGettingStarted('N');
			if (open_health_check) usrsettings.setHealthCheck('Y'); else usrsettings.setHealthCheck('N');
			if (open_recent_updates) usrsettings.setRecentUpdates('Y'); else usrsettings.setRecentUpdates('N');
			if (show_log) usrsettings.setDebugLog('Y'); else usrsettings.setDebugLog('N');
            if (show_modules) usrsettings.setModules('Y'); else usrsettings.setModules('N');
			usrsettings.saveSettings(settings_filename, log);
            sql._SetSavedString(false); // Reset saved string if we save new one to file
		}

        // Get main viewport
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_center = ImGui::GetMainViewport()->GetCenter();

        // Enable dockspace
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_None);

		// Get list of connection strings
        static std::vector<std::string> connectionStrings = getListOfConnStrings();

        // Begin main menu bar
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Close Application"))
                {
                    return 0;
                }
                // End File menu
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Getting Started", NULL, &open_getting_started));
                if (ImGui::MenuItem("Modules", NULL, &show_modules));
                if (ImGui::MenuItem("Recent Updates", NULL, &open_recent_updates));
                if (ImGui::MenuItem("Health Check", NULL, &open_health_check));
				if (ImGui::MenuItem("Debug Log", NULL, &show_log));
                /*ImGui::BeginDisabled(); if (ImGui::MenuItem("XML Parser", NULL, &show_xml_parser_window)); ImGui::EndDisabled();*/
                if (ImGui::MenuItem("Demo Window", NULL, &show_demo_window));

                // End View menu
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Modules"))
            {
                ImGui::SeparatorText("RMS/JMS");
                if (ImGui::MenuItem(genExprtLabel, NULL, &show_generic_export_window));
                if (ImGui::MenuItem(oneBttnLabel, NULL, &show_one_button_refresh_window));
                ImGui::SeparatorText("CAD");
                if (ImGui::MenuItem(servlogLabel, NULL, &show_servlog_viewer));
                ImGui::SeparatorText("SQL");
                if (ImGui::MenuItem(sqlQryLabel, NULL, &show_sql_query_builder_window));
                if (ImGui::MenuItem(genericDataImportLabel, NULL, &show_generic_import_data_window));

                // End Modules menu
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings"))
            {
                if (ImGui::BeginMenu("Fonts"))
                {
                    ImGui::ShowFontSelector("Fonts");
                    // End Font Menu
                    ImGui::EndMenu();
                }
                // Open popup for SQL settings
                if (ImGui::MenuItem("Open SQL Configuration Window", NULL, &show_sql_conn_window));
                if(connectionStrings.size() > 0)
                {
                    if (ImGui::BeginMenu("Quick SQL Connection"))
                    {
                        static ImGuiTextFilter conn_filter;
                        conn_filter.Draw("Filter##Connections", 110.0f);
                        for (int i = 0; i < connectionStrings.size(); i++)
                        {
                            if(conn_filter.PassFilter(connectionStrings[i].c_str()))
                            {
                                ImGui::PushID(i);
                                if (ImGui::MenuItem(("%s", connectionStrings[i]).c_str()))
                                {
                                    try
                                    {
                                        // Load the connection string from the file
                                        std::string conn_str = ("C:\\ImplementationToolbox\\ConnectionStrings\\" + connectionStrings[i]);
                                        // Set the connection string in the SQL class
                                        sql._SetConnectionAttempt();            // Show we attempted connection
                                        sql._SetConnected(sql.readConnString(conn_str));
                                        log.AddLog("[INFO] Loading connection: %s\n", connectionStrings[i].c_str());
                                        if (!sql._GetConnected())
                                        {
                                            log.AddLog("[ERROR] Failed to connect to: %s\n", sql._GetSource().c_str());
                                        }
                                        else
                                        {
                                            log.AddLog("[INFO] Successfully connected to: %s\n", sql._GetSource().c_str());
                                            // Set the connection string in the SQL class
                                            // sql._SetConnectionString();
                                        }
                                    }
                                    catch (std::exception& e)
                                    {
                                        log.AddLog("[ERROR] Failed to load connection string: %s\n", e.what());
                                    }
                                }
                                ImGui::PopID();
                            }
                        }
                        // End Connection string menu
                        ImGui::EndMenu();
                    }
                }

                // End settings menu
                ImGui::EndMenu();
            }
            if (isDarkMode)
            {
                if (ImGui::Button("Light Mode"))
                {
                    ImGui::StyleColorsLight();
                    isDarkMode = false;
                }
            }
            else
            {
                if (ImGui::Button("Dark Mode"))
                {
                    ImGui::StyleColorsDark();
                    isDarkMode = true;
                }
            }
            if (show_generic_export_window)
            {
                if (ImGui::BeginMenu("Generic Export Generator Options"))
                {
                    // Center window when it opens
                    ImGui::SetNextWindowPos(window_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    if (ImGui::MenuItem("Information", NULL, &gen_export_info));
                    if (ImGui::MenuItem("Help"));

                    // End Gen Exprt Gen menu options
                    ImGui::EndMenu();
                }
            }
            
            //saveDarkModeSettings("ImplementationToolbox.ini", isDarkMode);
            ImGui::EndMainMenuBar();
        }

		// Show log if enabled
        if (show_log)
        {
			log.Draw("Debug Log", &show_log);
        }

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
            module_buttons_x = ImGui::GetWindowWidth(),
            module_buttons_y = 430,
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
            module_button_size_y = 50;        

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
            moduleButtonPos = ImVec2(module_buttons_posx, module_buttons_posy);
            // Size of module buttons to select from
            //moduleSelectionSize = ImVec2(module_button_size_x, module_button_size_y);

        // Window constraint min/max for module windows
        const ImVec2 window_min = ImVec2(600, 400);
        const ImVec2 window_max = ImVec2(1920, 1080);

        static std::string directory_path = "C:\\ImplementationToolbox\\";
        static std::string units_directory_path = directory_path + "Units\\";

        // Display the CST logo at the top of the screen
        //ImGui::SetNextWindowPos(cstLogoPos);
        //ImGui::SetNextWindowSize(cstLogoSize);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, cst_purple);
        ImGui::Begin("cstlogo", NULL, ImGuiWindowFlags_NoDecoration);
        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Image((ImTextureID)(intptr_t)my_texture, ImVec2(my_image_width, my_image_height));
        // End logo image window
        ImGui::End();

        // End orange color
        ImGui::PopStyleColor();
        
        // Open the SQL configuration window if the menu item is selected
        if (show_sql_conn_window)
        {
            ImGui::SetNextWindowPos(window_center);
            sql.DisplaySqlConfigWindow(&show_sql_conn_window, directory_path + "ConnectionStrings\\", log);
        }

        // Create a program health window to check for required setup at the start.
        static bool directoryFound = createDirectory(directory_path, log);
        static bool fieldsLoaded = genericFieldCheck();
        if (open_health_check) {
            //ImGui::SetNextWindowSize(healthCheckSize);
            //ImGui::SetNextWindowPos(healthCheckPos/*, ImGuiCond_Always, ImVec2(1.0f, 0.0f)*/ );

            // Begin Health check window
            ImGui::Begin("Health Check", &open_health_check, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar );
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
                if (!sql._GetConnectionAttempt()) {
                    DisplayColoredText("\tNo SQL Connection attempted.", false);
                }
                else if (!sql._GetConnected()) {
                    DisplayColoredText("\tSQL Connection failed!", false);
                }
                else {
                    DisplayColoredText("\tSQL Connection Succeeded!", true);
                    ImGui::SetItemTooltip("Connection info:\nServer: %s\nDatabase: %s\nUsername: %s", sql._GetSource(), sql._GetDatabase(), sql._GetUsername());
                }

                // End health check table
                ImGui::EndTable();
            }
                

            // End program Health window
            ImGui::End();
        }

        //ImGui::SetNextWindowSize(gettingStartedSize);
        //ImGui::SetNextWindowPos(gettingStartedPos);
        if (open_getting_started) {
            ImGui::Begin("Getting Started", &open_getting_started, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
            ImGui::TextWrapped("To get started, click on modules from the menu bar and select the module you'd like to use.");

            // End getting started
            ImGui::End();
        }

        if(show_modules)
        {
            ImVec2 moduleSelectionSize = ImVec2(ImGui::GetWindowWidth(), module_button_size_y);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, (ImVec2(0, 4.5)));
            ImGui::Begin("Module Selection", &show_modules);
            
            // Show generic export generator button 
            ImGui::SeparatorText("RMS/JMS");
            if (!show_generic_export_window)
            {
                if (ImGui::Button(genExprtLabel, ImVec2(ImGui::GetWindowWidth(), module_button_size_y)))
                {
                    show_generic_export_window = true;
                }
            }
            else
            {
                showDisabledButton(genExprtLabel, ImVec2(ImGui::GetWindowWidth(), module_button_size_y));
            }
            // Show one button refresh button 
            if (!show_one_button_refresh_window)
            {
                if (ImGui::Button(oneBttnLabel, ImVec2(ImGui::GetWindowWidth(), module_button_size_y)))
                {
                    show_one_button_refresh_window = true;
                }
            }
            else
            {
                showDisabledButton(oneBttnLabel, ImVec2(ImGui::GetWindowWidth(), module_button_size_y));
            }

            ImGui::SeparatorText("CAD");
            if (!show_servlog_viewer)
            {
                if (ImGui::Button(servlogLabel, ImVec2(ImGui::GetWindowWidth(), module_button_size_y)))
                {
                    show_servlog_viewer = true;
                }
            }
            else
            {
                showDisabledButton(servlogLabel, ImVec2(ImGui::GetWindowWidth(), module_button_size_y));
            }
            ImGui::SeparatorText("SQL");
            // Show sql query builder button 
            if (!show_sql_query_builder_window)
            {
                if (ImGui::Button(sqlQryLabel, ImVec2(ImGui::GetWindowWidth(), module_button_size_y)))
                {
                    show_sql_query_builder_window = true;
                }
            }
            else
            {
                showDisabledButton(sqlQryLabel, ImVec2(ImGui::GetWindowWidth(), module_button_size_y));
            }
            if (!show_generic_import_data_window)
            {
                if (ImGui::Button(genericDataImportLabel, ImVec2(ImGui::GetWindowWidth(), module_button_size_y)))
                {
                    show_generic_import_data_window = true;
                }
            }
            else
            {
                showDisabledButton(genericDataImportLabel, ImVec2(ImGui::GetWindowWidth(), module_button_size_y));
            }

            // End Module Selection window
            ImGui::End();

            // End no window padding
            ImGui::PopStyleVar();
        }

        // Recent Updates window
        //ImGui::SetNextWindowSize(recentUpdateSize);
        //ImGui::SetNextWindowPos(recentUpdatePos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 0));

        if (open_recent_updates) 
        {
            ImGui::Begin(currVersion, &open_recent_updates, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
            displayUpdates();

            // End recent updates
            ImGui::End();
        }

        // End window padding style
        ImGui::PopStyleVar();
        

        //Start windows if the option is selected
        if (show_generic_export_window)
        {
            ImGui::SetNextWindowSizeConstraints(window_min, window_max);
            ImGui::Begin(genExprtLabel, &show_generic_export_window, ImGuiWindowFlags_HorizontalScrollbar);
            showGenericExportWindow(&show_generic_export_window, sql, log);
            ImGui::End();
        }
        if (show_one_button_refresh_window)
        {
            ImGui::SetNextWindowSizeConstraints(window_min, window_max);
            ImGui::Begin(oneBttnLabel, &show_one_button_refresh_window);
            showOneButtonRefreshWindow(&show_one_button_refresh_window, log);
            ImGui::End();
        }
        if (show_servlog_viewer)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
            ImGui::SetNextWindowSizeConstraints(window_min, window_max);
            ImGui::Begin(servlogLabel, &show_servlog_viewer);
            servlogViewer(&show_servlog_viewer, sql, log, usrsettings);
            ImGui::End();
            ImGui::PopStyleVar();
        }
        if (show_sql_query_builder_window)
        {
            ImGui::SetNextWindowSizeConstraints(window_min, window_max);
            ImGui::Begin(sqlQryLabel, &show_sql_query_builder_window);
            showSqlQueryBuilderWindow(&show_sql_query_builder_window, log);
            ImGui::End();
        }
        if (show_generic_import_data_window)
        {
            ImGui::SetNextWindowSizeConstraints(window_min, window_max);
            ImGui::Begin(genericDataImportLabel, &show_generic_import_data_window);
            genericDataImport(&show_generic_import_data_window, sql, log, directory_path);
            ImGui::End();
        }

        // Set modules in designated area with tabs
        //ImGui::SetNextWindowPos(modulePos, ImGuiCond_Always);
        //ImGui::SetNextWindowSize(moduleSize);
    //    ImGui::Begin("Modules", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    //    if (!show_generic_export_window && !show_one_button_refresh_window && !show_sql_query_builder_window && !show_generic_import_data_window && !show_servlog_viewer) 
    //    {
    //        ImGui::Text("Click a button to open that module here. You can have multiple modules open at once.");
    //    }
    //    else 
    //    {
    //        if (ImGui::BeginTabBar("Modules"), ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs) 
    //        {
    //            if (ImGui::BeginTabItem(genExprtLabel, &show_generic_export_window, ImGuiTabItemFlags_None)) 
    //            {
    //                showGenericExportWindow(&show_generic_export_window, sql);

    //                // End Generic export generator tab
    //                ImGui::EndTabItem();
    //            }
    //            // Add tab for one button refresh window
    //            if (ImGui::BeginTabItem(oneBttnLabel, &show_one_button_refresh_window, ImGuiTabItemFlags_None)) 
    //            {
    //                showOneButtonRefreshWindow(&show_one_button_refresh_window);

    //                // End Refresh tab
    //                ImGui::EndTabItem();
    //            }
    //            // Add tab for servlog viewer
				//if (ImGui::BeginTabItem(servlogLabel, &show_servlog_viewer, ImGuiTabItemFlags_None))
				//{
				//	servlogViewer(&show_servlog_viewer);
				//	// End servlog viewer tab
				//	ImGui::EndTabItem();
				//}
    //            // Add tab for sql generator
    //            if (ImGui::BeginTabItem(sqlQryLabel, &show_sql_query_builder_window, ImGuiTabItemFlags_None)) 
    //            {
    //                showSqlQueryBuilderWindow(&show_sql_query_builder_window);

    //                // End sql query builder tab
    //                ImGui::EndTabItem();
    //            }
    //            // Tab for XML parser module
    //            //if (ImGui::BeginTabItem("XML Parser", &show_xml_parser_window, ImGuiTabItemFlags_None)) 
    //            //{
    //            //    xmlParser(directory_path);

    //            //    // End XML Parser
    //            //    ImGui::EndTabItem();
    //            //}
    //            // Map Import for CAD   -- DEPRECATED
    //            //if (ImGui::BeginTabItem(mapDataImportLabel, &show_map_import_window, ImGuiTabItemFlags_None))
    //            //{
    //            //    mapImport(sql, directory_path);

    //            //    // End Map Import tab
    //            //    ImGui::EndTabItem();
    //            //}
    //            //// Unit Import for CAD    -- DEPRECATED
    //            //if (ImGui::BeginTabItem(unitImportLabel, &show_unit_import_window, ImGuiTabItemFlags_None)) 
    //            //{
    //            //    unitBuilder(&show_unit_import_window, sql, units_directory_path);

    //            //    // End Unit Import Window
    //            //    ImGui::EndTabItem();
    //            //}                
    //            if (ImGui::BeginTabItem(genericDataImportLabel, &show_generic_import_data_window, ImGuiTabItemFlags_None))
    //            {
				//	genericDataImport(sql, log, directory_path);

				//	// End Generic Data Import tab
				//	ImGui::EndTabItem();
    //            }

    //            // End Modules tab
    //            ImGui::EndTabBar();
    //        }
    //    }

    //    // End modules child window
    //    ImGui::End();        

        if (gen_export_info) { genExportInfoModPop(&gen_export_info); }

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

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
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
