#pragma once
#include "../IManager.h"
#include "../audio/AudioResourceWave.h"
class AudioInterface;
namespace Engineframework
{
class AudioManager : public IManager
{
public:
    AudioManager() = default;
    ~AudioManager() override = default;
    void init() override;
    void shutdown() override;
    AudioInterface* getAudioInterface() const { return mAudio; }
    void play(AudioResourceWave* wave, bool loop = false);
    void pauseAll();

private:
    AudioInterface* mAudio = nullptr;
};

}  // namespace Engineframework