#pragma once
#include <chrono>
namespace Engineframework
{
class Timestep
{
public:
    Timestep(float time = 0.0f)
        : mTime(time)
    {
    }

    operator float() const { return mTime; }

    inline float getSeconds() const { return mTime; }
    inline float getMilliseconds() const { return mTime * 1000.0f; }

private:
    float mTime;
};

class Timer
{
public:
    Timer() { reset(); }
    void reset()
    {
        mStart = std::chrono::steady_clock::now();
        mLastFrameTime = 0.0f;
    }
    float getTime() const { return std::chrono::duration<float>(std::chrono::steady_clock::now() - mStart).count(); }
    Timestep tick()
    {
        float time = getTime();
        float rawDt = time - mLastFrameTime;
        mLastFrameTime = time;
        rawDt = rawDt > MAX_DELTA ? MAX_DELTA : rawDt;
        return Timestep(rawDt);
    }

private:
    static constexpr float MAX_DELTA = 0.1f;
    std::chrono::time_point<std::chrono::steady_clock> mStart;
    float mLastFrameTime = 0.0f;
};
}  // namespace Engineframework
