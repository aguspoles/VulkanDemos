#include "pch.h"

#include "core/Logger.h"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include "platform/WindowsPlatform.h"
#include "DemoApplication.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
{

    prm::WindowsPlatform platform{ hInstance, hPrevInstance,
                                  lpCmdLine, nCmdShow };

    prm::Log::Init(); //Initialize after creating the platform, so the output for logging is set

    auto* app = new prm::DemoApplication();

    auto code = platform.Initialize(app);

    if (code == prm::ExitCode::Success)
    {
        code = platform.MainLoop();
    }

    platform.Terminate(code);

    return EXIT_SUCCESS;
}

#endif