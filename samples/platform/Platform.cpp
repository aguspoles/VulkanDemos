#include "pch.h"
#include "platform/Platform.h"
#include "platform/InputEvents.h"
#include "core/Logger.h"

namespace prm
{
    const uint32_t Platform::MIN_WINDOW_WIDTH = 420;
    const uint32_t Platform::MIN_WINDOW_HEIGHT = 320;

    std::string Platform::m_ExternalStorageDirectory;

    std::string Platform::m_TempDirectory;

    ExitCode Platform::Initialize(Application* app)
    {
        m_ActiveApp = std::unique_ptr<Application>(app);

        ICreateWindow(m_WindowProperties);

        if (!m_Window)
        {
            LOGE("Window creation failed!");
            return ExitCode::FatalError;
        }

        return ExitCode::Success;
    }

    ExitCode Platform::MainLoop()
    {
        if (!AppRequested())
        {
            LOGI("An app was not requested, can not continue");
            return ExitCode::Close;
        }

        if (!StartApp())
        {
            LOGE("Failed to load requested application");
            return ExitCode::FatalError;
        }

        while (!m_Window->ShouldClose() && !m_CloseRequested)
        {
            try
            {
                Update();

                m_Window->ProcessEvents();
            }
            catch (std::exception& e)
            {
                LOGE("Error Message: {}", e.what());
                LOGE("Failed when running application {}", m_ActiveApp->GetName());

                OnAppError(m_ActiveApp->GetName());

                return ExitCode::FatalError;
            }
        }

        return ExitCode::Success;
    }

    void Platform::Update()
    {
        auto delta_time = static_cast<float>(m_Timer.Tick<Timer::Seconds>());

        if (m_Focused)
        {
            OnUpdate(delta_time);

            if (m_FixedSimulationFps)
            {
                delta_time = m_SimulationFrameTime;
            }

            m_ActiveApp->Update(delta_time);
        }
    }

    void Platform::Terminate(ExitCode code)
    {
        if (m_ActiveApp)
        {
            const std::string& id = m_ActiveApp->GetName();

            OnAppClose();

            m_ActiveApp->Finish();
        }

        m_ActiveApp.reset();
        m_Window.reset();

        OnPlatformClose();

        // Halt on all unsuccessful exit codes unless ForceClose is in use
        if (code != ExitCode::Success)
        {
#ifndef ANDROID
            std::cout << "Press any key to continue";
            std::cin.get();
#endif
        }
    }

    void Platform::Close()
    {
        if (m_Window)
        {
            m_Window->Close();
        }

        // Fallback incase a window is not yet in use
        m_CloseRequested = true;

        OnAppClose();
    }

    void Platform::ForceSimulationFps(float fps)
    {
        m_FixedSimulationFps = true;
        m_SimulationFrameTime = 1 / fps;
    }

    void Platform::DisableInputProcessing()
    {
        m_ProcessInputEvents = false;
    }

    void Platform::SetFocus(bool _focused)
    {
        m_Focused = _focused;
    }

    void Platform::SetWindowProperties(const Window::OptionalProperties& properties)
    {
        m_WindowProperties.title = properties.title.has_value() ? properties.title.value() : m_WindowProperties.title;
        m_WindowProperties.mode = properties.mode.has_value() ? properties.mode.value() : m_WindowProperties.mode;
        m_WindowProperties.resizable = properties.resizable.has_value() ? properties.resizable.value() : m_WindowProperties.resizable;
        m_WindowProperties.vsync = properties.vsync.has_value() ? properties.vsync.value() : m_WindowProperties.vsync;
        m_WindowProperties.extent.width = properties.extent.width.has_value() ? properties.extent.width.value() : m_WindowProperties.extent.width;
        m_WindowProperties.extent.height = properties.extent.height.has_value() ? properties.extent.height.value() : m_WindowProperties.extent.height;
    }

    const std::string& Platform::GetExternalStorageDirectory()
    {
        return m_ExternalStorageDirectory;
    }

    const std::string& Platform::GetTempDirectory()
    {
        return m_TempDirectory;
    }

    Application& Platform::GetApp()
    {
        assert(m_ActiveApp && "Application is not valid");
        return *m_ActiveApp;
    }

    Application& Platform::GetApp() const
    {
        assert(m_ActiveApp && "Application is not valid");
        return *m_ActiveApp;
    }

    Window& Platform::GetWindow() const
    {
        return *m_Window;
    }

    void Platform::SetExternalStorageDirectory(const std::string& dir)
    {
        m_ExternalStorageDirectory = dir;
    }

    void Platform::SetTempDirectory(const std::string& dir)
    {
        m_TempDirectory = dir;
    }

    bool Platform::AppRequested() const
    {
        return m_ActiveApp != nullptr;
    }

    bool Platform::StartApp()
    {
        if (!m_ActiveApp->Prepare(*this))
        {
            LOGE("Failed to prepare app.");
            return false;
        }

        OnAppStart();

        return true;
    }

    void Platform::HandleInputEvent(const InputEvent& input_event)
    {
        if (m_ProcessInputEvents && m_ActiveApp)
        {
            m_ActiveApp->HandleInputEvent(input_event);
        }

        if (input_event.GetSource() == EventSource::Keyboard)
        {
            const auto& key_event = static_cast<const KeyInputEvent&>(input_event);

            if (key_event.GetCode() == KeyCode::Back ||
                key_event.GetCode() == KeyCode::Escape)
            {
                Close();
            }
        }
    }

    void Platform::Resize(uint32_t width, uint32_t height)
    {
        const auto extent = Window::Extent{ std::max<uint32_t>(width, MIN_WINDOW_WIDTH),
            std::max<uint32_t>(height, MIN_WINDOW_HEIGHT) };
        if (m_Window)
        {
            const auto actual_extent = m_Window->Resize(extent);

            if (m_ActiveApp)
            {
                m_ActiveApp->Resize(actual_extent.width, actual_extent.height);
            }
        }
    }

    void Platform::OnAppError(const std::string& app_id)
    {

    }

    void Platform::OnUpdate(float delta_time)
    {

    }

    void Platform::OnAppStart()
    {

    }

    void Platform::OnAppClose()
    {

    }

    void Platform::OnPlatformClose()
    {

    }


}

