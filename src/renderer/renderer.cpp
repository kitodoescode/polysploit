#include "renderer.h"
#include "scheduler/scheduler.h"

#include <minhook.h>
#pragma comment(lib, "minhook.x64.lib")

renderer_t::renderer_t() {
    device = nullptr;
    device_context = nullptr;
    swap_chain = nullptr;
    render_target_view = nullptr;
    original_wnd_proc = nullptr;
    game_window = nullptr;
    is_init = false;
    draw = false;
    dpi_scale = 1.f;
    original_present = nullptr;
    original_resize_buffers = nullptr;
}

renderer_t::~renderer_t() {
    clean_up();
}

bool renderer_t::get_swapchain_vtable() {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "polysploit_dummy", NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow("polysploit_dummy", "", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) {
        UnregisterClass("polysploit_dummy", GetModuleHandle(NULL));
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* tmp_device = nullptr;
    ID3D11DeviceContext* tmp_context = nullptr;
    IDXGISwapChain* tmp_swapchain = nullptr;
    D3D_FEATURE_LEVEL feature_level;
    const D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, levels, 2, D3D11_SDK_VERSION, &sd, &tmp_swapchain, &tmp_device, &feature_level, &tmp_context);

    if (FAILED(hr) || !tmp_swapchain) {
        if (tmp_swapchain) tmp_swapchain->Release();
        if (tmp_context) tmp_context->Release();
        if (tmp_device) tmp_device->Release();
        DestroyWindow(hwnd);
        UnregisterClass("polysploit_dummy", GetModuleHandle(NULL));
        return false;
    }

    void** vtable = *reinterpret_cast<void***>(tmp_swapchain);
    original_present = reinterpret_cast<present_fn>(vtable[8]);
    original_resize_buffers = reinterpret_cast<resize_buffers_fn>(vtable[13]);

    tmp_swapchain->Release();
    tmp_context->Release();
    tmp_device->Release();
    DestroyWindow(hwnd);
    UnregisterClass("polysploit_dummy", GetModuleHandle(NULL));
    return true;
}

bool renderer_t::init_hooks() {
    if (!get_swapchain_vtable())
        return false;

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
        return false;

    if (MH_CreateHook(reinterpret_cast<LPVOID>(original_present), &present_h, reinterpret_cast<LPVOID*>(&original_present)) != MH_OK)
        return false;

    if (MH_CreateHook(reinterpret_cast<LPVOID>(original_resize_buffers), &resize_buffers_h, reinterpret_cast<LPVOID*>(&original_resize_buffers)) != MH_OK)
        return false;

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
        return false;

    return true;
}

bool renderer_t::init_imgui(IDXGISwapChain* swapchain) {
    if (!swapchain) return false;

    swap_chain = swapchain;
    swap_chain->AddRef();

    if (FAILED(swapchain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device))))
        return false;

    device->GetImmediateContext(&device_context);

    DXGI_SWAP_CHAIN_DESC desc;
    if (FAILED(swapchain->GetDesc(&desc)))
        return false;

    game_window = desc.OutputWindow;
    if (!game_window)
        return false;

    create_render_target();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    ImGui_ImplWin32_Init(game_window);
    ImGui_ImplDX11_Init(device, device_context);

    editor.SetLanguageDefinition(editor.Lua());
    editor.SetText("print('helo sire :]')");

    original_wnd_proc = (WNDPROC)SetWindowLongPtr(game_window, GWLP_WNDPROC, (LONG_PTR)wnd_proc);

    return true;
}

void renderer_t::create_render_target() {
    if (!swap_chain || !device) return;

    ID3D11Texture2D* back_buffer = nullptr;
    if (SUCCEEDED(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer)))) {
        device->CreateRenderTargetView(back_buffer, NULL, &render_target_view);
        back_buffer->Release();
    }
}

void renderer_t::cleanup_render_target() {
    if (render_target_view) {
        render_target_view->Release();
        render_target_view = nullptr;
    }
}

HRESULT WINAPI renderer_t::present_h(IDXGISwapChain* swapchain, UINT sync, UINT flags) {
    if (!renderer.is_init) {
        if (renderer.init_imgui(swapchain))
            renderer.is_init = true;
    }

    if (renderer.is_init && renderer.draw) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        using namespace ImGui;

        SetNextWindowSize(ImVec2(550, 350));
        if (Begin("polysploit"), nullptr, ImGuiWindowFlags_NoResize) {
            if (BeginTabBar("##tabbar")) {
                if (BeginTabItem("editor")) {
                    renderer.editor.Render("TextEditor", ImVec2(0, GetContentRegionAvail().y - 24.f), true);
                    SetCursorPosY(GetCursorPosY() + 4);
                    if (Button("execute", ImVec2(0, 24))) scheduler.queue_script(renderer.editor.GetText().c_str());
                    SameLine();
                    if (Button("clear", ImVec2(0, 24))) renderer.editor.SetText("");
                    SameLine();
                    if (Button("execute clipboard", ImVec2(0, 24))) scheduler.queue_script(GetClipboardText());
                    EndTabItem();
                }

                if (BeginTabItem("scripts")) {
                    static int curr_script = 0;
                    const char* labels[] = { "infinite yield", "unc", "sunc", "vuln test" };
                    const char* sources[] = {
                        "loadstring(game:HttpGet(\"https://raw.githubusercontent.com/EdgeIY/infiniteyield/master/source\"))()",
                        "loadstring(game:HttpGet(\"https://raw.githubusercontent.com/unified-naming-convention/NamingStandard/refs/heads/main/UNCCheckEnv.lua\"))()",
                        R"(
getgenv().sUNCDebug = {
    ["printcheckpoints"] = true,
    ["delaybetweentests"] = 0,
    ["printtesttimetaken"] = true,
}
loadstring(game:HttpGet("https://script.sunc.su/"))()
                        )",
                        "loadstring(game:HttpGet(\"https://raw.githubusercontent.com/zryr/Vulnerability-Check/refs/heads/main/Script\"))()"
                    };

                    Combo("script", &curr_script, labels, 4);
                    if (Button("copy")) SetClipboardText(sources[curr_script]);
                    if (Button("execute")) scheduler.queue_script(sources[curr_script]);
                    EndTabItem();
                }

                if (BeginTabItem("options")) {
                    if (Button("unload")) exit(0);
                    EndTabItem();
                }
                EndTabBar();
            }
        }
        End();

        ImGui::Render();
        renderer.device_context->OMSetRenderTargets(1, &renderer.render_target_view, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return renderer.original_present(swapchain, sync, flags);
}

HRESULT WINAPI renderer_t::resize_buffers_h(IDXGISwapChain* swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags) {
    renderer.cleanup_render_target();
    HRESULT hr = renderer.original_resize_buffers(swapchain, count, width, height, format, flags);
    renderer.create_render_target();
    return hr;
}

LRESULT CALLBACK renderer_t::wnd_proc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_KEYDOWN && (wparam == VK_HOME || wparam == VK_RSHIFT || wparam == VK_INSERT))
        renderer.draw = !renderer.draw;

    if (renderer.draw && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wparam, lparam))
        return true;

    switch (msg) {
    case 522: case 513: case 533: case 514: case 134: case 516: case 517: case 258:
    case 257: case 132: case 127: case 255: case 523: case 524: case 793:
        if (renderer.draw) return true;
    }

    return CallWindowProc(renderer.original_wnd_proc, hWnd, msg, wparam, lparam);
}

void renderer_t::clean_up() {
    if (original_wnd_proc && game_window)
        SetWindowLongPtr(game_window, GWLP_WNDPROC, (LONG_PTR)original_wnd_proc);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (render_target_view) { render_target_view->Release(); render_target_view = nullptr; }
    if (swap_chain) { swap_chain->Release(); swap_chain = nullptr; }
    if (device_context) { device_context->Release(); device_context = nullptr; }
    if (device) { device->Release(); device = nullptr; }
}

void renderer_t::render() {
    init_hooks();
}