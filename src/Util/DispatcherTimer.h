#pragma once

#include <chrono>
#include <thread>

namespace Util
{
class DispatcherTimer
{
public:
    explicit DispatcherTimer(std::chrono::milliseconds interval)
        : m_interval(interval)
    {
    }

    void SetInterval(std::chrono::milliseconds interval)
    {
        m_interval = interval;
        m_nextTime = std::chrono::steady_clock::time_point{};
    }

    void Wait()
    {
        const auto now = std::chrono::steady_clock::now();
        if (m_nextTime == std::chrono::steady_clock::time_point{})
        {
            m_nextTime = now + m_interval;
            std::this_thread::sleep_for(m_interval);
        }
        else
        {
            if (now < m_nextTime)
            {
                std::this_thread::sleep_until(m_nextTime);
            }
            m_nextTime += m_interval;
        }
    }

private:
    std::chrono::milliseconds m_interval;
    std::chrono::steady_clock::time_point m_nextTime{};
};
}
