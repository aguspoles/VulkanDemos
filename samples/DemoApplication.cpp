#include "pch.h"
#include "DemoApplication.h"

#include "platform/Platform.h"
#include "platform/InputEvents.h"

namespace prm
{
    DemoApplication::DemoApplication() 
    {
        SetName("Demo");
    }

    DemoApplication::~DemoApplication()
    {
    }

    bool DemoApplication::Prepare(Platform& _platform)
    {
        Application::Prepare(_platform);

        m_Renderer.Init(_platform.GetWindow());

        return true;
    }

    bool DemoApplication::Resize(const uint32_t width, const uint32_t height)
    {
        Application::Resize(width, height);
        m_Renderer.RecreateSwapchain(Window::Extent{ width, height });
        return true;
    }

    void DemoApplication::HandleInputEvent(const InputEvent& input_event)
    {
    }

    void DemoApplication::Update(float delta_time)
    {
        m_Renderer.Draw(m_Platform->GetWindow().GetExtent());
        Application::Update(delta_time);
    }

}
