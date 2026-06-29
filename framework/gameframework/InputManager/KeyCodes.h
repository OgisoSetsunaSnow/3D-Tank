#pragma once

#include <cstdint>

#if VTS_PLATFORM_OSX
#import <Carbon/Carbon.h>
#endif
/**
 * @brief 键盘按键码定义（统一定义）
 * 使用虚拟键码(VK_*)对应的值
 * 所有输入相关模块都应使用这个统一定义
 */
enum class KeyCode : uint8_t
{
	// 字母键
	A = 0x41,
	B = 0x42,
	C = 0x43,
	D = 0x44,
	E = 0x45,
	F = 0x46,
	G = 0x47,
	H = 0x48,
	I = 0x49,
	J = 0x4A,
	K = 0x4B,
	L = 0x4C,
	M = 0x4D,
	N = 0x4E,
	O = 0x4F,
	P = 0x50,
	Q = 0x51,
	R = 0x52,
	S = 0x53,
	T = 0x54,
	U = 0x55,
	V = 0x56,
	W = 0x57,
	X = 0x58,
	Y = 0x59,
	Z = 0x5A,

	// 数字键
	Num0 = 0x30,
	Num1 = 0x31,
	Num2 = 0x32,
	Num3 = 0x33,
	Num4 = 0x34,
	Num5 = 0x35,
	Num6 = 0x36,
	Num7 = 0x37,
	Num8 = 0x38,
	Num9 = 0x39,

	// 方向键
	Left = 0x25,
	Up = 0x26,
	Right = 0x27,
	Down = 0x28,

	// 功能键
	F1 = 0x70,
	F2 = 0x71,
	F3 = 0x72,
	F4 = 0x73,
	F5 = 0x74,
	F6 = 0x75,
	F7 = 0x76,
	F8 = 0x77,
	F9 = 0x78,
	F10 = 0x79,
	F11 = 0x7A,
	F12 = 0x7B,

	// 特殊键
	Space = 0x20,
	Enter = 0x0D,
	Escape = 0x1B,
	Backspace = 0x08,
	Tab = 0x09,
	Shift = 0x10,
	Control = 0x11,
	Alt = 0x12,

	// 鼠标按键（兼容性）
	MouseLeft = 0x01,
	MouseRight = 0x02,
	MouseMiddle = 0x04,
};

/**
 * @brief 鼠标按键定义
 */
enum class MouseButton : int
{
	Left = 0,    // 左键
	Right = 1,   // 右键
	Middle = 2   // 中键
};

#if VTS_PLATFORM_OSX
inline CGKeyCode toCGKeyCode(KeyCode key)
{
    switch (key)
    {
    // 字母键
    case KeyCode::A:
        return kVK_ANSI_A;
    case KeyCode::B:
        return kVK_ANSI_B;
    case KeyCode::C:
        return kVK_ANSI_C;
    case KeyCode::D:
        return kVK_ANSI_D;
    case KeyCode::E:
        return kVK_ANSI_E;
    case KeyCode::F:
        return kVK_ANSI_F;
    case KeyCode::G:
        return kVK_ANSI_G;
    case KeyCode::H:
        return kVK_ANSI_H;
    case KeyCode::I:
        return kVK_ANSI_I;
    case KeyCode::J:
        return kVK_ANSI_J;
    case KeyCode::K:
        return kVK_ANSI_K;
    case KeyCode::L:
        return kVK_ANSI_L;
    case KeyCode::M:
        return kVK_ANSI_M;
    case KeyCode::N:
        return kVK_ANSI_N;
    case KeyCode::O:
        return kVK_ANSI_O;
    case KeyCode::P:
        return kVK_ANSI_P;
    case KeyCode::Q:
        return kVK_ANSI_Q;
    case KeyCode::R:
        return kVK_ANSI_R;
    case KeyCode::S:
        return kVK_ANSI_S;
    case KeyCode::T:
        return kVK_ANSI_T;
    case KeyCode::U:
        return kVK_ANSI_U;
    case KeyCode::V:
        return kVK_ANSI_V;
    case KeyCode::W:
        return kVK_ANSI_W;
    case KeyCode::X:
        return kVK_ANSI_X;
    case KeyCode::Y:
        return kVK_ANSI_Y;
    case KeyCode::Z:
        return kVK_ANSI_Z;

    // 数字键
    case KeyCode::Num0:
        return kVK_ANSI_0;
    case KeyCode::Num1:
        return kVK_ANSI_1;
    case KeyCode::Num2:
        return kVK_ANSI_2;
    case KeyCode::Num3:
        return kVK_ANSI_3;
    case KeyCode::Num4:
        return kVK_ANSI_4;
    case KeyCode::Num5:
        return kVK_ANSI_5;
    case KeyCode::Num6:
        return kVK_ANSI_6;
    case KeyCode::Num7:
        return kVK_ANSI_7;
    case KeyCode::Num8:
        return kVK_ANSI_8;
    case KeyCode::Num9:
        return kVK_ANSI_9;

    // 方向键
    case KeyCode::Left:
        return kVK_LeftArrow;
    case KeyCode::Up:
        return kVK_UpArrow;
    case KeyCode::Right:
        return kVK_RightArrow;
    case KeyCode::Down:
        return kVK_DownArrow;

    // 功能键
    case KeyCode::F1:
        return kVK_F1;
    case KeyCode::F2:
        return kVK_F2;
    case KeyCode::F3:
        return kVK_F3;
    case KeyCode::F4:
        return kVK_F4;
    case KeyCode::F5:
        return kVK_F5;
    case KeyCode::F6:
        return kVK_F6;
    case KeyCode::F7:
        return kVK_F7;
    case KeyCode::F8:
        return kVK_F8;
    case KeyCode::F9:
        return kVK_F9;
    case KeyCode::F10:
        return kVK_F10;
    case KeyCode::F11:
        return kVK_F11;
    case KeyCode::F12:
        return kVK_F12;

    // 特殊键
    case KeyCode::Space:
        return kVK_Space;
    case KeyCode::Enter:
        return kVK_Return;
    case KeyCode::Escape:
        return kVK_Escape;
    case KeyCode::Backspace:
        return kVK_Delete;  // macOS Delete = PC Backspace
    case KeyCode::Tab:
        return kVK_Tab;
    case KeyCode::Shift:
        return kVK_Shift;
    case KeyCode::Control:
        return kVK_Control;
    case KeyCode::Alt:
        return kVK_Option;

    default:
        return static_cast<CGKeyCode>(0xFF);  // 未映射
    }
}

// KeyCodes.h 中全部定义的键，供 macOS 逐帧轮询使用
static constexpr KeyCode kAllKeys[] = {
    KeyCode::A,      KeyCode::B,         KeyCode::C,     KeyCode::D,     KeyCode::E,       KeyCode::F,
    KeyCode::G,      KeyCode::H,         KeyCode::I,     KeyCode::J,     KeyCode::K,       KeyCode::L,
    KeyCode::M,      KeyCode::N,         KeyCode::O,     KeyCode::P,     KeyCode::Q,       KeyCode::R,
    KeyCode::S,      KeyCode::T,         KeyCode::U,     KeyCode::V,     KeyCode::W,       KeyCode::X,
    KeyCode::Y,      KeyCode::Z,         KeyCode::Num0,  KeyCode::Num1,  KeyCode::Num2,    KeyCode::Num3,
    KeyCode::Num4,   KeyCode::Num5,      KeyCode::Num6,  KeyCode::Num7,  KeyCode::Num8,    KeyCode::Num9,
    KeyCode::Left,   KeyCode::Up,        KeyCode::Right, KeyCode::Down,  KeyCode::F1,      KeyCode::F2,
    KeyCode::F3,     KeyCode::F4,        KeyCode::F5,    KeyCode::F6,    KeyCode::F7,      KeyCode::F8,
    KeyCode::F9,     KeyCode::F10,       KeyCode::F11,   KeyCode::F12,   KeyCode::Space,   KeyCode::Enter,
    KeyCode::Escape, KeyCode::Backspace, KeyCode::Tab,   KeyCode::Shift, KeyCode::Control, KeyCode::Alt,
};
#endif