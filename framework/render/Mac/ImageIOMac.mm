#include <Platform.h>

#if VTS_PLATFORM_OSX
// TGA texture loading is handled by the shared render/tga.cpp + RenderTextureResource.cpp.
// This file intentionally left minimal – the PC ImageIO helpers (LoadImageFromFile /
// LoadImageFromMemory) are only referenced from PC-guarded code and are not needed on Mac.
#endif // VTS_PLATFORM_OSX

