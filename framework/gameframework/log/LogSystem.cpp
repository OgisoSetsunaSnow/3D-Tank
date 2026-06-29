#include "gameframework/log/LogSystem.h"

#include <iomanip>
#include <sstream>

#include <Platform.h>


LogSystem& LogSystem::sGetInstance()
{
    static LogSystem instance;
    return instance;
}

std::string LogSystem::sGetTimestamp()
{
    time_t now = time(nullptr);
    tm localTime{};

#if VTS_PLATFORM_WINDOWS
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << localTime.tm_hour << ":" << std::setw(2) << localTime.tm_min << ":"
        << std::setw(2) << localTime.tm_sec;

    return oss.str();
}

void LogSystem::addLog(const std::string& channelName, LogLevel level, const std::string& message)
{
    if (!m_isEnabled)
        return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastResetTime).count();
    if (elapsed >= 16)
    {
        m_frameLogCount = 0;
        m_lastResetTime = now;
    }

    if (m_frameLogCount >= m_maxLogsPerFrame)
        return;
    m_frameLogCount++;

    std::string prefix;
    if (level == LogLevel::LOG_INFO)
        prefix = "[INFO]  ";
    else if (level == LogLevel::LOG_WARNING)
        prefix = "[WARNING]  ";
    else if (level == LogLevel::LOG_ERROR)
        prefix = "[ERROR] ";

    LogEntry entry{level, prefix + message, sGetTimestamp()};

    LogChannel& channel = m_channels[channelName];
    channel.name = channelName;
    channel.entries.push_back(entry);
    m_allEntries.push_back(entry);
}

void LogSystem::setEnable(bool Enable)
{
    m_isEnabled = Enable;
}

bool LogSystem::isEnabled() const
{
    return m_isEnabled;
}
