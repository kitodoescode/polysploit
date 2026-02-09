#pragma once
#include "misc/globals.h"
#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class renderer_t {
public:
    using present_fn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
    using resize_buffers_fn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    renderer_t();
    ~renderer_t();

    ID3D11Device* device;
    ID3D11DeviceContext* device_context;
    IDXGISwapChain* swap_chain;
    ID3D11RenderTargetView* render_target_view;

    present_fn original_present;
    resize_buffers_fn original_resize_buffers;
    WNDPROC original_wnd_proc;

    HWND game_window;
    bool is_init;
    bool draw;
    float dpi_scale;
    TextEditor editor;

    bool get_swapchain_vtable();
    bool init_hooks();
    bool init_imgui(IDXGISwapChain* swapchain);

    void create_render_target();
    void cleanup_render_target();

    static HRESULT WINAPI present_h(IDXGISwapChain* swapchain, UINT sync, UINT flags);
    static HRESULT WINAPI resize_buffers_h(IDXGISwapChain* swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags);
    static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);

    void clean_up();
    void render();
};

inline renderer_t renderer;