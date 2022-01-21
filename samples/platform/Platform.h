#pragma once

#include "platform/Application.h"
#include "platform/Window.h"
#include "core/Timer.h"

#if defined(VK_USE_PLATFORM_XLIB_KHR)
#    undef Success
#endif

namespace prm
{
    enum class ExitCode
    {
        Success = 0, /* App executed as expected */
        Help,        /* App should show help */
        Close,       /* App has been requested to close at initialization */
        FatalError   /* App encountered an unexpected error */
    };

    class Platform
    {
    public:
        Platform() = default;

        virtual ~Platform() = default;

        virtual ExitCode Initialize(Application* app);

        ExitCode MainLoop();

        /**
         * @brief Runs the application for one frame
         */
        void Update();

        /**
         * @brief Terminates the platform and the application
         * @param code Determines how the platform should exit
         */
        virtual void Terminate(ExitCode code);

        virtual void Close();

        /**
         * @brief Returns the working directory of the application set by the platform
         * @returns The path to the working directory
         */
        static const std::string& GetExternalStorageDirectory();

        /**
         * @brief Returns the suitable directory for temporary files from the environment variables set in the system
         * @returns The path to the temp folder on the system
         */
        static const std::string& GetTempDirectory();

        /**
         * @return The VkInstance extension name for the platform
         */
        virtual const char* GetSurfaceExtension() = 0;

        virtual void Resize(uint32_t width, uint32_t height);

        virtual void HandleInputEvent(const InputEvent& input_event);

        Window& GetWindow() const;

        Application& GetApp() const;

        Application& GetApp();

        static void SetExternalStorageDirectory(const std::string& dir);

        static void SetTempDirectory(const std::string& dir);

        void SetFocus(bool focused);

        bool AppRequested() const;

        bool StartApp();

        void ForceSimulationFps(float fps);

        void DisableInputProcessing();

        void SetWindowProperties(const Window::OptionalProperties& properties);

        static const uint32_t MIN_WINDOW_WIDTH;
        static const uint32_t MIN_WINDOW_HEIGHT;

    protected:
        std::unique_ptr<Window> m_Window{ nullptr };

        std::unique_ptr<Application> m_ActiveApp{ nullptr };

        /**
         * @brief Handles the creation of the window
         *
         * @param properties Preferred window configuration
         */
        virtual void ICreateWindow(const Window::Properties& properties) = 0;

        void OnUpdate(float delta_time);
        void OnAppError(const std::string& app_id);
        void OnAppStart();
        void OnAppClose();
        void OnPlatformClose();

        Window::Properties m_WindowProperties;              /* Source of truth for window state */
        bool               m_FixedSimulationFps{ false };    /* Delta time should be fixed with a fabricated value */
        float              m_SimulationFrameTime = 0.016f; /* A fabricated delta time */
        bool               m_ProcessInputEvents{ true };     /* App should continue processing input events */
        bool               m_Focused;                        /* App is currently in focus at an operating system level */
        bool               m_CloseRequested{ false };         /* Close requested */

    private:
        Timer m_Timer;

        static std::string m_ExternalStorageDirectory;

        static std::string m_TempDirectory;
    };

}
