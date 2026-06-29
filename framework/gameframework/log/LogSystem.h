#pragma once
#include <chrono>
#include <cstdio>
#include <ctime>
#include <map>
#include <string>
#include <vector>


enum class LogLevel
{
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

struct LogEntry
{
    LogLevel level;
    std::string message;
    std::string timestamp;
};

struct LogChannel
{
    std::string name;
    std::vector<LogEntry> entries;
};

class LogSystem
{
public:
    static LogSystem& sGetInstance();

    void addLog(const std::string& channelName, LogLevel level, const std::string& message);

    void setEnable(bool enable);
    bool isEnabled() const;

    void setMaxLogsPerFrame(int max) { m_maxLogsPerFrame = max; }

    const std::vector<LogEntry>& getAllEntries() const { return m_allEntries; }
    const std::map<std::string, LogChannel>& getChannels() const { return m_channels; }

private:
    LogSystem() = default;

    static std::string sGetTimestamp();

    std::map<std::string, LogChannel> m_channels;
    std::vector<LogEntry> m_allEntries;

    bool m_isEnabled = true;

    int m_frameLogCount = 0;
    int m_maxLogsPerFrame = 10;
    std::chrono::steady_clock::time_point m_lastResetTime = std::chrono::steady_clock::now();
};

#define LOG_INFO(channel, fmt, ...)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        char _b[512];                                                                                                  \
        std::snprintf(_b, 512, fmt, ##__VA_ARGS__);                                                                    \
        LogSystem::sGetInstance().addLog(channel, LogLevel::LOG_INFO, _b);                                             \
    } while (0)

#define LOG_WARN(channel, fmt, ...)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        char _b[512];                                                                                                  \
        std::snprintf(_b, 512, fmt, ##__VA_ARGS__);                                                                    \
        LogSystem::sGetInstance().addLog(channel, LogLevel::LOG_WARNING, _b);                                          \
    } while (0)

#define LOG_ERROR(channel, fmt, ...)                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        char _b[512];                                                                                                  \
        std::snprintf(_b, 512, fmt, ##__VA_ARGS__);                                                                    \
        LogSystem::sGetInstance().addLog(channel, LogLevel::LOG_ERROR, _b);                                            \
    } while (0)