#include <Platform.h>

#if VTS_PLATFORM_PS4
#include <scebase_common.h>
#include <audioout.h>
#include <audioout/mastering.h>
#include <assert.h>
#include "audio/PS4/AudioPS4.h"

#define NUM_BUFFERS				(2)
#define	USER_GRAIN				(256)  
#define SAMPLING_RATE			(48000)

//E AudioOut Mastering Presets
#ifdef SULPHA_EXPORTED_CUSTOM_PRESETS
#define AUDIOOUT_MASTERING_SAMPLE_NUM_PRESETS (8)
#else
#define AUDIOOUT_MASTERING_SAMPLE_NUM_PRESETS (7)
#endif

static SceAudioOutMasteringParams s_audioMasteringPresets = SCE_AUDIO_OUT_MASTERING_PRESET_SMALL_TV_V2;

//E This is the "flags" parameter associated with each of the s_audioMasteringPresets presets. 
//E By default "flags" is 0. However, custom presets exported from Sulpha will save custom configuration flags to the header file (see audioout_mastering_custom_preset.h).
static uint32_t s_audioMasteringPresetsFlags = 0;

AudioInterface* AudioInterface::CreateAudio()
{
	return new AudioPS4();
}

AudioResourceWave* AudioInterface::CreateAudioResource()
{
	return new AudioResourceWavePS4();
}


class ScoppedAutoLockPS4
{
public:
	ScoppedAutoLockPS4(pthread_mutex_t& mtx, bool hasLocked = false):CachedMutex(mtx)
	{
		if(!hasLocked)
			pthread_mutex_lock(&CachedMutex);
	}
	~ScoppedAutoLockPS4()
	{
		pthread_mutex_unlock(&CachedMutex);
	}
private:
	pthread_mutex_t& CachedMutex;
};

bool AudioPS4::Init()
{
	ContextIndex = 0;

	int threadRet;
	int returnCode = sceAudioOutInit();
	assert(returnCode == SCE_OK);

	int32_t  vol[SCE_AUDIO_OUT_CHANNEL_MAX];
	for (int i = 0; i < SCE_AUDIO_OUT_CHANNEL_MAX; i++)
	{
		vol[i] = SCE_AUDIO_VOLUME_0dB / 4;
	}

	int numChannelsConfig[MAX_PORTS_AUDIO_PS4] = { 1,1,1,1,  2,2,2,2 };
	for (int k = 0; k < MAX_PORTS_AUDIO_PS4; ++k)
	{
		Context[k].NumChannels = numChannelsConfig[k];
		Context[k].AudioPortHandle = sceAudioOutOpen(SCE_USER_SERVICE_USER_ID_SYSTEM,
			SCE_AUDIO_OUT_PORT_TYPE_MAIN,
			0,
			USER_GRAIN,
			SAMPLING_RATE,
			Context[k].NumChannels == 1 ? SCE_AUDIO_OUT_PARAM_FORMAT_S16_MONO : SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);
		if (Context[k].AudioPortHandle < 0)
		{
			assert(false);
		}

		sceAudioOutSetVolume(Context[k].AudioPortHandle, (SCE_AUDIO_VOLUME_FLAG_FL_CH | SCE_AUDIO_VOLUME_FLAG_FR_CH
			| SCE_AUDIO_VOLUME_FLAG_CNT_CH | SCE_AUDIO_VOLUME_FLAG_LFE_CH
			| SCE_AUDIO_VOLUME_FLAG_RL_CH | SCE_AUDIO_VOLUME_FLAG_RR_CH
			| SCE_AUDIO_VOLUME_FLAG_BL_CH | SCE_AUDIO_VOLUME_FLAG_BR_CH), vol);

		int32_t returnCode = sceAudioOutMasteringInit(0);
		assert(returnCode == SCE_OK);

		returnCode = sceAudioOutMasteringSetParam(&s_audioMasteringPresets.paramsHeader, s_audioMasteringPresetsFlags);
		assert(returnCode == SCE_OK);

		threadRet = pthread_mutex_init(&Context[k].BufferReadingMutex, nullptr);
		assert(threadRet == 0);
	}

	threadRet = pthread_cond_init(&BufferAvailableCondition, nullptr);
	assert(threadRet == 0);

	for (int t = 0; t < MAX_NUM_AUDIO_THREAD; ++t)
	{
		threadRet = pthread_mutex_init(&BufferAvailableMutex[t], nullptr);
		assert(threadRet == 0);
		threadRet = pthread_create(&(SoundThreads[t]), NULL, AudioPS4::ThreadLoopWrapper, this);
		assert(threadRet == 0);
	}

	return true;
}

bool AudioPS4::Terminate()
{
	IsExit = true;
	for (int t = 0; t < MAX_NUM_AUDIO_THREAD; ++t)
	{
		pthread_join(SoundThreads[t], NULL);
		pthread_mutex_destroy(&BufferAvailableMutex[t]);
	}
	for (int c = 0; c < MAX_PORTS_AUDIO_PS4; ++c)
	{
		pthread_mutex_destroy(&Context[c].BufferReadingMutex);
	}
	pthread_cond_destroy(&BufferAvailableCondition);
	return true;
}

void* AudioPS4::ThreadLoopWrapper(void* self)
{
	AudioPS4* selfps4 = (AudioPS4*)self;
	selfps4->ThreadLoop();
	return nullptr;
}

int AudioPS4::GetThreadIndex()
{
	pthread_t currentThread = pthread_self();
	for (int t=0; t<MAX_NUM_AUDIO_THREAD; ++t)
	{
		if (currentThread == SoundThreads[t])
		{
			return t;
		}
	}

	assert(false);
	return 0;
}

void AudioPS4::ThreadLoop()
{
	int sleepCounter = 0;

	do
	{
		int expected = ContextIndex;
		int newValue;
		do 
		{
			newValue = (expected + 1) % MAX_PORTS_AUDIO_PS4;
		} while (!ContextIndex.compare_exchange_strong(expected, newValue));
		
		if (Context[newValue].SendDataToOutput() == false)
		{
			++sleepCounter;
		}
		else
		{
			sleepCounter = 0;
		}

		if (sleepCounter >= MAX_PORTS_AUDIO_PS4)
		{
			sleepCounter = 0;
			int threadIndex = GetThreadIndex();
			pthread_mutex_lock(&BufferAvailableMutex[threadIndex]);
			pthread_cond_wait(&BufferAvailableCondition, &BufferAvailableMutex[threadIndex]);
		}
	} while (IsExit == false);

	ThreadLoopTerminate();
}

void AudioPS4::ThreadLoopTerminate()
{
	for (int k = 0; k < MAX_PORTS_AUDIO_PS4; ++k)
	{
		sceAudioOutOutput(Context[k].AudioPortHandle, NULL);
	}

	int returnCode = sceAudioOutMasteringTerm();
	assert(returnCode == SCE_OK);

	for (int k = 0; k < MAX_PORTS_AUDIO_PS4; ++k)
	{
		sceAudioOutClose(Context[k].AudioPortHandle);
	}
}

bool AudioPS4::BeginPlay(std::shared_ptr<AudioResourceWave::WaveDataBuffer>& newBuffer, int numChannels)
{
	int retryCount = 0;
	int k = 0;
	bool isSuccessful = false;
	while (retryCount <= MAX_PORTS_AUDIO_PS4)
	{
		if (pthread_mutex_trylock(&Context[k].BufferReadingMutex) == 0)
		{
			ScoppedAutoLockPS4 holdlock(Context[k].BufferReadingMutex, true);
			if (Context[k].IsIdle() && Context[k].IsNumChannelsTheSame(numChannels))
			{
				Context[k].BeginPlay(newBuffer);
				pthread_cond_signal(&BufferAvailableCondition);
				isSuccessful = true;
				break;
			}
		}
		++retryCount;
		k = (k + 1) % MAX_PORTS_AUDIO_PS4;
	}

	return isSuccessful;
}

void AudioPS4::EndPlay(std::shared_ptr<AudioResourceWave::WaveDataBuffer>& playingBuffer)
{
	int contextIndex = -1;
	for (int k = 0; k < MAX_PORTS_AUDIO_PS4; ++k)
	{
		if (Context[k].CurrentPlayingBuffer.get() == playingBuffer.get())
		{
			contextIndex = k;
			break;
		}
	}

	if (contextIndex >= 0)
	{
#if 1
		ScoppedAutoLockPS4 holdlock(Context[contextIndex].BufferReadingMutex);
		Context[contextIndex].EndPlay();
#else
		struct timespec timeoutTime;
		clock_gettime(CLOCK_REALTIME, &timeoutTime);
		timeoutTime.tv_nsec += 1000 * 500;
		if (pthread_mutex_timedlock(&Context[contextIndex].BufferReadingMutex, &timeoutTime) == 0)
		{
			ScoppedAutoLockPS4 holdlock(Context[contextIndex].BufferReadingMutex, true);
			Context[contextIndex].EndPlay();
		}
#endif
	}
}

bool AudioPS4Context::SendDataToOutput()
{
	if (pthread_mutex_trylock(&BufferReadingMutex) != 0)
	{
		return false;
	}

	ScoppedAutoLockPS4 holdlock(BufferReadingMutex, true);
	if (CurrentPlayingBuffer.get() == nullptr || BufferReadingPosition >= CurrentPlayingBuffer->DataBytes)
	{
		return false;
	}

	size_t bufferSizePerFrame = USER_GRAIN * NumChannels * sizeof(uint16_t);
	uint16_t* outputBuffer;
	assert(BufferReadingPosition < CurrentPlayingBuffer->DataBytes);
	size_t readableDataSize = CurrentPlayingBuffer->DataBytes - BufferReadingPosition;
	// The remaining amount of data is less than the data output unit?
	if (readableDataSize < bufferSizePerFrame)
	{
		// Copy to remainingAdjustBuffer the rest of the data.
		int16_t  remainingAdjustBuffer[USER_GRAIN * SCE_AUDIO_OUT_CHANNEL_MAX];
		memset(remainingAdjustBuffer, 0, sizeof(remainingAdjustBuffer));
		memcpy(remainingAdjustBuffer, CurrentPlayingBuffer->Data + BufferReadingPosition, readableDataSize);
		outputBuffer = (uint16_t*)remainingAdjustBuffer;
	}
	else
	{
		outputBuffer = (uint16_t*)(CurrentPlayingBuffer->Data + BufferReadingPosition);
	}

	int numSamplesSent = sceAudioOutOutput(AudioPortHandle, outputBuffer);
	assert(numSamplesSent == USER_GRAIN || numSamplesSent == 0);
	BufferReadingPosition += bufferSizePerFrame;

	if (BufferReadingPosition >= CurrentPlayingBuffer->DataBytes)
	{
		EndPlay();
	}

	return true;
}

void AudioPS4Context::BeginPlay(std::shared_ptr<AudioResourceWave::WaveDataBuffer>& newBuffer)
{
	CurrentPlayingBuffer = newBuffer;
	BufferReadingPosition = 0;
}

void AudioPS4Context::EndPlay()
{
	CurrentPlayingBuffer.reset();
	BufferReadingPosition = 0;
}

bool AudioPS4Context::IsIdle() const
{
	return CurrentPlayingBuffer.get() == nullptr;
}

bool AudioPS4Context::IsNumChannelsTheSame(int numChannels) const
{
	return NumChannels == numChannels;
}

bool AudioResourceWavePS4::WarmUp(AudioInterface* ad)
{
	(void)ad;
	return WaveData.get() != nullptr;
}

bool AudioResourceWavePS4::Play(AudioInterface* ad)
{
	AudioPS4* adps4 = (AudioPS4*)ad;
	return adps4->BeginPlay(WaveData, WaveFormat.nChannels);
}

void AudioResourceWavePS4::Stop(AudioInterface* ad)
{
	AudioPS4* adps4 = (AudioPS4*)ad;
	adps4->EndPlay(WaveData);
}
#endif