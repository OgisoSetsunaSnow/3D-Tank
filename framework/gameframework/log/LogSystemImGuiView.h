#pragma once
#include "gameframework/log/LogSystem.h"

struct ImVec4;

class LogSystemImGuiView
{
public:
    // 对外接口，每帧调用一次
    void draw();

private:
    void drawToolbar();
    void drawChannelTabs();
    void drawChannelEntries(const LogChannel* channel);
    void drawSingleEntry(const LogEntry& entry);
    void drawToggleButton(const char* label, bool& isActive, ImVec4 colorOn, ImVec4 colorOff);

    bool m_showInfo = true;
    bool m_showWarning = true;
    bool m_showError = true;
    bool m_isOpen = true;
};