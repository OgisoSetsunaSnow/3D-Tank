#include "FpsCounter.h"

#include "gameframework/timer/timer.h"
namespace Engineframework
{
void FpsCounter::update(const Timestep& ts)
{
    mFpsSamples[mIndex] = ts.getSeconds();
    mIndex = (mIndex + 1) % SAMPLE_COUNT;
    if (mSamplesFilled < SAMPLE_COUNT)
        ++mSamplesFilled;

    float sum = 0.f;
    for (int i = 0; i < mSamplesFilled; ++i)
        sum += mFpsSamples[i];
    mAvgDt = sum / static_cast<float>(mSamplesFilled);

    snprintf(mFpsText, sizeof(mFpsText), "FPS: %.0f  (%.2f ms)", 1.0f / mAvgDt, mAvgDt * 1000.f);
}
float FpsCounter::getFps() const
{
    return mSamplesFilled > 0 ? 1.f / mAvgDt : 0.f;
}
const char* FpsCounter::getText() const
{
    return mFpsText;
}

}  // namespace Engineframework