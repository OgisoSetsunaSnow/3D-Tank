#pragma once
#include <Platform.h>

#if VTS_PLATFORM_WINDOWS

#include "audio/AudioInterface.h"
#include "audio/AudioResourceWave.h"

#include <wrl/client.h>
#include <xaudio2.h>

class AudioPC : public AudioInterface
{
    friend class AudioResourceWavePC;

public:
    virtual bool Init() override;
    virtual bool Terminate() override;

protected:
    Microsoft::WRL::ComPtr<IXAudio2> XAudio2Device;
};


class AudioResourceWavePC : public AudioResourceWave, public IXAudio2VoiceCallback
{
public:
    virtual bool WarmUp(AudioInterface* ad, bool loop) override;
    virtual bool Play(AudioInterface* ad, bool loop) override;
    virtual void Stop(AudioInterface* ad) override;
    virtual ~AudioResourceWavePC();

    // IXAudio2VoiceCallback
    virtual void OnVoiceProcessingPassStart(UINT32 bytesRequired) { (void) bytesRequired; }
    virtual void OnVoiceProcessingPassEnd() {}
    virtual void OnStreamEnd();
    virtual void OnBufferStart(void* pBufferContext) { (void) pBufferContext; }
    virtual void OnBufferEnd(void* pBufferContext) { (void) pBufferContext; }
    virtual void OnLoopEnd(void* pBufferContext) { (void) pBufferContext; }
    virtual void OnVoiceError(void* pBufferContext, HRESULT Error);

protected:
    bool IsPlaying = false;
    IXAudio2SourceVoice* SourceVoice = nullptr;
};

#endif