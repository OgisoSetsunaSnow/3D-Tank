#pragma once

#include <Platform.h>

#if VTS_PLATFORM_PS4
#include "audio/AudioInterface.h"
#include <pthread.h>
#include <memory>
#include <atomic>

#define MAX_PORTS_AUDIO_PS4 (8)
#define MAX_NUM_AUDIO_THREAD (2)

class AudioPS4Context
{
	friend class AudioResourceWavePS4;
	friend class AudioPS4;
public:
	bool SendDataToOutput();
	bool IsIdle() const;
	bool IsNumChannelsTheSame(int numChannels) const;
protected:
	void BeginPlay(std::shared_ptr<AudioResourceWave::WaveDataBuffer>& newBuffer);
	void EndPlay();
	std::shared_ptr<AudioResourceWave::WaveDataBuffer> CurrentPlayingBuffer;
	uint32_t BufferReadingPosition = 0;
	uint32_t NumChannels = 0;
	int32_t AudioPortHandle = 0;
	pthread_mutex_t BufferReadingMutex;
};

class AudioPS4 : public AudioInterface
{
	friend class AudioResourceWavePS4;

public:
	virtual bool Init() override;
	virtual bool Terminate() override;

protected:
	bool BeginPlay(std::shared_ptr<AudioResourceWave::WaveDataBuffer>& newBuffer, int numChannels);
	void EndPlay(std::shared_ptr<AudioResourceWave::WaveDataBuffer>& playingBuffer);
	void ThreadLoop();
	void ThreadLoopTerminate();
	static void* ThreadLoopWrapper(void*);
	int GetThreadIndex();
	pthread_t SoundThreads[MAX_NUM_AUDIO_THREAD];
	pthread_mutex_t BufferAvailableMutex[MAX_NUM_AUDIO_THREAD];
	pthread_cond_t BufferAvailableCondition;
	std::atomic<int> ContextIndex;
	bool IsExit = false;
	AudioPS4Context Context[MAX_PORTS_AUDIO_PS4];
};

class AudioResourceWavePS4 : public AudioResourceWave
{
public:
	virtual bool WarmUp(AudioInterface* ad) override;
	virtual bool Play(AudioInterface* ad) override;
	virtual void Stop(AudioInterface* ad) override;

protected:
};

#endif