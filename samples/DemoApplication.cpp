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

        return true;
    }

    bool DemoApplication::Resize(const uint32_t width, const uint32_t height)
    {
        Application::Resize(width, height);
        return true;
    }

    void DemoApplication::HandleInputEvent(const InputEvent& input_event)
    {
    }

    void DemoApplication::Update(float delta_time)
    {
        Application::Update(delta_time);
    }

}
