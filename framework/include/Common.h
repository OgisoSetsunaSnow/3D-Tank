#pragma once
#include"Platform.h"


#ifdef VTS_PLATFORM_WINDOWS
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;