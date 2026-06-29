#include <assert.h>
#include <math.h>
#include "render/BMfont.h"

// https://www.angelcode.com/products/bmfont/

#define PTR_ADD(ptr, offset) (((char*)ptr) + offset)


BMFont::~BMFont()
{
    if (CharDescription)
    {
        delete[] CharDescription;
        CharDescription = nullptr;
    }
    if (CoordInfo)
    {
        delete[] CoordInfo;
        CoordInfo = nullptr;
    }
}

bool BMFont::LoadFontDescription(std::istream& fileinput)
{
	if (CharDescription)
	{
		return true;
	}

	size_t bufferSize = 1024 * 1024;
	char* buffer = (char*)malloc(bufferSize);
	fileinput.read(buffer, bufferSize);
	assert(fileinput.eof());	// bufferSize is not enough ?

	const size_t header_size = 4; //"BMF", char=3 (version 3)
	uint8_t* blockType = (uint8_t*)PTR_ADD(buffer, header_size); // Block type is just the number of the block
	uint32_t* blockLen = (uint32_t*)PTR_ADD(blockType, sizeof(uint8_t)); // How many bytes long this block is, not counting the type or len.
	void* block = PTR_ADD(buffer, header_size + sizeof(uint8_t) + sizeof(uint32_t)); // The block itself
	SpaceHorizontal = *(uint8_t*)PTR_ADD(block, 11);
	SpaceVertical = *(uint8_t*)PTR_ADD(block, 12);
	// Check we found the first block ...
	if (*blockType != 1)
	{
		assert(false);
		free(buffer);
		return false;
	}

	// and move onto the second
	blockType = (uint8_t*)PTR_ADD(block, *blockLen);
	blockLen = (uint32_t*)PTR_ADD(blockType, 1);
	block = PTR_ADD(blockType, sizeof(uint8_t) + sizeof(uint32_t));
	// Check we found the second block
	if (*blockType != 2)
	{
		assert(false);
		free(buffer);
		return false;
	}
	// And pull the fields we need out of it
	LineHeight = *(uint16_t*)PTR_ADD(block, 0);
	ScaleW = *(uint16_t*)PTR_ADD(block, 4);
	ScaleH = *(uint16_t*)PTR_ADD(block, 6);

	// Move onto the third block
	blockType = (uint8_t*)PTR_ADD(block, *blockLen);
	blockLen = (uint32_t*)PTR_ADD(blockType, 1);
	block = PTR_ADD(blockType, sizeof(uint8_t) + sizeof(uint32_t));

	// Check we found it
	if (*blockType != 3)
	{
		assert(false);
		free(buffer);
		return false;
	}
		
	// And move onto the fourth block
	blockType = (uint8_t*)PTR_ADD(block, *blockLen);
	blockLen = (uint32_t*)PTR_ADD(blockType, 1);
	block = PTR_ADD(blockType, sizeof(uint8_t) + sizeof(uint32_t));

	// Check we found it
	if (*blockType != 4)
	{
		assert(false);
		free(buffer);
		return false;
	}

	// As per the docs, the way to know how many characters there are is to divide the block size by
	// the size of a character's description, which is 20 bytes.
	CharSetLength = *blockLen / 20;
	assert((int64_t)PTR_ADD(block, CharSetLength * sizeof(FontDesc)) < (int64_t)PTR_ADD(buffer, bufferSize));
	CharDescription = new FontDesc[CharSetLength];
	memset(CharDescription, 0, CharSetLength * sizeof(FontDesc));
	memcpy(CharDescription, block, CharSetLength * sizeof(FontDesc));
	free(buffer);
	return true;
}

void BMFont::PostLoadFont(unsigned int textureWidth, unsigned int textureHeight)
{
	if (CoordInfo)
	{
		delete[] CoordInfo;
	}

	assert(CharSetLength > 0 && CharDescription);
	assert(textureWidth == ScaleW && textureHeight == ScaleH);
	CoordInfo = new TextureCoordInfo[CharSetLength];

	int refWidth = FindReferenceCharacterWidth();
	for (unsigned int k = 0; k < CharSetLength; ++k)
	{
		assert(CharDescription[k].id <= 255);
		CoordInfo[k].Character = (char)(CharDescription[k].id);
		(void)CoordInfo[k].Character;
		CoordInfo[k].UVOffsetScale[0] = 1.0f * CharDescription[k].x / textureWidth;			// u offset
		CoordInfo[k].UVOffsetScale[1] = 1.0f * CharDescription[k].y / textureHeight;		// v offset
		CoordInfo[k].UVOffsetScale[2] = 1.0f * CharDescription[k].width / textureWidth;		// u scale
		CoordInfo[k].UVOffsetScale[3] = 1.0f * CharDescription[k].height / textureHeight;		// u scale
		CoordInfo[k].QuadWidthScale = 1.0f * CharDescription[k].width / refWidth;			// if quad mesh can be scaled down on width if font is not mono size 
	}
}

int BMFont::FindReferenceCharacterWidth()
{
	int referenceWidth = 0;
	int minDiff = INT_MAX;
	for (unsigned int x = 0; x < CharSetLength; ++x)
	{
		int diff = abs(CharDescription[x].width - CharDescription[x].height);
		if (diff < minDiff)
		{
			minDiff = diff;
			referenceWidth = std::max(CharDescription[x].width, CharDescription[x].height);
		}
	}

	assert(referenceWidth > 0);
	return referenceWidth;
}

void BMFont::BuildInstanceData(InstanceDataBuildingContext& ctx, char character, float(&localTransformOut)[4][4], float(&uvOffsetScaleOut)[4])
{
	for (unsigned int c = 0; c < CharSetLength; ++c)
	{
		if (CoordInfo[c].Character == character)
		{
			float quadScaleX = CoordInfo[c].QuadWidthScale * ctx.FontScale;
			float quadWidthThisChar = ctx.QuadWidth * quadScaleX;
			// quad pivot point is at the center of quad
			// offset of LocalPosX is 1/2 width of this character + 1/2 width of previous character
			ctx.LocalPosX += quadWidthThisChar / 2;
			float localxform[4][4] =
			{
				{quadScaleX, 0, 0, 0},
				{0, ctx.FontScale, 0, 0},
				{0, 0, 1.0f, 0},
				{ctx.LocalPosX, 0, 0, 1}
			};
			memcpy(localTransformOut, localxform, sizeof(localxform));
			memcpy(uvOffsetScaleOut, CoordInfo[c].UVOffsetScale, sizeof(CoordInfo[c].UVOffsetScale));
			ctx.LocalPosX += quadWidthThisChar / 2;
			break;
		}
	}
}

float BMFont::GetTextTotalWidth(const char* text, float fontScale, float charQuadWidth)
{
	int length = (int)strlen(text);
	assert(length < 1024);	// null terminated ?

	float charQuadLocalTransform[4][4];
	float UVOffsetScale[4];

	BMFont::InstanceDataBuildingContext ctx(fontScale, charQuadWidth);
	for (int k = 0; k < length; ++k)
	{
		BuildInstanceData(ctx, text[k], charQuadLocalTransform, UVOffsetScale);
	}

	return ctx.LocalPosX;
}