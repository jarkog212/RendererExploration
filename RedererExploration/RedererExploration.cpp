// RedererExploration.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <chrono>
#include <string>
#include <fstream>

#include "Utils.h"
#include "DX12/DX12Core.h"

namespace
{
    static constexpr wchar_t k_winTitle[] = L"Base Window";
    static WNDCLASSEXW g_winClass;
    static HWND g_mainWindow = NULL;
    static bool g_running = true;
    static float g_fps = 0.f;

    void Close(HWND hwnd)
    {
        if (::g_mainWindow == hwnd)
        {
            DX12::ReleaseAll();
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

    FALLBACK_ASSERT(
        RegisterClassExW(&::g_winClass),
        return 0,
        "Window class registration failed.");

    ::g_mainWindow = Utils::System::CreateWindowsWindow(::k_winTitle, 1920, 1080, &::g_winClass);
    FALLBACK_ASSERT(::g_mainWindow, return 0, "Window creation failed.");

    ShowWindow(::g_mainWindow, nCmdShow);
    UpdateWindow(::g_mainWindow);
    DX12::Initialize(::g_mainWindow);

    while (::g_running)
    {
        const std::chrono::time_point preLoopTimePoint = std::chrono::high_resolution_clock::now();
        MSG msg = {};
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (!::g_running)
                break;
        }

        DX12::Render();

        // conversion to float also changes units to seconds
        const std::chrono::duration<float> frameTime = std::chrono::high_resolution_clock::now() - preLoopTimePoint;
        ::g_fps = 1.f / frameTime.count();

        DISPLAY_RAW_OUTPUT("Current recorded FPS: %f", Utils::GetFPS());
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

HWND UtilsImpl::System::CreateWindowsWindow(
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

bool UtilsImpl::System::FileExists(std::filesystem::path& path)
{
    CRITICAL_ASSERT(!path.empty(), "Cannot determine if the file exists for an invalid path.");
    return std::filesystem::exists(path);
}

unsigned long long int UtilsImpl::System::FileSizeOf(std::filesystem::path& path)
{
    std::error_code ec;
    unsigned long long int fileSize = std::filesystem::file_size(path, ec);
    CRITICAL_ASSERT(!ec, "Error trying to get the filesize: [%u] %s", ec.value(), ec.message().data());

    return fileSize;
}

void UtilsImpl::System::LoadFile(std::filesystem::path& path, char*& buffer, unsigned long long int& size)
{
    CRITICAL_ASSERT(!path.empty(), "Trying to load a file with empty path.");

    std::ifstream file(path, std::ios::binary);
    CRITICAL_ASSERT(file, "File cound not be opened: %s", path.string().c_str());
    
    if (buffer)
    {
        if (size == 0)
        {
            DISPLAY_ERROR("Loading a file where buffer weas provided but size is unknown. %s", path.string().c_str());
            assert(false);
        }

        CRITICAL_ASSERT(file.read(buffer, size), "File cound not be read: %s", path.string().c_str());
        return;
    }
        
    size = UtilsImpl::System::FileSizeOf(path);
    buffer = new char[size];

    CRITICAL_ASSERT(file.read(buffer, size), "File cound not be read: %s", path.string().c_str());
}

void UtilsImpl::System::SaveFile(std::filesystem::path& path, const char* buffer, const unsigned long long int size)
{
    CRITICAL_ASSERT(!path.empty(), "Trying to save a file with empty path.");
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::out | std::ios::binary);

    FALLBACK_ASSERT(
        file && Utils::System::FileExists(path),
        file.close(); assert(false),
        "Could not save file: %s",
        path.string().c_str());

    file.write(buffer, size);
    file.close();
}

float UtilsImpl::GetFps() 
{
    return ::g_fps;
}
