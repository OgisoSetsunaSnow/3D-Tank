#include <assert.h>
#include "audio/AudioResourceWave.h"

//E WAV File definitions
#define WAVE_FILE_CHUNK_ID_LENGTH				(0x04)
#define WAVE_FILE_CHUNK_SIZE_LENGTH				(0x04)

static bool FindChunkInfo(std::istream& wavFile, const char* strChunkId, int* puChunkSize)
{
	bool bFoundChunk = false;
	union 
	{
		char c[4];
		int ui;
	} readBuffer;

	readBuffer.ui = 0;
	while (!wavFile.eof() && !bFoundChunk)
	{
		wavFile.read(readBuffer.c, WAVE_FILE_CHUNK_ID_LENGTH);
		bFoundChunk = (strncmp(readBuffer.c, strChunkId, WAVE_FILE_CHUNK_ID_LENGTH) == 0);
	}

	if (bFoundChunk)
	{
		wavFile.read(readBuffer.c, WAVE_FILE_CHUNK_SIZE_LENGTH);
		*puChunkSize = readBuffer.ui;
		return true;
	}

	return false;
}

bool AudioResourceWave::ReadFile(std::istream& wavFile)
{
	wavFile.seekg(0);
	int chunkSize = 0;
	if (!FindChunkInfo(wavFile, "fmt ", &chunkSize))
	{
		assert(false);
		return false;
	}
	memset(&WaveFormat, 0, sizeof(WaveFormatDef));
	wavFile.read((char*)&WaveFormat, chunkSize);

	int dataBytes = 0;
	if (!FindChunkInfo(wavFile, "data", &dataBytes))
	{
		assert(false);
		return false;
	}
		
	WaveData = std::make_shared<WaveDataBuffer>(dataBytes);

	wavFile.read(WaveData->Data, dataBytes);

	return true;
}

bool AudioResourceWave::Play(AudioInterface* ad, bool loop)
{
	(void)ad;
	(void)loop;
	assert(false);	// use AudioResourceWavePC or AudioResourceWavePS4
	return false;
}

void AudioResourceWave::Stop(AudioInterface* ad)
{
	(void)ad;
	assert(false);	// use AudioResourceWavePC or AudioResourceWavePS4
}