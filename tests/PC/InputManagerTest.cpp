#include "framework/game/InputManager/InputManager.h"
#include "framework/include/Common.h"
#include <Windows.h>
#include <cstdio>

/**
 * @brief 输入管理器功能测试
 * 监控键盘(W/A/S/D)和鼠标输入状态，并显示到控制台
 */
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nShowCmd);

	// 分配控制台窗口
	if (!AllocConsole())
	{
		MessageBoxA(NULL, "Failed to allocate console", "Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	// 配置控制台IO
	FILE* fpIn, * fpOut;
	freopen_s(&fpIn, "CONIN$", "r", stdin);
	freopen_s(&fpOut, "CONOUT$", "w", stdout);
	freopen_s(&fpOut, "CONOUT$", "w", stderr);
	SetConsoleOutputCP(CP_UTF8);

	// 显示控制台窗口
	HWND consoleWindow = GetConsoleWindow();
	ShowWindow(consoleWindow, SW_SHOW);
	SetForegroundWindow(consoleWindow);

	printf("=== InputManager Test ===\n");
	printf("Press ESC to exit\n");
	printf("Test: W/A/S/D keys and mouse buttons\n\n");
	fflush(stdout);

	// 获取输入管理器单例
	auto& input = InputManager::GetInstance();
	
	// 记录前一帧的状态用于检测状态变化
	bool prevW = false, prevA = false, prevS = false, prevD = false;
	bool prevLeft = false, prevRight = false;
	int frameCounter = 0;

	// 主测试循环
	while (true)
	{
		// 每帧更新输入状态
		input.update();

		// 检查ESC键以退出
		if (input.isKeyJustPressed(KeyCode::Escape))
		{
			printf("\n[ESC pressed] Test ended\n");
			fflush(stdout);
			break;
		}

		// 获取当前帧的输入状态
		bool wPressed = input.isKeyPressed(KeyCode::W);
		bool aPressed = input.isKeyPressed(KeyCode::A);
		bool sPressed = input.isKeyPressed(KeyCode::S);
		bool dPressed = input.isKeyPressed(KeyCode::D);
		bool leftPressed = input.isMouseButtonPressed(MouseButton::Left);
		bool rightPressed = input.isMouseButtonPressed(MouseButton::Right);

		// 检测状态变化并打印
		if (wPressed != prevW)
		{
			printf("[W] %s\n", wPressed ? "PRESSED" : "RELEASED");
			fflush(stdout);
		}
		if (aPressed != prevA)
		{
			printf("[A] %s\n", aPressed ? "PRESSED" : "RELEASED");
			fflush(stdout);
		}
		if (sPressed != prevS)
		{
			printf("[S] %s\n", sPressed ? "PRESSED" : "RELEASED");
			fflush(stdout);
		}
		if (dPressed != prevD)
		{
			printf("[D] %s\n", dPressed ? "PRESSED" : "RELEASED");
			fflush(stdout);
		}
		if (leftPressed != prevLeft)
		{
			printf("[Mouse Left] %s\n", leftPressed ? "PRESSED" : "RELEASED");
			fflush(stdout);
		}
		if (rightPressed != prevRight)
		{
			printf("[Mouse Right] %s\n", rightPressed ? "PRESSED" : "RELEASED");
			fflush(stdout);
		}

		// 每30帧打印一次完整状态信息
		frameCounter++;
		if (frameCounter % 30 == 0)
		{
			printf("\n--- Frame %d Status ---\n", frameCounter);
			printf("Keys: W=%s A=%s S=%s D=%s\n",
				wPressed ? "DOWN" : "UP",
				aPressed ? "DOWN" : "UP",
				sPressed ? "DOWN" : "UP",
				dPressed ? "DOWN" : "UP");
			printf("Mouse: Left=%s Right=%s\n",
				leftPressed ? "DOWN" : "UP",
				rightPressed ? "DOWN" : "UP");

#ifdef VTS_PLATFORM_WINDOWS
			auto mousePos = input.getMousePosition();
			printf("Mouse Position: (%.0f, %.0f)\n\n", mousePos.x, mousePos.y);
#else
			auto mousePos = input.getMousePosition();
			printf("Mouse Position: (%.0f, %.0f)\n\n", mousePos.x, mousePos.y);
#endif
			fflush(stdout);
		}

		// 保存当前帧状态
		prevW = wPressed;
		prevA = aPressed;
		prevS = sPressed;
		prevD = dPressed;
		prevLeft = leftPressed;
		prevRight = rightPressed;

		// 帧延迟（模拟60FPS）
		Sleep(16);
	}

	printf("Test completed successfully\n");
	fflush(stdout);
	return 0;
}