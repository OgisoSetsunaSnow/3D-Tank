#pragma once

#include <Platform.h>

#if VTS_PLATFORM_WINDOWS
#include <string>
#include <Windows.h>
#include <memory>
#include <functional>

/**
* Encapsulate a window class.
*
* Calls <c>::RegisterClass()</c> on create, and <c>::UnregisterClass()</c>
* on destruction.
*/
struct WindowClass final
{
public:
	WindowClass (const std::string& name,
		::WNDPROC procedure = ::DefWindowProc);
	~WindowClass ();

	const std::string& GetName () const;

	WindowClass (const WindowClass&) = delete;
	WindowClass& operator= (const WindowClass&) = delete;

private:
	std::string name_;
};

struct IWindow
{
public:
	using MessageHandler = std::function<bool(HWND, UINT, WPARAM, LPARAM)>;

	IWindow () = default;
	IWindow (const IWindow&) = delete;
	IWindow& operator=(const IWindow&) = delete;

	virtual ~IWindow ();

	bool IsClosed () const;
	virtual void OnClose () = 0;

	int GetWidth () const;
	int GetHeight () const;

	void setMessageHandler(MessageHandler handler)
	{
		m_messageHandler = handler;
	}

	bool handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (m_messageHandler)
		{
			if (m_messageHandler(hwnd, msg, wParam, lParam))
				return true;
		}
		switch (msg)
		{
		case WM_CLOSE:
			OnClose();
			return true;
		}
		return false;
	}
    virtual void PumpEvents() = 0;

private:
	virtual bool IsClosedImpl () const = 0;
	virtual int GetWidthImpl () const = 0;
	virtual int GetHeightImpl () const = 0;

	MessageHandler m_messageHandler;
};

class Window : public IWindow
{
public:
	Window (const std::string& title, int width, int height);

	HWND GetHWND () const;

	void OnClose () override
	{
		isClosed_ = true;
	}

    bool processMessages();
    void PumpEvents() override;

private:
	bool IsClosedImpl () const override
	{
		return isClosed_;
	}

	int GetWidthImpl () const override
	{
		return width_;
	}

	int GetHeightImpl () const override
	{
		return height_;
	}
	std::unique_ptr<WindowClass> windowClass_;
	HWND hwnd_ = 0;
	bool isClosed_ = false;
	int width_ = -1, height_ = -1;
};
#endif