#ifndef UNHIDENPCS_LOGGER_HPP
#define UNHIDENPCS_LOGGER_HPP
#pragma once
#include "../pch.hpp"
#include "util.hpp"

namespace logging
{
    enum class LogLevel
    {
        Debug,
        Info,
        Warning,
        Error
    };

    const char* logLevelToString(LogLevel level);

    struct LogEntry
    {
        std::string timestamp {};
        LogLevel    level {};
        std::string message {};
    };

    using LogCallback = std::function<void(const LogEntry& entry)>;

    class Logger
    {
    private:
        static Logger* _instance;

        std::string           _name {};
        std::filesystem::path _path {};
        std::ofstream         _file {};
        LogLevel              _level = LogLevel::Debug;

        HANDLE _hConsole {};
        bool   _consoleCreated {};

        std::deque<LogEntry>     _recentEntries {};
        std::vector<LogCallback> _callbacks {};

        std::mutex _mutex {};

    public:
        static Logger* instance();

        explicit Logger(
            const std::string&           name,
            const std::filesystem::path& path,
            LogLevel                     level   = LogLevel::Debug,
            bool                         console = false
        );

        ~Logger();

        void setLevel(LogLevel level);

        [[nodiscard]] LogLevel level() const;

        void registerCallback(const LogCallback& callback);

        void unregisterCallback(const LogCallback& callback);

        template <typename... Args>
        void log(LogLevel level, const std::string& format, Args&&... args);

        bool setConsole(bool value);

    private:
        void runCallbacks(const LogEntry& entry) const;

        void printLogEntry(const LogEntry& entry, bool toFile = true);
    };

    template <typename... Args>
    void Logger::log(LogLevel level, const std::string& format, Args&&... args)
    {
        if (level < _level)
            return;

        const auto now  = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        const auto ms   = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm tm {};
        localtime_s(&tm, &time);

        const auto  timestamp = fmt::format("[{:02}:{:02}:{:02}:{:03}]", tm.tm_hour, tm.tm_min, tm.tm_sec, ms.count());
        std::string message {};

        try
        {
            message = fmt::format(format, std::forward<Args>(args)...);
        }
        catch (const std::exception& e)
        {
            message = "Error formatting log message \"" + format + "\": " + e.what();
            level   = LogLevel::Error;
        }

        std::lock_guard lock(_mutex);

        if (_recentEntries.size() >= 250)
            _recentEntries.pop_front();
        _recentEntries.push_back({ timestamp, level, message });

        const auto& entry = _recentEntries.back();

        printLogEntry(entry);
        runCallbacks(entry);
    }
}

#define LOG_DBG(fmt, ...) if (logging::Logger::instance()) logging::Logger::instance()->log(logging::LogLevel::Debug, "[{}:{}\t{}()]\t" fmt, util::getFileName(__FILE__), __LINE__, __func__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) if (logging::Logger::instance()) logging::Logger::instance()->log(logging::LogLevel::Info, " " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) if (logging::Logger::instance()) logging::Logger::instance()->log(logging::LogLevel::Warning, " " fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) if (logging::Logger::instance()) logging::Logger::instance()->log(logging::LogLevel::Error, "[{}:{}\t{}()]\t" fmt, util::getFileName(__FILE__), __LINE__, __func__, ##__VA_ARGS__)

#endif //UNHIDENPCS_LOGGER_HPP
