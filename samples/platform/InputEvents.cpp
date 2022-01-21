#include "pch.h"
#include "platform/InputEvents.h"

namespace prm
{
    InputEvent::InputEvent(EventSource source) :
        m_Source{ source }
    {
    }

    EventSource InputEvent::GetSource() const
    {
        return m_Source;
    }

    KeyInputEvent::KeyInputEvent(KeyCode code, KeyAction action) :
        InputEvent{ EventSource::Keyboard },
        m_Code{ code },
        m_Action{ action }
    {
    }

    KeyCode KeyInputEvent::GetCode() const
    {
        return m_Code;
    }

    KeyAction KeyInputEvent::GetAction() const
    {
        return m_Action;
    }

    MouseButtonInputEvent::MouseButtonInputEvent(MouseButton button, MouseAction action, float pos_x, float pos_y) :
        InputEvent{ EventSource::Mouse },
        m_button{ button },
        m_Action{ action },
        m_PosX{ pos_x },
        m_PosY{ pos_y }
    {
    }

    MouseButton MouseButtonInputEvent::GetButton() const
    {
        return m_button;
    }

    MouseAction MouseButtonInputEvent::GetAction() const
    {
        return m_Action;
    }

    float MouseButtonInputEvent::GetPosX() const
    {
        return m_PosX;
    }

    float MouseButtonInputEvent::GetPosY() const
    {
        return m_PosY;
    }

}
