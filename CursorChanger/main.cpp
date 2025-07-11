// Dear ImGui: standalone example application for DirectX 12

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <string>
#include <tchar.h>

#include "CursorChanger.h"
#include "CursorSettingUI.h"
#include "Debugger.h"
#include "DynamicLibraryLoader.h"
#include "SettingManager.h"
#include "SystemSetting.h"
#include "SystemSettingUI.h"
#include "SystemTrayManager.h"

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

// Config for example app
static constexpr int APP_NUM_FRAMES_IN_FLIGHT = 2;
static constexpr int APP_NUM_BACK_BUFFERS = 2;
static constexpr int APP_SRV_HEAP_SIZE = 64;

struct FrameContext
{
    ID3D12CommandAllocator*     CommandAllocator;
    UINT64                      FenceValue;
};

// Simple free list based allocator
struct ExampleDescriptorHeapAllocator
{
    ID3D12DescriptorHeap*       Heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT                        HeapHandleIncrement;
    ImVector<int>               FreeIndices;

    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
    {
        IM_ASSERT(Heap == nullptr && FreeIndices.empty());
        Heap = heap;
        D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
        HeapType = desc.Type;
        HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
        HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
        HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
        FreeIndices.reserve(static_cast<int>(desc.NumDescriptors));
        for (int n = static_cast<int>(desc.NumDescriptors); n > 0; n--)
            FreeIndices.push_back(n - 1);
    }
    void Destroy()
    {
        Heap = nullptr;
        FreeIndices.clear();
    }
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
    {
        IM_ASSERT(FreeIndices.Size > 0);
        const int idx = FreeIndices.back();
        FreeIndices.pop_back();
        out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (static_cast<SIZE_T>(idx) * HeapHandleIncrement);
        out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (static_cast<SIZE_T>(idx) * HeapHandleIncrement);
    }
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
    {
        int cpu_idx = static_cast<int>((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
        int gpu_idx = static_cast<int>((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
        IM_ASSERT(cpu_idx == gpu_idx);
        FreeIndices.push_back(cpu_idx);
    }
};

// Data
static FrameContext                 g_frameContext[APP_NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;

static ID3D12Device*                g_pd3dDevice = nullptr;
static ID3D12DescriptorHeap*        g_pd3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = nullptr;
static ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;
static ID3D12CommandQueue*          g_pd3dCommandQueue = nullptr;
static ID3D12GraphicsCommandList*   g_pd3dCommandList = nullptr;
static ID3D12Fence*                 g_fence = nullptr;
static HANDLE                       g_fenceEvent = nullptr;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3*             g_pSwapChain = nullptr;
static bool                         g_SwapChainOccluded = false;
static HANDLE                       g_hSwapChainWaitableObject = nullptr;
static ID3D12Resource*              g_mainRenderTargetResource[APP_NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[APP_NUM_BACK_BUFFERS] = {};

// My static variables
// static HCURSOR g_defaultCursor = nullptr;
// static HCURSOR g_changedCursor = nullptr;
static const std::wstring g_version = L"v1.1.1";
static const std::wstring g_appName = L"CursorChanger";
static const std::wstring g_appVersion = g_appName + L" " + g_version;
static std::unique_ptr<CursorChanger> g_pCursorChanger;
static std::unique_ptr<SystemTrayManager> g_pTrayManager;
static std::unique_ptr<ProcessManager> g_pProcessManager;
static std::unique_ptr<SettingManager> g_pSettingManager;
static constexpr unsigned int MONITORING_TIME = 1;
static HWINEVENTHOOK g_hWinEventHook = nullptr;
static bool g_timerActive = false;

using RegOpenKeyExW_t = LSTATUS(WINAPI*)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
using RegQueryValueExW_t = LSTATUS(WINAPI*)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
using RegSetValueExW_t = LSTATUS(WINAPI*)(HKEY, LPCWSTR, DWORD, DWORD, const BYTE*, DWORD);
using RegDeleteValueW_t = LSTATUS(WINAPI*)(HKEY, LPCWSTR);
using RegCloseKey_t = LSTATUS(WINAPI*)(HKEY);
using GetModuleFileNameW_t = DWORD(WINAPI*)(HMODULE, LPWSTR, DWORD);
using GetModuleFileName_t = DWORD(WINAPI*)(HMODULE, LPSTR, DWORD);
using GetWindowThreadProcessId_t = DWORD(WINAPI*)(HWND, LPDWORD);
using OpenProcess_t = HANDLE(WINAPI*)(DWORD, BOOL, DWORD);
using QueryFullProcessImageNameA_t = BOOL(WINAPI*)(HANDLE, DWORD, LPSTR, LPDWORD);
using CloseHandle_t = BOOL(WINAPI*)(HANDLE);

static RegOpenKeyExW_t RegOpenKeyExWFunc = nullptr;
static RegQueryValueExW_t RegQueryValueExWFunc = nullptr;
static RegSetValueExW_t RegSetValueExWFunc = nullptr;
static RegCloseKey_t RegCloseKeyWFunc = nullptr;
static RegDeleteValueW_t RegDeleteValueExWFunc = nullptr;
static GetModuleFileNameW_t GetModuleFileNameWFunc = nullptr;
static GetModuleFileName_t GetModuleFileNameFunc = nullptr;
static GetWindowThreadProcessId_t GetWindowThreadProcessIdFunc = nullptr;
static OpenProcess_t OpenProcessFunc = nullptr;
static QueryFullProcessImageNameA_t QueryFullProcessImageNameAFunc = nullptr;
static CloseHandle_t CloseHandleFunc = nullptr;


// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void SetEnableProcessMonitoring(HWND hwnd, bool enable);
void MonitorProcessAndChangeCursor();
void CALLBACK CursorFocusEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
static bool RegisterAsStartupProgram(const std::wstring& appName, const std::wstring& appPath, bool enable);
static bool ValidateStartupRegistration(const std::wstring& appName);
void InitializeDynamicFunctions();

// Main code
#ifdef _DEBUG
int main(int, char**)
{
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#endif
    // Add handlers
    InitializeDynamicFunctions();
    SetConsoleCtrlHandler(CursorChanger::ConsoleCtrlHandler, TRUE);
    SetUnhandledExceptionFilter(CursorChanger::CursorUnhandledExceptionFilter);

    g_hWinEventHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, CursorFocusEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName,
        g_appVersion.c_str(), WS_OVERLAPPEDWINDOW,
        100, 100, screenWidth / 3, screenHeight / 2, nullptr, nullptr, wc.hInstance, nullptr);

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

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = g_pd3dDevice;
    init_info.CommandQueue = g_pd3dCommandQueue;
    init_info.NumFramesInFlight = APP_NUM_FRAMES_IN_FLIGHT;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
    // (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
    init_info.SrvDescriptorHeap = g_pd3dSrvDescHeap;
    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)            { return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
    ImGui_ImplDX12_Init(&init_info);

    // Before 1.91.6: our signature was using a single descriptor. From 1.92, specifying SrvDescriptorAllocFn/SrvDescriptorFreeFn will be required to benefit from new features.
    //ImGui_ImplDX12_Init(g_pd3dDevice, APP_NUM_FRAMES_IN_FLIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap, g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(), g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

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

    static ImFont* g_gulimFont = nullptr;
    g_gulimFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\gulim.ttc", 16.0f, nullptr, io.Fonts->GetGlyphRangesKorean());
    if (g_gulimFont == nullptr) {
        OutputDebugStringW(L"Failed to load Gulim font\n");
    }

    // Our state
    // bool show_demo_window = true;
    // bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool shouldOpen = true;

    // My variables
    g_pCursorChanger = std::make_unique<CursorChanger>();
    g_pSettingManager = std::make_unique<SettingManager>();
    g_pProcessManager = std::make_unique<ProcessManager>();
    Debugger debugger;

    // Get app path
    wchar_t appPath[MAX_PATH];
    GetModuleFileNameWFunc(nullptr, appPath, MAX_PATH);
    std::wstring appPathW = appPath;
    
    SystemSettingUI systemSettingUI(g_pSettingManager->pSystemSetting);
    CursorSettingUI cursorSettingUI(g_pSettingManager->pCursorSetting,
        [&]()
        {
            g_pCursorChanger->ChangeCursor(g_pSettingManager->pCursorSetting->cursorPath);
        },
        [&]()
        {
            CursorChanger::RestoreCursor();
        },
        [&]()
        {
            if (g_pSettingManager->pCursorSetting->isFocusOnly)
            {
                SetEnableProcessMonitoring(hwnd, false);
            }
            else
            {
                SetEnableProcessMonitoring(hwnd, true);
            }
        }
    );

    
    if (g_pSettingManager->pSystemSetting->shouldRegisterStartUp)
    {
        ValidateStartupRegistration(g_appName);
    }
    SetEnableProcessMonitoring(hwnd, !g_pSettingManager->pCursorSetting->isFocusOnly);

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

        // Handle window screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        float windowWidth = static_cast<float>(GetSystemMetrics( SM_CXSCREEN ));
        float windowHeight = static_cast<float>(GetSystemMetrics( SM_CYSCREEN ));
        ImVec2 windowSize = ImVec2( windowWidth, windowHeight );
        
        ImGui::SetNextWindowPos( ImVec2( 0, 0 ), ImGuiCond_Always );
        ImGui::SetNextWindowSize( windowSize, ImGuiCond_Always );
        if (ImGui::Begin("Cursor Tool", &shouldOpen, ImGuiWindowFlags_NoCollapse))
        {
            systemSettingUI.UpdateImGui();
            cursorSettingUI.UpdateImGui();
            if (ImGui::Button("Save Settings"))
            {
                if (g_pSettingManager->UpdateSettingsFile(g_pSettingManager->settingsPath))
                {
                    Debugger::Log("Settings updated successfully.");
                }
                else
                {
                    Debugger::Log("Failed to update settings.", Debugger::Type::Error);
                }

                RegisterAsStartupProgram(g_appName, appPathW, g_pSettingManager->pSystemSetting->shouldRegisterStartUp);
            }
            debugger.UpdateImGui();
        }
        ImGui::End();

        // Rendering
        ImGui::Render();

        FrameContext* frameCtx = WaitForNextFrameResources();
        UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
        frameCtx->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = g_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        // Render Dear ImGui graphics
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
        g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->Close();

        g_pd3dCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&g_pd3dCommandList));

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

        UINT64 fenceValue = g_fenceLastSignaledValue + 1;
        g_pd3dCommandQueue->Signal(g_fence, fenceValue);
        g_fenceLastSignaledValue = fenceValue;
        frameCtx->FenceValue = fenceValue;
    }

    WaitForLastSubmittedFrame();

    // Cleanup
    if (g_hWinEventHook)
    {
        UnhookWinEvent(g_hWinEventHook);
        g_hWinEventHook = nullptr;
    }
    CursorChanger::RestoreCursor();
    
    ImGui_ImplDX12_Shutdown();
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
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = APP_NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != nullptr)
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = APP_NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = APP_SRV_HEAP_SIZE;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
        g_pd3dSrvDescHeapAlloc.Create(g_pd3dDevice, g_pd3dSrvDescHeap);
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < APP_NUM_FRAMES_IN_FLIGHT; i++)
        if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
        g_pd3dCommandList->Close() != S_OK)
        return false;

    if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;

    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (g_fenceEvent == nullptr)
        return false;

    {
        IDXGIFactory4* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;
        if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
            return false;
        swapChain1->Release();
        dxgiFactory->Release();
        g_pSwapChain->SetMaximumFrameLatency(APP_NUM_BACK_BUFFERS);
        g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
    for (UINT i = 0; i < APP_NUM_FRAMES_IN_FLIGHT; i++)
        if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = nullptr; }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
    if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
    if (g_fence) { g_fence->Release(); g_fence = nullptr; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void CreateRenderTarget()
{
    for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
        g_mainRenderTargetResource[i] = pBackBuffer;
    }
}

void CleanupRenderTarget()
{
    WaitForLastSubmittedFrame();

    for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
        if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; }
}

void WaitForLastSubmittedFrame()
{
    FrameContext* frameCtx = &g_frameContext[g_frameIndex % APP_NUM_FRAMES_IN_FLIGHT];

    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue == 0)
        return; // No fence was signaled

    frameCtx->FenceValue = 0;
    if (g_fence->GetCompletedValue() >= fenceValue)
        return;

    g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
    WaitForSingleObject(g_fenceEvent, INFINITE);
}

FrameContext* WaitForNextFrameResources()
{
    UINT nextFrameIndex = g_frameIndex + 1;
    g_frameIndex = nextFrameIndex;

    HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, nullptr };
    DWORD numWaitableObjects = 1;

    FrameContext* frameCtx = &g_frameContext[nextFrameIndex % APP_NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue != 0) // means no fence was signaled
    {
        frameCtx->FenceValue = 0;
        g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
        waitableObjects[1] = g_fenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

    return frameCtx;
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
    if (g_pTrayManager && g_pTrayManager->ProcessMessage(msg, wParam, lParam))
        return 0;
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_TIMER:
        if (wParam == 1)
        {
            MonitorProcessAndChangeCursor();
        }
        return 0;
    case WM_CREATE:
        {
            HCURSOR hCursor = LoadCursor(nullptr, IDC_ARROW);
            HICON hIcon = CopyIcon(hCursor);
            g_pTrayManager = std::make_unique<SystemTrayManager>(hWnd, hIcon, "Cursor Changer");
            g_pTrayManager->SetOnDoubleClickCallback([hWnd]()
            {
                ShowWindow(hWnd, SW_RESTORE);
                g_pTrayManager->RemoveFromTray();
            });
            SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            return 0;
        }
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            WaitForLastSubmittedFrame();
            CleanupRenderTarget();
            HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
            assert(SUCCEEDED(result) && "Failed to resize swapchain.");
            CreateRenderTarget();
        }

        if (wParam == SIZE_MINIMIZED)
        {
            if (g_pSettingManager && g_pSettingManager->pSystemSetting && g_pSettingManager->pSystemSetting->isSystemTrayMode)
            {
                ShowWindow(hWnd, SW_HIDE);
                g_pTrayManager->AddToTray();
            }
        }
        return 0;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case 1:
                ShowWindow(hWnd, SW_RESTORE);
                g_pTrayManager->RemoveFromTray();
                break;
            case 2:
                DestroyWindow(hWnd);
                break;
            default: ;
            }
            return 0;
        }
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        
        if (g_pTrayManager)
        {
            g_pTrayManager->RemoveFromTray();
            g_pTrayManager.reset();
        }
        
        ::PostQuitMessage(0);
        return 0;
    default: ;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void CALLBACK CursorFocusEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (g_pSettingManager == nullptr || g_pCursorChanger == nullptr)
    {
        return;
    }

    if (g_pSettingManager->pCursorSetting->shouldChangeByProcess == false)
    {
        return;
    }

    if (g_pSettingManager->pCursorSetting->isFocusOnly == false)
    {
        return;
    }
    
    if (event == EVENT_SYSTEM_FOREGROUND)
    {
        DWORD processId;
        GetWindowThreadProcessIdFunc(hwnd, &processId);

        HANDLE hProcess = OpenProcessFunc(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (hProcess)
        {
            char processName[MAX_PATH] = "";
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameAFunc(hProcess, 0, processName, &size))
            {
                std::string processNameStr(processName);
                std::string targetProcessName = g_pSettingManager->pCursorSetting->targetProcess;

                if (processNameStr.find(targetProcessName) != std::string::npos)
                {
                    g_pCursorChanger->ChangeCursor(g_pSettingManager->pCursorSetting->cursorPath);
                }
                else
                {
                    CursorChanger::RestoreCursor();
                }
            }

            CloseHandleFunc(hProcess);
        }
    }
}

static void SetEnableProcessMonitoring(HWND hwnd, bool enable)
{
    if (enable)
    {
        UINT_PTR timerId = SetTimer(hwnd, 1, MONITORING_TIME * 1000, nullptr);
        if (timerId != 0)
        {
            g_timerActive = true;
            Debugger::Log("Process monitoring enabled.");
        }
        else
        {
            Debugger::Log("Failed to set timer for process monitoring.", Debugger::Type::Error);
        }
    }
    else
    {
        if (g_timerActive && IsWindow(hwnd))
        {
            if (KillTimer(hwnd, 1) == 0)
            {
                Debugger::Log("Failed to kill timer.");
            }
            else
            {
                g_timerActive = false;
                Debugger::Log("Timer killed successfully.");
            }
        }
    }
}

void MonitorProcessAndChangeCursor()
{
    if (g_pProcessManager == nullptr || g_pSettingManager == nullptr)
    {
        return;
    }

    if (g_pSettingManager->pCursorSetting->shouldChangeByProcess == false)
    {
        return;
    }

    static auto lastCheckTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastCheckTime);
    if (elapsedTime < std::chrono::seconds(MONITORING_TIME))
    {
        return;
    }

    lastCheckTime = currentTime;
    
    const std::string& targetProcessName = g_pSettingManager->pCursorSetting->targetProcess;
    if (targetProcessName.empty())
    {
        return;
    }
    
    g_pProcessManager->UpdateProcesses();
    if (g_pProcessManager->IsProcessRunning(targetProcessName) == false)
    {
        return;
    }

    if (CursorChanger::IsCursorChanged())
    {
        return;
    }

    g_pCursorChanger->ChangeCursor(g_pSettingManager->pCursorSetting->cursorPath);
}

static bool RegisterAsStartupProgram(const std::wstring& appName, const std::wstring& appPath, bool enable)
{
    HKEY hKey;
    const wchar_t* keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    
    LONG result = RegOpenKeyExWFunc(HKEY_CURRENT_USER, keyPath, 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
    {
        return false;
    }
    
    if (enable)
    {
        result = RegSetValueExWFunc(hKey, appName.c_str(), 0, REG_SZ, 
                              reinterpret_cast<const BYTE*>(appPath.c_str()), 
                              static_cast<DWORD>((appPath.length() + 1) * sizeof(wchar_t)));
    }
    else
    {
        result = RegDeleteValueExWFunc(hKey, appName.c_str());
    }
    
    RegCloseKeyWFunc(hKey);
    if (result != ERROR_SUCCESS)
    {
        Debugger::Log("Failed to register startup program.", Debugger::Type::Error);
    }
    else
    {
        Debugger::Log("Startup program registration " + std::string(enable ? "enabled" : "disabled") + ".");
    }
    
    return (result == ERROR_SUCCESS);
}

static bool ValidateStartupRegistration(const std::wstring& appName)
{
    HKEY hKey;
    const wchar_t* keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    
    LONG result = RegOpenKeyExWFunc(HKEY_CURRENT_USER, keyPath, 0, KEY_READ | KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
        return false;
    
    wchar_t currentPath[MAX_PATH];
    GetModuleFileNameWFunc(nullptr, currentPath, MAX_PATH);
    
    wchar_t registeredPath[MAX_PATH] = {0};
    DWORD dataSize = sizeof(registeredPath);
    DWORD dataType;
    result = RegQueryValueExWFunc(hKey, appName.c_str(), nullptr, &dataType, 
                            reinterpret_cast<BYTE*>(registeredPath), &dataSize);
    
    bool isValid = false;
    
    if (result == ERROR_SUCCESS && dataType == REG_SZ)
    {
        if (wcscmp(currentPath, registeredPath) != 0)
        {
            result = RegSetValueExWFunc(hKey, appName.c_str(), 0, REG_SZ,
                                  reinterpret_cast<const BYTE*>(currentPath),
                                  static_cast<DWORD>((wcslen(currentPath) + 1) * sizeof(wchar_t)));
            isValid = (result == ERROR_SUCCESS);
            Debugger::Log("Updated startup registry path");
        }
        else
        {
            isValid = true;
        }
    }
    
    RegCloseKeyWFunc(hKey);
    return isValid;
}

void InitializeDynamicFunctions()
{
    DynamicLibraryLoader::LoadLibraryFrom(L"user32.dll");
    DynamicLibraryLoader::LoadLibraryFrom(L"kernel32.dll");
    DynamicLibraryLoader::LoadLibraryFrom(L"advapi32.dll");

    RegOpenKeyExWFunc = DynamicLibraryLoader::GetFunctionOrNull<RegOpenKeyExW_t>(L"advapi32.dll", "RegOpenKeyExW");
    RegQueryValueExWFunc = DynamicLibraryLoader::GetFunctionOrNull<RegQueryValueExW_t>(L"advapi32.dll", "RegQueryValueExW");
    RegSetValueExWFunc = DynamicLibraryLoader::GetFunctionOrNull<RegSetValueExW_t>(L"advapi32.dll", "RegSetValueExW");
    RegDeleteValueExWFunc = DynamicLibraryLoader::GetFunctionOrNull<RegDeleteValueW_t>(L"advapi32.dll", "RegDeleteValueW");
    RegCloseKeyWFunc = DynamicLibraryLoader::GetFunctionOrNull<RegCloseKey_t>(L"advapi32.dll", "RegCloseKey");
    GetModuleFileNameWFunc = DynamicLibraryLoader::GetFunctionOrNull<GetModuleFileNameW_t>(L"kernel32.dll", "GetModuleFileNameW");
    GetModuleFileNameFunc = DynamicLibraryLoader::GetFunctionOrNull<GetModuleFileName_t>(L"kernel32.dll", "GetModuleFileNameA");
    GetWindowThreadProcessIdFunc = DynamicLibraryLoader::GetFunctionOrNull<GetWindowThreadProcessId_t>(L"user32.dll", "GetWindowThreadProcessId");
    OpenProcessFunc = DynamicLibraryLoader::GetFunctionOrNull<OpenProcess_t>(L"kernel32.dll", "OpenProcess");
    QueryFullProcessImageNameAFunc = DynamicLibraryLoader::GetFunctionOrNull<QueryFullProcessImageNameA_t>(L"kernel32.dll", "QueryFullProcessImageNameA");
    CloseHandleFunc = DynamicLibraryLoader::GetFunctionOrNull<CloseHandle_t>(L"kernel32.dll", "CloseHandle");
}