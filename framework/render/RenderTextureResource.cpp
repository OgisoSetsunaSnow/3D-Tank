
#include <iostream>
#include <assert.h>
#include "render/RenderInterface.h"
#include "render/tga.h"

class TgaFileStreamer : public tga::FileInterface {
public:
	TgaFileStreamer(std::istream& _inputStream)
		:InputStream(_inputStream)
	{
		_inputStream.seekg(0);
	}

	bool ok() const override
	{
		return !InputStream.eof();
	}

	size_t tell() override
	{
		return InputStream.tellg();
	}

	void seek(size_t absPos) override
	{
		InputStream.seekg(absPos);
	}

	uint8_t read8() override
	{
		InputStream.read((char*)ReadBuffer, 1);
		return ReadBuffer[0];
	}

	void write8(uint8_t value) override
	{
		(void)value;
		assert(false);	// not implemented
	}

private:
	std::istream& InputStream;
	uint8_t ReadBuffer[1];
};

bool RenderTextureResource::LoadTGATexture(std::istream& inputStream)
{
	TgaFileStreamer file(inputStream);
	tga::Decoder decoder(&file);

	tga::Header header;
	if (!decoder.readHeader(header))
	{
		return false;
	}

	tga::Image image;
	image.bytesPerPixel = header.bytesPerPixel();
	assert(image.bytesPerPixel == RGBAPixel::GetBytesPerPixel());
	image.rowstride = header.width * image.bytesPerPixel;
	image.pixels = (uint8_t*)malloc(header.height * image.rowstride);
	memset(image.pixels, 0, header.height * image.rowstride);

	if (!decoder.readImage(header, image, nullptr))
	{
		free(image.pixels);
		return false;
	}

	// Optional post-process to fix the alpha channel in
	// some TGA files where alpha=0 for all pixels when
	// it shouldn't.
	decoder.postProcessImage(header, image);

	RGBATextureDataCPU = (RGBAPixel*)image.pixels;
	Width = header.width;
	Height = header.height;

	return true;
}

void RenderTextureResource::ResetTextureData()
{
	if (RGBATextureDataCPU)
	{
		free(RGBATextureDataCPU);
		RGBATextureDataCPU = nullptr;
		RGBATextureDataBytes = 0;

		Width = 0;
		Height = 0;
	}
}

void RenderTextureResource::CreateDefaultTextureData()
{
	Width = 16;
	Height = 16;

	RGBATextureDataBytes = sizeof(RGBAPixel) * Width * Height;
	RGBATextureDataCPU = (RGBAPixel*)realloc(RGBATextureDataCPU, RGBATextureDataBytes);
	for (int k = 0; k < Width * Height; ++k)
	{
		RGBATextureDataCPU[k].PixelData[0] = 255;
		RGBATextureDataCPU[k].PixelData[1] = 255;
		RGBATextureDataCPU[k].PixelData[2] = 255;
		RGBATextureDataCPU[k].PixelData[3] = 255;
	}
}