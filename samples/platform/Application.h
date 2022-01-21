#pragma once

namespace prm
{
    class Window;
    class Platform;
    class InputEvent;

    class Application
    {
    public:
        Application();

        virtual ~Application() = default;

        /**
         * @brief Prepares the application for execution
         * @param platform The platform the application is being run on
         */
        virtual bool Prepare(Platform& platform);

        /**
         * @brief Updates the application
         * @param delta_time The time since the last update
         */
        virtual void Update(float delta_time);

        /**
         * @brief Handles cleaning up the application
         */
        virtual void Finish();

        /**
         * @brief Handles resizing of the window
         * @param width New width of the window
         * @param height New height of the window
         */
        virtual bool Resize(const uint32_t width, const uint32_t height);

        /**
         * @brief Handles input events of the window
         * @param input_event The input event object
         */
        virtual void HandleInputEvent(const InputEvent& input_event);

        const std::string& GetName() const;

        void SetName(const std::string& name);

    protected:
        float m_Fps{ 0.0f };

        float m_FrameTime{ 0.0f };   // In ms

        uint32_t m_FrameCount{ 0 };

        uint32_t m_LastFrameCount{ 0 };

        Platform* m_Platform;

    private:
        std::string m_Name{};
    };
}
