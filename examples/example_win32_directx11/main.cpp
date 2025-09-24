// Dear ImGui: standalone example application for DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <imgui_internal.h>
#include <d3d11.h>
#include <tchar.h>

//
#include "includes/globals.h"

#include "includes/TitleBar.h"
#include "includes/application.h"
#include "includes/interface.h"

#include <Uxtheme.h>
#include <dwmapi.h>

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

#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#pragma comment(lib, "setupapi.lib")


int InstalarLoopback() {
    // 1) Crear DeviceInfoList para la clase de red
    HDEVINFO hDevInfo = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_NET, nullptr);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    SP_DEVINFO_DATA devInfo = { sizeof(devInfo) };

    // 2) Crear un nuevo DeviceInfo
    if (!SetupDiCreateDeviceInfo(
        hDevInfo,
        TEXT("Microsoft KM-TEST Loopback Adapter"),
        &GUID_DEVCLASS_NET,
        nullptr,
        nullptr,
        DICD_GENERATE_ID,
        &devInfo)) {
        DWORD err = GetLastError();
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return err;
    }

    // 3) Asignar el Hardware ID "*MSLOOP\0\0"
    const wchar_t* hwId = L"*MSLOOP\0\0";
    if (!SetupDiSetDeviceRegistryProperty(
        hDevInfo,
        &devInfo,
        SPDRP_HARDWAREID,
        reinterpret_cast<const BYTE*>(hwId),
        (DWORD)((wcslen(hwId) + 2) * sizeof(wchar_t)))) {
        DWORD err = GetLastError();
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return err;
    }

    // 4) Registrar el dispositivo en PnP
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, hDevInfo, &devInfo)) {
        DWORD err = GetLastError();
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return err;
    }

    // --- PASOS NUEVOS PARA SELECCIONAR EL DRIVER ---

    // 5) Construir la lista de drivers compatibles
    if (!SetupDiBuildDriverInfoList(hDevInfo, &devInfo, SPDIT_COMPATDRIVER)) {
        DWORD err = GetLastError();
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return err;
    }

    // 6) Obtener el primer SP_DRVINFO_DATA (índice 0)
    SP_DRVINFO_DATA drvInfo = { sizeof(drvInfo) };
    if (!SetupDiEnumDriverInfo(
        hDevInfo,
        &devInfo,
        SPDIT_COMPATDRIVER,
        0,
        &drvInfo)) {
        DWORD err = GetLastError();
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return err;
    }

    // 7) Seleccionar ese driver
    if (!SetupDiSetSelectedDriver(hDevInfo, &devInfo, &drvInfo)) {
        DWORD err = GetLastError();
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return err;
    }

    // 8) Notificar al instalador la selección
    if (!SetupDiCallClassInstaller(DIF_SELECTDEVICE, hDevInfo, &devInfo)) {
        DWORD err = GetLastError();
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return err;
    }

    // --- FIN DE LA SELECCIÓN, AHORA SÍ INSTALAR ---

    // 9) Instalar el dispositivo con el driver seleccionado
    if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICE, hDevInfo, &devInfo)) {
        DWORD err = GetLastError();
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return err;
    }

    // 10) Limpiar
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return 0; // Éxito
}

// Main code
int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    // Debug console
    #ifdef _DEBUG
        showConsole = true;
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$", "r", stdin);
    #endif


        int res = InstalarLoopback();
        if (res == 0) {
            wprintf(L"Loopback instalado correctamente.\n");
        }
        else {
            wprintf(L"Error al instalar: %u\n", res);
        }


    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEXW wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
    ::RegisterClassEx(&wc);
    hwnd = ::CreateWindow(wc.lpszClassName, L"ArtNet to DMX Interface", WS_POPUP, 100, 100, WINDOWS_WIDTH, WINDOWS_HEIGHT, NULL, NULL, wc.hInstance, NULL);

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Call app load event
    Application::onLoad();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    Application::style();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Our state
    bool main_windows = true;
    bool show_demo_window = true;

    //titlebar_init(100, 100, io);

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

        // Demo
        #ifdef _DEBUG
            ImGui::ShowDemoWindow(&show_demo_window);
        #endif

        // Setup windows flags
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoSavedSettings;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        window_flags |= ImGuiWindowFlags_NoScrollbar;
        window_flags |= ImGuiWindowFlags_NoScrollWithMouse;

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        titlebar(hwnd, io, "ArtNet to DMX Interface", 30, viewport, true, false, true);
    
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGui::Begin("##main_windows", &main_windows, window_flags);
        {
            Application::renderUI();

            ImGui::End();
        }   

        // Rendering
        ImGui::Render();

        const float clear_rgba_zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_rgba_zero);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
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
