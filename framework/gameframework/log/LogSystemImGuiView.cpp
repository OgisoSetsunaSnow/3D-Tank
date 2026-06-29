#include "gameframework/log/LogSystemImGuiView.h"

#include <imgui.h>

void LogSystemImGuiView::draw()
{
    if (!m_isOpen)
        return;
    ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Log System", &m_isOpen);
    drawToolbar();
    ImGui::Separator();
    drawChannelTabs();
    ImGui::End();
}

void LogSystemImGuiView::drawToolbar()
{
    drawToggleButton(" INFO ", m_showInfo, ImVec4(0.9f, 0.9f, 0.9f, 1.f), ImVec4(0.35f, 0.35f, 0.35f, 1.f));
    ImGui::SameLine();

    drawToggleButton(" WARNING ", m_showWarning, ImVec4(0.8f, 0.65f, 0.0f, 1.f), ImVec4(0.35f, 0.30f, 0.05f, 1.f));
    ImGui::SameLine();

    drawToggleButton(" ERROR ", m_showError, ImVec4(0.8f, 0.2f, 0.2f, 1.f), ImVec4(0.35f, 0.10f, 0.10f, 1.f));
    ImGui::SameLine();

    // Pause 按钮通过 LogSystem 接口读写数据
    bool enabled = LogSystem::sGetInstance().isEnabled();
    const char* pauseLabel = enabled ? " Pause " : " Paused ";
    if (ImGui::Button(pauseLabel, ImVec2(80, 0)))
        LogSystem::sGetInstance().setEnable(!enabled);
}

void LogSystemImGuiView::drawChannelTabs()
{
    if (!ImGui::BeginTabBar("ChannelTabs"))
        return;

    if (ImGui::BeginTabItem("All"))
    {
        drawChannelEntries(nullptr);
        ImGui::EndTabItem();
    }

    const auto& m_channels = LogSystem::sGetInstance().getChannels();
    for (auto& m_channel : m_channels)
    {

        const std::string tabLabel = m_channel.first;

        if (ImGui::BeginTabItem(tabLabel.c_str()))
        {
            drawChannelEntries(&m_channel.second);
            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
}

void LogSystemImGuiView::drawChannelEntries(const LogChannel* channel)
{
    ImGui::BeginChild("LogScrollArea", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (channel == nullptr)
    {
        // 全局时间顺序
        for (const LogEntry& entry : LogSystem::sGetInstance().getAllEntries())
            drawSingleEntry(entry);
    }
    else
    {
        for (const LogEntry& entry : channel->entries)
            drawSingleEntry(entry);
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}

void LogSystemImGuiView::drawSingleEntry(const LogEntry& entry)
{
    if (entry.level == LogLevel::LOG_INFO && !m_showInfo)
        return;
    if (entry.level == LogLevel::LOG_WARNING && !m_showWarning)
        return;
    if (entry.level == LogLevel::LOG_ERROR && !m_showError)
        return;

    const std::string displayText = "[" + entry.timestamp + "] " + entry.message;

    if (entry.level == LogLevel::LOG_INFO)
    {
        ImGui::TextUnformatted(displayText.c_str());
    }
    else if (entry.level == LogLevel::LOG_WARNING)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.0f, 1.0f));
        ImGui::TextUnformatted(displayText.c_str());
        ImGui::PopStyleColor();
    }
    else if (entry.level == LogLevel::LOG_ERROR)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextUnformatted(displayText.c_str());
        ImGui::PopStyleColor();
    }
}

void LogSystemImGuiView::drawToggleButton(const char* label, bool& isActive, ImVec4 colorOn, ImVec4 colorOff)
{
    const ImVec4 cur = isActive ? colorOn : colorOff;
    const ImVec4 hov = ImVec4(cur.x + 0.1f, cur.y + 0.1f, cur.z + 0.1f, 1.f);
    const ImVec4 act = ImVec4(cur.x - 0.1f, cur.y - 0.1f, cur.z - 0.1f, 1.f);

    ImGui::PushStyleColor(ImGuiCol_Button, cur);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, act);

    if (ImGui::Button(label, ImVec2(80, 0)))
        isActive = !isActive;

    ImGui::PopStyleColor(3);
}