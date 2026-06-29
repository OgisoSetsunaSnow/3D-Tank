#include "audio/PC/AudioPC.h"

#if VTS_PLATFORM_WINDOWS
#include <assert.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>
#include <xaudio2.h>
#include <xapofx.h>

AudioInterface* AudioInterface::CreateAudio()
{
	return new AudioPC();
}

AudioResourceWave* AudioInterface::CreateAudioResource()
{
	return new AudioResourceWavePC();
}

bool AudioPC::Init()
{
	//
	// Initialize XAudio2
	//
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		assert(false);
		return false;
	}

	UINT32 flags = 0;
	hr = XAudio2Create(XAudio2Device.GetAddressOf(), flags);
	if (FAILED(hr))
	{
		CoUninitialize();
		assert(false);
		return false;
	}

	//
	// Create a mastering voice
	//
	IXAudio2MasteringVoice* pMasteringVoice = nullptr;
	if (FAILED(hr = XAudio2Device->CreateMasteringVoice(&pMasteringVoice)))
	{
		XAudio2Device.Reset();
		CoUninitialize();
		assert(false);
		return false;
	}

#ifdef MASTERING_LIMITER
	FXMASTERINGLIMITER_PARAMETERS params = {};
	params.Release = FXMASTERINGLIMITER_DEFAULT_RELEASE;
	params.Loudness = FXMASTERINGLIMITER_DEFAULT_LOUDNESS;

	ComPtr<IUnknown> pVolumeLimiter;
	hr = CreateFX(__uuidof(FXMasteringLimiter), &pVolumeLimiter, &params, sizeof(params));
	if (FAILED(hr))
	{
		XAudio2Device.Reset();
		CoUninitialize();
		assert(false);
		return hr;
	}

	UINT32 nChannels;
	XAUDIO2_VOICE_DETAILS details;
	pMasteringVoice->GetVoiceDetails(&details);
	nChannels = details.InputChannels;

	XAUDIO2_EFFECT_DESCRIPTOR desc = {};
	desc.InitialState = TRUE;
	desc.OutputChannels = nChannels;
	desc.pEffect = pVolumeLimiter.Get();

	XAUDIO2_EFFECT_CHAIN chain = { 1, &desc };
	hr = pMasteringVoice->SetEffectChain(&chain);
	if (FAILED(hr))
	{
		XAudio2Device.Reset();
		pVolumeLimiter.Reset();
		CoUninitialize();
		return hr;
	}
#endif // MASTERING_LIMITER

	return true;
}

bool AudioPC::Terminate()
{
	XAudio2Device.Reset();
	CoUninitialize();
	return true;
}


// 创建SourceVoice
bool AudioResourceWavePC::WarmUp(AudioInterface* ad, bool loop)
{
	if (SourceVoice != nullptr)
		return true;

	AudioPC* adpc = (AudioPC*)ad;
	if (!adpc || !adpc->XAudio2Device || !WaveData || !WaveData->Data || WaveData->DataBytes <= 0)
		return false;

	static_assert(sizeof(WAVEFORMATEXTENSIBLE) >= sizeof(AudioResourceWave::WaveFormatDef), "the layout of WAVEFORMATEXTENSIBLE has changed ?");
	WAVEFORMATEXTENSIBLE fmtExt;
	::ZeroMemory(&fmtExt, sizeof(WAVEFORMATEXTENSIBLE));
	memcpy(&fmtExt, &WaveFormat, sizeof(AudioResourceWave::WaveFormatDef));

	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = WaveData->DataBytes;
	buffer.pAudioData = (BYTE*)WaveData->Data;
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	if (loop)
	{
		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}
	HRESULT hr;
	IXAudio2VoiceCallback* callback = this;
	UINT32 flags = 0;
	if (FAILED(hr = adpc->XAudio2Device->CreateSourceVoice(&SourceVoice, (WAVEFORMATEX*)&fmtExt, flags, XAUDIO2_DEFAULT_FREQ_RATIO, callback)))
	{
		assert(false);
		return false;
	}


	return true;
}

bool AudioResourceWavePC::Play(AudioInterface* ad, bool loop)
{
	HRESULT hr;
	if (!WarmUp(ad, loop))
		return false;
	if (!IsPlaying)	
	{
        SourceVoice->Stop();
        SourceVoice->FlushSourceBuffers();

        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = WaveData->DataBytes;
        buffer.pAudioData = (BYTE*)WaveData->Data;
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        if (loop)
        {
            buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        }
        if (FAILED(hr = SourceVoice->SubmitSourceBuffer(&buffer)))
        {
            assert(false);
			return false;
		} 
	}
	if (FAILED(hr = SourceVoice->Start(0)))
	{
		assert(false);
		return false;
	}

	IsPlaying = true;
	return true;
}

void AudioResourceWavePC::Stop(AudioInterface* ad)
{
	(void)ad;
	if (SourceVoice)
	{
		SourceVoice->Stop();
	}
}

void AudioResourceWavePC::OnStreamEnd()
{
	IsPlaying = false;
}

void AudioResourceWavePC::OnVoiceError(void* pBufferContext, HRESULT Error)
{
	(void)pBufferContext;
	(void)Error;
	assert(false);
}

AudioResourceWavePC::~AudioResourceWavePC()
{
	if (SourceVoice)
	{
		// https://gamedev.net/forums/topic/606132-is-deleting-memory-from-ixaudio2sourcevoice-possible/4834334/
		SourceVoice->DestroyVoice();
	}
}

#endif