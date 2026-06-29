#pragma once
#include <iostream>

class BMFont
{
public:
	bool LoadFontDescription(std::istream& fileinput);
	void PostLoadFont(unsigned int textureWidth, unsigned int textureHeight);

	~BMFont();

	struct InstanceDataBuildingContext
	{
		float LocalPosX;
		float FontScale;
		float QuadWidth;

		InstanceDataBuildingContext(float fontScale, float quadWidth)
			:LocalPosX(0), FontScale(fontScale), QuadWidth(quadWidth)
		{ }
	};
	void BuildInstanceData(InstanceDataBuildingContext& ctx, char character, float(&localTransformOut)[4][4], float(&uvOffsetScaleOut)[4]);
	float GetTextTotalWidth(const char* text, float fontScale, float charQuadWidth);

private:
	int FindReferenceCharacterWidth();

	struct FontDesc
	{
		uint32_t id;	// ascii
		uint16_t x;
		uint16_t y;
		uint16_t width;
		uint16_t height;
		int16_t xoffset;
		int16_t yoffset;
		int16_t xadvance;
		uint8_t page;
		uint8_t chnl;
	};

	struct TextureCoordInfo
	{
		float UVOffsetScale[4];
		float QuadWidthScale;
		char Character;
	};

	unsigned int ScaleW = 0;
	unsigned int ScaleH = 0;
	unsigned int LineHeight = 0;
	unsigned int SpaceHorizontal = 0;
	unsigned int SpaceVertical = 0;

	unsigned int CharSetLength = 0;
	FontDesc* CharDescription = nullptr;
	TextureCoordInfo* CoordInfo = nullptr;
};