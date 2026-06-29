#include "AudioManager.h"

#include "../audio/AudioInterface.h"
namespace Engineframework
{
void AudioManager::init()
{
    mAudio = AudioInterface::CreateAudio();
    mAudio->Init();
}
void AudioManager::shutdown()
{
    mAudio->Terminate();
    delete mAudio;
    mAudio = nullptr;
}

void AudioManager::play(AudioResourceWave* wave, bool loop)
{
    wave->Play(mAudio, loop);
}
}  // namespace Engineframework