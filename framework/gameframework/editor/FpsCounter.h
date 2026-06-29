#pragma once
namespace Engineframework
{
class Timestep;
class FpsCounter
{
public:
    FpsCounter() = default;
    ~FpsCounter() = default;
    void update(const Timestep& ts);
    float getFps() const;
    const char* getText() const;
    float getAvdDt() { return mAvgDt; }

private:
    static constexpr int SAMPLE_COUNT = 60;
    float mFpsSamples[SAMPLE_COUNT] = {};
    int mIndex = 0;
    int mSamplesFilled = 0;
    char mFpsText[48] = "FPS: --";
    float mAvgDt = 0.f;
};
}  // namespace Engineframework
