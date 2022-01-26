#pragma once

namespace prm
{
    class Window
    {
    public:
        struct Extent
        {
            uint32_t width;
            uint32_t height;
        };

        struct OptionalExtent
        {
            std::optional<uint32_t> width;
            std::optional<uint32_t> height;
        };

        enum class Mode
        {
            Headless,
            Fullscreen,
            FullscreenBorderless,
            Default
        };

        enum class Vsync
        {
            OFF,
            ON,
            Default
        };

        struct OptionalProperties
        {
            std::optional<std::string>  title;
            std::optional<Mode>         mode;
            std::optional<bool>         resizable;
            std::optional<Vsync>        vsync;
            OptionalExtent              extent;
        };

        struct Properties
        {
            std::string title;
            Mode        mode = Mode::Default;
            bool        resizable = true;
            Vsync       vsync = Vsync::Default;
            Extent      extent = { 1280, 720 };
        };

        Window(const Properties& properties);

        virtual ~Window() = default;

        virtual vk::SurfaceKHR CreateSurface(vk::Instance instance) = 0;

        virtual bool ShouldClose() = 0;

        virtual void ProcessEvents();

        virtual void Close() = 0;

        virtual float GetDpiFactor() const = 0;

        virtual float GetContentScaleFactor() const;

        Extent Resize(const Extent& extent);

        const Extent& GetExtent() const;

        Mode GetWindowMode() const;

        virtual const char** GetInstanceExtensions(uint32_t& count) const = 0;

        virtual void WaitEvents() {}

    protected:
        Properties m_Properties;
    };
} 
