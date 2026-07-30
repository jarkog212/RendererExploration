// RedererExploration.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>

constexpr wchar_t CLASS_NAME[] = L"BaseWindowClass";
constexpr wchar_t WINDOW_TITLE[] = L"Base Window";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/,
    PWSTR /*pCmdLine*/,
    int nCmdShow)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(nullptr, L"Window class registration failed.", L"Error", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        0,                                  
        CLASS_NAME,                         
        WINDOW_TITLE,                       
        WS_OVERLAPPEDWINDOW,                
        CW_USEDEFAULT, CW_USEDEFAULT,       
        800, 600,                           
        nullptr,                            
        nullptr,                            
        hInstance,                          
        nullptr                             
    );

    if (!hwnd)
    {
        MessageBoxW(nullptr, L"Window creation failed.", L"Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   //switch (uMsg)
   //{
   //case WM_DESTROY:
   //    PostQuitMessage(0);
   //    return 0;
   //
   //case WM_CLOSE:
   //    DestroyWindow(hwnd);
   //    return 0;
   //
   //case WM_PAINT:
   //{
   //    PAINTSTRUCT ps;
   //    HDC hdc = BeginPaint(hwnd, &ps);
   //    FillRect(hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
   //    EndPaint(hwnd, &ps);
   //    return 0;
   //}
   //
   //case WM_SIZE:
   //    // handle resize here if needed (width = LOWORD(lParam), height = HIWORD(lParam))
   //    return 0;
   //
   //case WM_KEYDOWN:
   //    if (wParam == VK_ESCAPE)
   //        DestroyWindow(hwnd);
   //    return 0;
   //
   //    default:
   //        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
   //}

   return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
