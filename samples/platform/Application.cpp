#include "pch.h"
#include "platform/Application.h"

#include "platform/Platform.h"
#include "platform/InputEvents.h"

namespace prm
{
    Application::Application() :
        m_Name{ "Sample Name" }
    {
    }

    bool Application::Prepare(Platform& _platform)
    {
        this->m_Platform = &_platform;

        return true;
    }

    void Application::Finish()
    {
    }

    bool Application::Resize(const uint32_t /*width*/, const uint32_t /*height*/)
    {
        return true;
    }

    void Application::HandleInputEvent(const InputEvent& input_event)
    {
    }

    void Application::Update(float delta_time)
    {
        m_Fps = 1.0f / delta_time;
        m_FrameTime = delta_time * 1000.0f;
    }

    const std::string& Application::GetName() const
    {
        return m_Name;
    }

    void Application::SetName(const std::string& name_)
    {
        m_Name = name_;
    }

}
