#include "pch.h"
#include "platform/Window.h"

namespace prm
{
    Window::Window(const Properties& properties) :
        m_Properties{ properties }
    {
    }

    void Window::ProcessEvents()
    {
    }

    Window::Extent Window::Resize(const Extent& new_extent)
    {
        if (m_Properties.resizable)
        {
            m_Properties.extent.width = new_extent.width;
            m_Properties.extent.height = new_extent.height;
        }

        return m_Properties.extent;
    }

    const Window::Extent& Window::GetExtent() const
    {
        return m_Properties.extent;
    }

    float Window::GetContentScaleFactor() const
    {
        return 1.0f;
    }

    Window::Mode Window::GetWindowMode() const
    {
        return m_Properties.mode;
    }
}
