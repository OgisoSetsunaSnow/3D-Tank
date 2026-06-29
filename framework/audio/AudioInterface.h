#pragma once

#include "audio/AudioResourceWave.h"

class AudioInterface
{
public:
	static AudioInterface* CreateAudio();
	static AudioResourceWave* CreateAudioResource();
	virtual ~AudioInterface() = default;
	virtual bool Init() { return false; }
	virtual bool Terminate() { return false; }
};