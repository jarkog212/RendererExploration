// RedererExploration.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include "Utils.h"
#include "DX12/DX12Renderer.h"

namespace
{
    static constexpr wchar_t k_winTitle[] = L"Base Window";
    static WNDCLASSEXW g_winClass;
    static HWND g_mainWindow = NULL;
    static bool g_running = true;

    void Close(HWND hwnd)
    {
        if (::g_mainWindow == hwnd)
        {
            DX12::Cleanup();
            ::g_running = false;
        }

        DestroyWindow(hwnd);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/,
    PWSTR /*pCmdLine*/,
    int nCmdShow)
{
    ::g_winClass.cbSize = sizeof(WNDCLASSEXW);
    ::g_winClass.style = CS_HREDRAW | CS_VREDRAW;
    ::g_winClass.lpfnWndProc = WindowProc;
    ::g_winClass.hInstance = hInstance;
    ::g_winClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    ::g_winClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    ::g_winClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    ::g_winClass.lpszClassName = L"BaseWindowClass";

    if (!RegisterClassExW(&::g_winClass))
    {
        DISPLAY_ERROR(L"Window class registration failed.")
        return 0;
    }

    ::g_mainWindow = Utils::CreateWindowsWindow(::k_winTitle, 1920, 1080, &::g_winClass);
    if (!::g_mainWindow)
    {
        DISPLAY_ERROR(L"Window creation failed.")
        return 0;
    }

    ShowWindow(::g_mainWindow, nCmdShow);
    UpdateWindow(::g_mainWindow);
    DX12::Initialize(::g_mainWindow);

    while (::g_running)
    {
        MSG msg = {};
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        DX12::Render();
    }

    return static_cast<int>(0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
     case WM_DESTROY:
         PostQuitMessage(0);
         return 0;
     
     case WM_CLOSE:
     case WM_QUIT:
         Close(hwnd);
         return 0;
     
     //case WM_PAINT:
     //{
     //    PAINTSTRUCT ps;
     //    HDC hdc = BeginPaint(hwnd, &ps);
     //    FillRect(hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
     //    EndPaint(hwnd, &ps);
     //    return 0;
     //}
     
     //case WM_SIZE:
     //    // handle resize here if needed (width = LOWORD(lParam), height = HIWORD(lParam))
     //    return 0;
     
     case WM_KEYDOWN:
         if (wParam == VK_ESCAPE)
             Close(hwnd);

         return 0;
     
     default:
         return DefWindowProcW(hwnd, uMsg, wParam, lParam);
   }
}

HWND UtilsImpl::CreateWindowsWindow(
    const wchar_t* title,
    const unsigned int width,
    const unsigned int height,
    WNDCLASSEXW* wc)
{
    if (!wc)
        wc = &::g_winClass;

    return CreateWindowExW(
        0,
        wc->lpszClassName,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr,
        nullptr,
        wc->hInstance,
        nullptr
    );
}
