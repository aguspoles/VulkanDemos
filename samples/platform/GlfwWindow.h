#pragma once
#include "platform/Window.h"

struct GLFWwindow;

namespace prm
{
    class Platform;
    class VulkanInstance;

    /**
     * @brief An implementation of GLFW, inheriting the behaviour of the Window interface
     */
    class GlfwWindow : public Window
    {
    public:
        GlfwWindow(Platform* platform, const Window::Properties& properties);

        virtual ~GlfwWindow();

        virtual vk::SurfaceKHR CreateSurface(vk::Instance instance, vk::PhysicalDevice physical_device) override;

        virtual bool ShouldClose() override;

        virtual void ProcessEvents() override;

        virtual void Close() override;

        float GetDpiFactor() const override;

        float GetContentScaleFactor() const override;

    private:
        GLFWwindow* m_Handle = nullptr;
    };
}
