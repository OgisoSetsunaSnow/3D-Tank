#pragma once

// Platform
#define VTS_PLATFORM_ANDROID    0
#define VTS_PLATFORM_IOS        0
#define VTS_PLATFORM_LINUX      0
#define VTS_PLATFORM_NX         0
#define VTS_PLATFORM_OSX        0
#define VTS_PLATFORM_PS4        0
#define VTS_PLATFORM_PS5        0
#define VTS_PLATFORM_VISIONOS   0
#define VTS_PLATFORM_WINDOWS    0
#define VTS_PLATFORM_XBOXONE    0

// Architecture
#define VTS_ARCH_32BIT 0
#define VTS_ARCH_64BIT 0

// Language standard version
#define VTS_LANGUAGE_CPP17 201703L
#define VTS_LANGUAGE_CPP20 202002L
#define VTS_LANGUAGE_CPP23 202207L

// Minimum Windows desktop target
#define VTS_WINVER_WINDOWS_10      0x0A00

#if defined(__x86_64__)    \
 || defined(_M_X64)        \
 || defined(__aarch64__)   \
 || defined(__64BIT__)     \
 || defined(__mips64)      \
 || defined(__powerpc64__) \
 || defined(__ppc64__)     \
 || defined(__LP64__)
#	undef  VTS_ARCH_64BIT
#	define VTS_ARCH_64BIT 64
#else
#	undef  VTS_ARCH_32BIT
#	define VTS_ARCH_32BIT 32
#endif //

#if defined(_DURANGO) || defined(_XBOX_ONE)
#	undef  VTS_PLATFORM_XBOXONE
#	define VTS_PLATFORM_XBOXONE 1
#elif defined(_WIN32) || defined(_WIN64)
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif // NOMINMAX
#	if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#		undef  VTS_PLATFORM_WINDOWS

#		if !defined(WINVER) && defined(_WIN32_WINNT)
#			define WINVER _WIN32_WINNT
#		elif defined(WINVER) && !defined(_WIN32_WINNT)
#			define _WIN32_WINNT WINVER
#		endif // synchronize Windows version macros
#		if !defined(WINVER)
#			define WINVER VTS_WINVER_WINDOWS_10
#		endif // !defined(WINVER)
#		if !defined(_WIN32_WINNT)
#			define _WIN32_WINNT VTS_WINVER_WINDOWS_10
#		endif // !defined(_WIN32_WINNT)
#		if (WINVER < VTS_WINVER_WINDOWS_10) || (_WIN32_WINNT < VTS_WINVER_WINDOWS_10)
#			error Windows 10 or later is required for the DirectX 12 renderer.
#		endif // WINVER/_WIN32_WINNT minimum
#		define VTS_PLATFORM_WINDOWS _WIN32_WINNT
#	endif
#elif defined(__ANDROID__)
// Android compiler defines __linux__
#	include <sys/cdefs.h> // Defines __BIONIC__ and includes android/api-level.h
#	undef  VTS_PLATFORM_ANDROID
#	define VTS_PLATFORM_ANDROID __ANDROID_API__
#elif  defined(__linux__)
#	undef  VTS_PLATFORM_LINUX
#	define VTS_PLATFORM_LINUX 1
#elif  defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) \
	|| defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__)
#	undef  VTS_PLATFORM_IOS
#	define VTS_PLATFORM_IOS 1
#elif defined(__has_builtin) && __has_builtin(__is_target_os) && __is_target_os(xros)
#	undef  VTS_PLATFORM_VISIONOS
#	define VTS_PLATFORM_VISIONOS 1
#elif defined(__has_builtin) && __has_builtin(__is_target_os) && __is_target_os(xros)
#	undef  VTS_PLATFORM_VISIONOS
#	define VTS_PLATFORM_VISIONOS 1
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#	undef  VTS_PLATFORM_OSX
#	define VTS_PLATFORM_OSX __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#elif defined(__ORBIS__)
#	undef  VTS_PLATFORM_PS4
#	define VTS_PLATFORM_PS4 1
#elif defined(__PROSPERO__)
#	undef  VTS_PLATFORM_PS5
#	define VTS_PLATFORM_PS5 1
#elif defined(__NX__)
#	undef  VTS_PLATFORM_NX
#	define VTS_PLATFORM_NX 1
#endif //


