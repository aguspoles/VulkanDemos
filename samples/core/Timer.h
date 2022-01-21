#pragma once

namespace prm
{
    /**
     * Encapsulates basic usage of chrono, providing a means to calculate float
     *        durations between time points via function calls.
     */
    class Timer
    {
    public:
        using Seconds = std::ratio<1>;
        using Milliseconds = std::ratio<1, 1000>;
        using Microseconds = std::ratio<1, 1000000>;
        using Nanoseconds = std::ratio<1, 1000000000>;

        // Configure
        using Clock = std::chrono::steady_clock;
        using DefaultResolution = Seconds;

        Timer();

        virtual ~Timer() = default;

        /**
         * Starts the timer, elapsed() now returns the duration since start()
         */
        void Start();

        /**
         * Laps the timer, elapsed() now returns the duration since the last lap()
         */
        void Lap();

        /**
         * Stops the timer, elapsed() now returns 0
         * @return The total execution time between `start()` and `stop()`
         */
        template <typename T = DefaultResolution>
        double Stop()
        {
            if (!m_Running)
            {
                return 0;
            }

            m_Running = false;
            m_Lapping = false;
            auto duration = std::chrono::duration<double, T>(Clock::now() - m_StartTime);
            m_StartTime = Clock::now();
            m_LapTime = Clock::now();

            return duration.count();
        }

        /**
         * Calculates the time difference between now and when the timer was started
         *        if lap() was called, then between now and when the timer was last lapped
         * @return The duration between the two time points (default in seconds)
         */
        template <typename T = DefaultResolution>
        double Elapsed()
        {
            if (!m_Running)
            {
                return 0;
            }

            Clock::time_point start = m_StartTime;

            if (m_Lapping)
            {
                start = m_LapTime;
            }

            return std::chrono::duration<double, T>(Clock::now() - start).count();
        }

        /**
         * Calculates the time difference between now and the last time this function was called
         * @return The duration between the two time points (default in seconds)
         */
        template <typename T = DefaultResolution>
        double Tick()
        {
            const auto now = Clock::now();
            const auto duration = std::chrono::duration<double, T>(now - m_PreviousTick);
            m_PreviousTick = now;
            return duration.count();
        }

        /**
         * Check if the timer is running
         */
        bool IsRunning() const;

    private:
        bool m_Running{ false };

        bool m_Lapping{ false };

        Clock::time_point m_StartTime;

        Clock::time_point m_LapTime;

        Clock::time_point m_PreviousTick;
    };
}  
