
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <chrono>
#include <iostream>
#include <cmath>  

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
	ImGuiIO& io = ImGui::GetIO(); (void)io;  //FONTS
	io.Fonts->AddFontFromFileTTF("imgui/misc/fonts/Cousine-Regular.ttf", 24.0f);

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

	// state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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

		style.Colors[ImGuiCol_Button] = ImVec4( 203 / 255.0f, 255 / 255.0f, 203 / 255.0f, 1.0f); //205,155,89
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4( 143 / 255.0f, 78 / 255.0f, 81 / 255.0f, 1.0f); //143, 78, 81
		style.Colors[ImGuiCol_ButtonActive] = ImVec4( 108 / 255.0f, 37 / 255.0f, 103 / 255, 1.0f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4( 221 / 255.0f, 221 / 255.0f, 221 / 255.0f, 1.0f); // 209, 176, 132
		style.Colors[ImGuiCol_Text] = ImVec4(.0f, .0f, .0f, 1.0f);

		style.FrameRounding = 10.0f; 
		style.WindowRounding = 5.0f;


		// Window
	   
			static long double a = 0; static long double b = 0; static long double result = 0;
			static bool next = false;  // next turns true on clicking an operator button, so that the numbers after the operator are added to b instead of a

			static bool decadd = false;
			static int a_dec_places = 0;  // for adding values before the decimal point
			static int b_dec_places = 0; 

			static bool subtract = false; static bool add = false;  static bool multiply = false; static bool divide = false; static bool power = false; //operators
			static bool root = false;

			ImGui::Begin("Calculator"); 

			( a == int (a) ) 
				? ImGui::Text("first number = %i", (int)a) 
				: ImGui::Text("first number = %Lf", a);              //truncates value if integer

			( b == int(b) ) 
				? ImGui::Text("second number = %i", (int)b)    
				: ImGui::Text("second number = %Lf", b);      

			(result == int(result)) 
				? ImGui::Text("result = %i", (int)result)      
				: ImGui::Text("result = %Lf", result);
			

			for (int i = 1; i <= 9; i++) {                        // buttons for numbers 1-9
				char label[8];
				snprintf(label, sizeof(label), "%i", i);
				ImGuiKey keycode = (ImGuiKey)(ImGuiKey_0 + i);
				if (ImGui::Button(label, ImVec2(80, 80)) || ImGui::IsKeyPressed(keycode)) { // add key event for number keys
					(next)
						? (decadd
							? (b += i * pow(10.0f, --b_dec_places))
							: (b = b * 10 + i))
						: (decadd
							? (a += i * pow(10.0f, --a_dec_places))
							: (a = a * 10 + i));
				}
					if (i % 3 != 0) ImGui::SameLine();
				
			}
			// ImGui::PushStyleColor(ImGuiCol_Button, ImVec4( 161 / 255.0f , 214 / 255.0f, 161 / 255.0f , 1.0f));

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4( 147 / 255.0f, 204 / 255.0f, 147 / 255.0f, 1.0f));
			  {
				if (ImGui::Button("CE", ImVec2(80, 80)) || ImGui::IsKeyPressed(ImGuiKey_C)) {                    // clear button
					a = 0; b = 0;
					next = false;
					add = false; subtract = false; multiply = false; divide = false; power = false; root = false; decadd = false; result = 0; a_dec_places = 0; b_dec_places = 0;
				} ImGui::SameLine();
			  }   
			ImGui::PopStyleColor();

			if (ImGui::Button("0", ImVec2(80, 80)) || ImGui::IsKeyPressed(ImGuiKey_0)) {                      // zero button
				(next && b != 0)
					? (b = (b * 10) + 0)
					: (next)
						? (b = 0)
						: (a != 0)
							? (a = (a * 10) + 0)
							: (a = 0);
			}
			  ImGui::SameLine();

			  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(147 / 255.0f, 204 / 255.0f, 147 / 255.0f, 1.0f));
			  
			  if (ImGui::Button("=", ImVec2(80, 80)) || ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
				  int x;
				  result == 0
					  ? x = a
					  : x = result;
				  result = add
					  ? (x + b)
					  : subtract
						? (x - b)
						: multiply
						  ? (x * b)
						  : divide
							? (x / b)
							: power
							  ? pow(x, b)
							  : root
								? pow(x, 1 / b)
								: 0;
			  } ImGui::PopStyleColor(1);
			   

			ImGui::NewLine(); //161, 214, 161
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4( 161 / 255.0f , 214 / 255.0f, 161 / 255.0f , 1.0f));

			if (ImGui::Button("/", ImVec2(80, 80)) && a != 0 ) { // button for divison
				next = true; divide = true; decadd = false;
			} ImGui::SameLine();
			if ( ImGui::IsKeyPressed(ImGuiKey_Slash) && a != 0 ) { // add key event for division
				next = true; divide = true; decadd = false;
			}

			if (ImGui::Button("*", ImVec2(80, 80)) && a != 0) { // button for multiplication
					next = true; multiply = true; decadd = false;
				} ImGui::SameLine();

			if (ImGui::Button("+", ImVec2(80, 80)) && a != 0) { // button for addition
					next = true; add = true; decadd = false;
				} ImGui::SameLine();

			if (ImGui::Button("-", ImVec2(80, 80)) && a != 0) { // for subtraction
					next = true; subtract = true; decadd = false;
			   } 
			//nextline
			if (ImGui::Button(".", ImVec2(80, 80)) && a != 0) {   // enable decimal input
				decadd = true;
			} ImGui::SameLine();		

			if (ImGui::Button("^x", ImVec2(80, 80)) && a != 0) { // for exponentiation
					next = true; power = true; decadd = false;
			}  ImGui::SameLine();

			if (ImGui::Button("^1/x", ImVec2(80, 80)) && a != 0) { // for root
					next = true; root = true; decadd = false;
			} ImGui::SameLine();

			if (ImGui::Button("1/x", ImVec2(80, 80)))
				next ? b = 1 / b
					 : a = 1 / a;

			ImGui::PopStyleColor();
		 ImGui::End();  // calculator window end   


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

	}//MAKE SURE RENDERING IS UNDER THE LOOP.
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
