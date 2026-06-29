#pragma once

#include <cstdint>

/**
 * @file InputAction.h
 * @brief  输入映射层的核心枚举
 * 
 * 这个文件定义了游戏逻辑层使用的抽象输入概念：
 *  GameAction: 离散的游戏动作（按下/释放）
 *  GameAxis: 连续的游戏轴（-1.0 到 +1.0）
 * 
 * 游戏逻辑只需要关心这些抽象概念，不需要知道具体按键
 */

/**
 * @brief 游戏动作枚举
 * 
 * 代表按下或者未按下
 * 每个动作可以绑定到多个按键或按钮
 * exp：
 *   GameAction::MoveForward 可以绑定到 KeyCode::W 或 Gamepad::DPadUp
 */
enum class GameAction : uint8_t
{
	// 移动控制
	MoveForward,      // 向前
	MoveBackward,     // 向后
	TurnLeft,         // 向左
	TurnRight,        ///向右
	Jump,

	// 战斗控制
	Fire,             // 开火
	AimMode,          // 瞄准
	Reload,           // 重新装填

	// 相机控制
	CameraZoomIn,     // 相机拉近
	CameraZoomOut,    //相机拉远
	CameraRotateLeft, //相机左旋
	CameraRotateRight,//相机右旋

	// UI和系统
	Pause,            //暂停游戏
	Menu,             //打开菜单
	Quit,             //退出游戏
	Interact,         // 交互
	
	// 特殊功能
	ToggleDebugView,  // 切换调试视图
	Screenshot,       // 截图

	// 总数标记
	COUNT             //动作总数来确定数组的大小


};

/**
 * @brief 游戏轴枚举
 * 
 * 连续的玩家输入
 * 轴可以组合多个按键或使用模拟输入  可以适用于手柄
 * 
 * 示例：
 *   GameAxis::MoveVertical:
 *     W键按下: +1.0
 *     S键按下: -1.0
 *     同时按下或都不按: 0.0
 *     手柄左摇杆Y轴: -1.0 到 +1.0
 */
enum class GameAxis : uint8_t
{
	// 移动轴
	MoveVertical,     // 前后移动轴（-1=后退, +1=前进）
	MoveHorizontal,   // 左右移动轴（-1=左移, +1=右移）[坦克通常只转向]
	
	// 转向轴
	TurnHorizontal,   // 转向轴（-1=左转, +1=右转）

	// 瞄准轴（用于炮塔控制）
	AimHorizontal,    // 水平瞄准轴（-1=左, +1=右）
	AimVertical,      // 垂直瞄准轴（-1=下, +1=上）
	// 相机轴
	CameraHorizontal, // 相机水平旋转轴
	CameraVertical,   // 相机垂直旋转轴
	CameraZoom,       // 相机缩放轴（-1=拉远, +1=拉近）

	// 总数标记
	COUNT ,            // 轴总数用于确定数组大小
};

/**
 * @brief 将 GameAction 转换为字符串（用于调试和配置）
 */
inline const char* gameActionToString(GameAction action)
{
	switch (action)
	{
		case GameAction::MoveForward: return "MoveForward";
		case GameAction::MoveBackward: return "MoveBackward";
		case GameAction::TurnLeft: return "TurnLeft";
		case GameAction::TurnRight: return "TurnRight";
		case GameAction::Jump: return "Jump";
		case GameAction::Fire: return "Fire";
		case GameAction::AimMode: return "AimMode";
		case GameAction::Reload: return "Reload";
		case GameAction::CameraZoomIn: return "CameraZoomIn";
		case GameAction::CameraZoomOut: return "CameraZoomOut";
		case GameAction::CameraRotateLeft: return "CameraRotateLeft";
		case GameAction::CameraRotateRight: return "CameraRotateRight";
		case GameAction::Pause: return "Pause";
		case GameAction::Menu: return "Menu";
		case GameAction::Quit: return "Quit";
		case GameAction::Interact: return "Interact";
		case GameAction::ToggleDebugView: return "ToggleDebugView";
		case GameAction::Screenshot: return "Screenshot";
		default: return "Unknown";
	}
}

/**
 * @brief 将 GameAxis 转换为字符串（用于调试和配置）
 */
inline const char* gameAxisToString(GameAxis axis)
{
	switch (axis)
	{
		case GameAxis::MoveVertical: return "MoveVertical";
		case GameAxis::MoveHorizontal: return "MoveHorizontal";
		case GameAxis::TurnHorizontal: return "TurnHorizontal";
		case GameAxis::AimHorizontal: return "AimHorizontal";
		case GameAxis::AimVertical: return "AimVertical";
		case GameAxis::CameraHorizontal: return "CameraHorizontal";
		case GameAxis::CameraVertical: return "CameraVertical";
		case GameAxis::CameraZoom: return "CameraZoom";
		default: return "Unknown";
	}
}
