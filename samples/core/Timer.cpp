#include "pch.h"
#include "core/Timer.h"

namespace prm
{
    Timer::Timer() :
        m_StartTime{ Clock::now() },
        m_PreviousTick{ Clock::now() }
    {
    }

    void Timer::Start()
    {
        if (!m_Running)
        {
            m_Running = true;
            m_StartTime = Clock::now();
        }
    }

    void Timer::Lap()
    {
        m_Lapping = true;
        m_LapTime = Clock::now();
    }

    bool Timer::IsRunning() const
    {
        return m_Running;
    }
}
