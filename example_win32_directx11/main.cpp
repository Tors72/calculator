// Dear ImGui: standalone example application for DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <chrono>
#include <iostream>
#include <cmath>   //ERROR BECAUSE CALC.EXE IN ANOTHER SUBFOLDER THAN EXPECTED

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code

int main(int, char**)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

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
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    int width = GetSystemMetrics(SM_CXSCREEN); // Get screen width
    int height = GetSystemMetrics(SM_CYSCREEN); // Get screen height
    bool opened = true;
    bool done = false;

    char name[30] = ""; 
    float clr[3] = { 0.0f, 0.0f, 0.0f }; 
    bool show_color_picker = false;
    int button_clicks = 0; 
    bool checkbox = false;
    bool show_calculator = false; // Flag to show/hide the second window

    int intslider = 10; 
    float fslider = 10.0f; //slider variable

    static auto last_time = std::chrono::high_resolution_clock::now(); // Last frame time
    static int fps = 0; // Frames per second counter
    static int frames = 0; // Frame counter
    static auto fps_time = std::chrono::high_resolution_clock::now(); // Time for FPS calculation

    while (!done)
    {
        frames++; // Increment frame count
        auto now = std::chrono::high_resolution_clock::now(); // Get current time
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - fps_time).count(); // Calculate duration since last FPS update
        if (duration > 1000) { // If more than 1 second has passed
            fps = frames; // Update FPS
            frames = 0; // Reset frame count
            fps_time = now; // Update FPS time
        }
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

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        //ImGui Style
        ImGuiStyle& style = ImGui::GetStyle();

        // Window
        ImGui::SetNextWindowSize(ImVec2(500, 500));
        ImGui::Begin("Dear ImGui DirectX11 Calculator App", &opened, ImGuiWindowFlags_MenuBar);


        // Window Elements

        ImGui::Text("Dimensions: %d x %d", width, height);
        ImGui::Text("FPS: %d", fps);

        ImGui::InputText("Name", name, IM_ARRAYSIZE(name));

        if (strlen(name) == 0)
            ImGui::Text("Hello world!");
        else
            ImGui::Text("Hello %s", name);
        ImGui::Text("Button Clicks = %d", button_clicks);

        if (ImGui::Button("Click Me"))
            button_clicks++;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("This is a tooltip for the button");

        if (ImGui::Button("Reset"))
            button_clicks = 0;

        if (ImGui::Button("Open Calc(short for calculator)"))
        {
            show_calculator = true;
        }
        ImGui::SliderInt("Slider", &intslider, 0, 100);
        ImGui::InputInt("Input Int", &intslider);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("This is a tooltip for the slider");

        ImGui::SliderFloat("##floatslider", &fslider, -50.0f, 100.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("try to find the value of pi ");
        if (fslider == 3.141f)
            ImGui::Text("Good work");
        if (ImGui::Checkbox("Cheat(loser)", &checkbox)) {

            if (checkbox)
                fslider = 3.141f;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(" ");

        if (ImGui::Button("Color Picker"))
            show_color_picker = !show_color_picker;
        if (show_color_picker)
            ImGui::ColorPicker3("Color", clr);


        ImGui::End();

        if (show_calculator)
        {
            static int a = 0;
            static int b = 0;
            static int result = 0;
            static bool next = false;

            static bool subtract = false; static bool add = false;  static bool multiply = false; static bool divide = false; static bool power = false; //operators

            ImGuiStyle& style = ImGui::GetStyle();

            ImGui::Begin("Actual Calculator", &show_calculator); // Pass pointer for close button
            ImGui::Text("Calculator");
            ImGui::Text("");
            ImGui::Text("first number = %i", a);
            ImGui::Text("second number = %i", b);
            ImGui::Text("result = %i", result);

            for (int i = 1; i <= 9; i++) {                        //NUMBER BUTTONS
                char label[8];
                snprintf(label, sizeof(label), "%d", i);
                if (ImGui::Button(label, ImVec2(60, 60))) {
                    if (next) {
                        if (b != 0) {
                            b = (b * 10) + i;
                        }
                        else
                            b = i;
                    }
                    else {
                        if (a != 0) {
                            a = (a * 10) + i;
                        }
                        else
                            a = i;
                    }
                }
                if (i % 3 != 0) ImGui::SameLine();
                else {}
            }

            if (ImGui::Button("Clear", ImVec2(60, 60))) {                    //CLEAR BUTTON
                a = 0; b = 0;
                next = false;
                add = false; subtract = false; multiply = false; divide = false; result = 0;
            }

            ImGui::SameLine();

            if (ImGui::Button("0", ImVec2(60, 60))) {                        //ZERO BUTTON
                if (next) {
                    if (b != 0) {
                        b = (b * 10);
                    }
                    else
                        b = 0;
                }
                else {
                    if (a != 0) {
                        a = (a * 10);
                    }
                    else
                        a = 0;
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("=", ImVec2(60, 60))) {
                result = (add) ? (a + b) : ((subtract) ? (a - b) : ((multiply) ? (a * b) : ((divide) ? (a / b) : (power) ? pow(a, b) : 0)));
                //added parentheses for clarification
            }

            ImGui::NewLine();

            if (ImGui::Button("/", ImVec2(60, 60))) { //  / BUTTON
                if (a != 0) {
                    next = true; divide = true;
                }
            } ImGui::SameLine();

            if (ImGui::Button("*", ImVec2(60, 60))) { //* BUTTON
                if (a != 0) {
                    next = true; multiply = true;
                }
            } ImGui::SameLine();

            if (ImGui::Button("+", ImVec2(60, 60))) { //+ BUTTON
                if (a != 0) {
                    next = true; add = true;
                }
            } ImGui::SameLine();

            if (ImGui::Button("-", ImVec2(60, 60))) { //- BUTTON
                if (a != 0) {
                    next = true; subtract = true;
                }
            } ImGui::SameLine();

            if (ImGui::Button("^x", ImVec2(60, 60))) { //^x BUTTON
                if (a != 0) {
                    next = true; power = true;
                }
            }  ImGui::End();
        }



        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        //HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync 
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

    }
    // Cleanup
    ImGui_ImplDX11_Shutdown();
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
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
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
