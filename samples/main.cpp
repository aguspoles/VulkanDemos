#include "pch.h"

#include "core/Logger.h"
#include "DemoApplication.h"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include "platform/WindowsPlatform.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
{

    prm::WindowsPlatform platform{ hInstance, hPrevInstance,
                                  lpCmdLine, nCmdShow };

    prm::Log::Init(); //Initialize after creating the platform, so the output for logging is set

    auto* app = new prm::DemoApplication();

    prm::ExitCode code = platform.Initialize(app);

    if (code == prm::ExitCode::Success)
    {
        code = platform.MainLoop();
    }

    platform.Terminate(code);

    return EXIT_SUCCESS;
}

#endif