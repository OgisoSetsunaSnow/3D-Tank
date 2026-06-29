#include "game/timer/GameTimerManager.h"
namespace Game
{

TimerHandle GameTimerManager::createTimerHandle()
{
    TimerHandle handle;
    handle.id = m_nextTimerId++;
    if (m_nextTimerId == 0)
        m_nextTimerId = 1;
    return handle;
}

TimerHandle GameTimerManager::setTimer(float duration, std::function<void()> callback, int times, bool immediately)
{
    TimerHandle handle = createTimerHandle();
    m_timers.emplace_back(duration, std::move(callback), times, immediately, handle);
    return handle;
}

void GameTimerManager::destroyTimer(TimerHandle handle)
{
    for (auto& timer : m_timers)
    {
        if (timer.m_handle == handle)
        {
            timer.m_isActive = false;  // 标记为非活跃，tick 末尾统一清除
            break;
        }
    }
}

void GameTimerManager::tick(float dt)
{
    size_t size = m_timers.size();
    for (size_t i = 0; i < size; i++)
    {
        auto& timer = m_timers[i];
        // 立即执行
        if (timer.m_immediately)
        {
            timer.m_immediately = false;
            timer.m_callBack();
            if (!timer.m_isLoop)
            {
                timer.m_times--;
                if (timer.m_times == 0)
                {
                    timer.m_isActive = false;
                    continue;
                }
            }
            continue;
        }

        if (!timer.m_isActive)
            continue;
        timer.m_remainingTime -= dt;
        if (timer.m_remainingTime < 0.f)
        {
            timer.m_callBack();  // 触发回调
            timer.m_remainingTime += timer.m_duration;
            if (timer.m_times > 0)
            {
                timer.m_times--;
            }
            if (timer.m_times == 0)
            {
                timer.m_isActive = false;
            }
        }
    }
    m_timers.erase(std::remove_if(m_timers.begin(), m_timers.end(), [](const GameTimer& t) { return !t.m_isActive; }),
                   m_timers.end());
}


}  // namespace Game
