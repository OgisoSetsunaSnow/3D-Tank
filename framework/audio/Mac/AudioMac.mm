#include <Platform.h>

#if VTS_PLATFORM_OSX
#include "audio/AudioInterface.h"
#include "audio/AudioResourceWave.h"
#include <assert.h>
#include <AudioToolbox/AudioQueue.h>  // for AudioQueue (low-level PCM playback)
#include <CoreAudio/CoreAudioTypes.h>
#include <thread>

// ---------------------------------------------------------------------------
// AudioMac – wraps a trivial output context (no persistent device needed;
// individual sounds manage their own AudioQueues).
// ---------------------------------------------------------------------------
class AudioMac : public AudioInterface
{
public:
    virtual bool Init() override { return true; }
    virtual bool Terminate() override { return true; }
};

// ---------------------------------------------------------------------------
// AudioResourceWaveMac – plays raw PCM using AudioQueue
// ---------------------------------------------------------------------------
class AudioResourceWaveMac : public AudioResourceWave
{
public:
    virtual bool WarmUp(AudioInterface* ad, bool loop) override;
    virtual bool Play(AudioInterface* ad, bool loop) override;
    virtual void Stop(AudioInterface* ad) override { (void)ad; /* fire-and-forget for this impl */ }
};

// Context passed to the AudioQueue callback so we can clean up when done.
struct AudioQueueContext
{
    AudioQueueRef        queue  = nullptr;
    AudioQueueBufferRef  buffer = nullptr;
    bool                loop       = false;      
    const void*         audioData  = nullptr;    
    UInt32              audioBytes = 0;
};

static void OnAudioPlaybackDone(void* inUserData,
                                AudioQueueRef inAQ,
                                AudioQueueBufferRef inBuffer);

static void OnAudioWarmUpDone(void* inUserData,
                              AudioQueueRef inAQ,
                              AudioQueueBufferRef inBuffer)
{
    (void)inUserData;
    (void)inAQ;
    (void)inBuffer;
}

static AudioStreamBasicDescription MakeStreamDescription(AudioResourceWave::WaveFormatDef waveFormat)
{
    AudioStreamBasicDescription asbd = {};
    asbd.mSampleRate       = (Float64)waveFormat.nSamplesPerSec;
    asbd.mFormatID         = kAudioFormatLinearPCM;
    asbd.mChannelsPerFrame = waveFormat.nChannels;
    asbd.mBitsPerChannel   = waveFormat.wBitsPerSample;
    asbd.mBytesPerFrame    = waveFormat.nBlockAlign;
    asbd.mFramesPerPacket  = 1;
    asbd.mBytesPerPacket   = waveFormat.nBlockAlign;

    if (waveFormat.wBitsPerSample == 32)
        asbd.mFormatFlags = kLinearPCMFormatFlagIsFloat | kLinearPCMFormatFlagIsPacked;
    else
        asbd.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;

    return asbd;
}

static bool WarmUpPlayback(AudioResourceWave::WaveFormatDef waveFormat)
{
    AudioStreamBasicDescription asbd = MakeStreamDescription(waveFormat);

    AudioQueueRef queue = nullptr;
    OSStatus err = AudioQueueNewOutput(&asbd,
                                       OnAudioWarmUpDone,
                                       &queue,
                                       /*inCallbackRunLoop=*/nullptr,
                                       /*inCallbackRunLoopMode=*/nullptr,
                                       /*inFlags=*/0,
                                       &queue);
    if (err != noErr)
        return false;

    AudioQueueBufferRef buffer = nullptr;
    UInt32 warmupBytes = (UInt32)std::max<int>(waveFormat.nBlockAlign, 256);
    err = AudioQueueAllocateBuffer(queue, warmupBytes, &buffer);
    if (err == noErr && buffer)
        buffer->mAudioDataByteSize = 0;

    AudioQueueDispose(queue, true);
    return err == noErr;
}

static void StartPlayback(AudioResourceWave::WaveFormatDef waveFormat,
                          std::shared_ptr<AudioResourceWave::WaveDataBuffer> waveData, bool loop)
{
    if (!waveData || !waveData->Data || waveData->DataBytes == 0)
        return;

    AudioStreamBasicDescription asbd = MakeStreamDescription(waveFormat);

    auto* ctx = new AudioQueueContext();
    ctx->loop = loop;
    ctx->audioData = waveData->Data;
    ctx->audioBytes = (UInt32)waveData->DataBytes;
    OSStatus err = AudioQueueNewOutput(&asbd,
                                       OnAudioPlaybackDone,
                                       ctx,
                                       /*inCallbackRunLoop=*/nullptr,
                                       /*inCallbackRunLoopMode=*/nullptr,
                                       /*inFlags=*/0,
                                       &ctx->queue);
    if (err != noErr)
    {
        delete ctx;
        printf("AudioMac: AudioQueueNewOutput failed (%d)\n", (int)err);
        return;
    }

    err = AudioQueueAllocateBuffer(ctx->queue, (UInt32)waveData->DataBytes, &ctx->buffer);
    if (err != noErr)
    {
        AudioQueueDispose(ctx->queue, true);
        delete ctx;
        return;
    }

    memcpy(ctx->buffer->mAudioData, waveData->Data, waveData->DataBytes);
    ctx->buffer->mAudioDataByteSize = (UInt32)waveData->DataBytes;

    AudioQueueEnqueueBuffer(ctx->queue, ctx->buffer, 0, nullptr);
    AudioQueueStart(ctx->queue, nullptr);
}

static void OnAudioPlaybackDone(void* inUserData,
                                     AudioQueueRef inAQ,
                                     AudioQueueBufferRef inBuffer)
{
    (void)inBuffer;
    // The callback fires after playback drains. Stopping the queue here can
    // race the queue's own shutdown path and trigger AudioQueueInternalStop_Sync.
    AudioQueueContext* ctx = static_cast<AudioQueueContext*>(inUserData);
    if (ctx->loop)
    {
        memcpy(inBuffer->mAudioData, ctx->audioData, ctx->audioBytes);
        inBuffer->mAudioDataByteSize = ctx->audioBytes;
        AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, nullptr);
    }
    else
    {
        AudioQueueDispose(inAQ ? inAQ : ctx->queue, true);
        delete ctx;
    }
}

bool AudioResourceWaveMac::WarmUp(AudioInterface* /*ad*/, bool loop)
{
    (void)loop;
    if (!WaveData || !WaveData->Data || WaveData->DataBytes == 0)
        return false;

    return WarmUpPlayback(WaveFormat);
}

bool AudioResourceWaveMac::Play(AudioInterface* /*ad*/, bool loop)
{
    if (!WaveData || !WaveData->Data || WaveData->DataBytes == 0)
        return false;

    AudioResourceWave::WaveFormatDef waveFormat = WaveFormat;
    std::shared_ptr<AudioResourceWave::WaveDataBuffer> waveData = WaveData;
    std::thread([waveFormat, waveData, loop]() {
        StartPlayback(waveFormat, waveData, loop);
    }).detach();

    return true;
}

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------
AudioInterface* AudioInterface::CreateAudio()
{
    return new AudioMac();
}

AudioResourceWave* AudioInterface::CreateAudioResource()
{
    return new AudioResourceWaveMac();
}

#endif // VTS_PLATFORM_OSX

