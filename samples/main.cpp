#include "pch.h"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include "platform/WindowsPlatform.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
{
    prm::WindowsPlatform plat(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    return EXIT_SUCCESS;
}

#endif