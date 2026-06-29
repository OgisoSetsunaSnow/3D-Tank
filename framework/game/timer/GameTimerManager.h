#pragma once
#include <algorithm>
#include <functional>
#include <vector>

namespace Game
{

struct TimerHandle
{
    uint32_t id = 0;
    bool isValid() const { return id != 0; }
    void invalidate() { id = 0; }
    bool operator==(const TimerHandle& other) const { return other.id == id; }
};

struct GameTimer
{
    /**
     * @brief Construct a new Game Timer object
     *
     * @param duration    间隔时间（秒）
     * @param callback    回调函数
     * @param times       触发次数，-1 表示无限循环
     * @param immediately 是否在第一次 tick 时立即触发一次
     * @param handle      句柄
     */
    GameTimer(float duration, std::function<void()> callback, int times, bool immediately, TimerHandle handle)
        : m_duration(duration)
        , m_callBack(callback)
        , m_times(times)
        , m_immediately(immediately)
        , m_handle(handle)
    {
        m_isLoop = (m_times == -1);
        m_isActive = true;
        m_remainingTime = duration;
    }
    ~GameTimer() = default;
    
    float m_duration;
    std::function<void()> m_callBack;
    int m_times;
    bool m_immediately;  // 首帧立即触发标志（触发后自动清除）
    bool m_isLoop;
    bool m_isActive;
    TimerHandle m_handle;
    float m_remainingTime;
};

class GameTimerManager
{
public:
    GameTimerManager() = default;
    ~GameTimerManager() = default;

    /**
     * @brief 注册一个计时器
     * @param duration    间隔时间（秒）
     * @param callback    到时回调
     * @param times       触发次数，-1 = 无限循环
     * @param immediately 是否立即触发第一次
     * @return 计时器句柄，可用于手动销毁
     */
    TimerHandle setTimer(float duration, std::function<void()> callback, int times = -1, bool immediately = false);

    /** @brief 通过句柄主动销毁计时器 */
    void destroyTimer(TimerHandle handle);

    /** @brief 每帧驱动 */
    void tick(float dt);

private:
    TimerHandle createTimerHandle();

    std::vector<GameTimer> m_timers;
    uint32_t m_nextTimerId = 1;
};

}  // namespace Game
