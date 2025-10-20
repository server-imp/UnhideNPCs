#include "logger.hpp"

logging::Logger* logging::Logger::_instance = nullptr;

const char* logging::logLevelToString(const LogLevel level)
{
    switch (level)
    {
        case LogLevel::Debug:
            return " DBG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return " ERR";
        default:
            return " UNK";
    }
}

logging::Logger* logging::Logger::instance()
{
    return _instance;
}

logging::Logger::Logger
(const std::string& name, const std::filesystem::path& path, const LogLevel level, const bool console)
{
    _name = name;
    _path = path;

    if (!std::filesystem::exists(path.parent_path()))
        std::filesystem::create_directories(path.parent_path());

    _file.open(_path, std::ios::trunc);
    _level    = level;
    _instance = this;

    if (console)
    {
        setConsole(true);
    }

    LOG_DBG("Initialized");
}

logging::Logger::~Logger()
{
    setConsole(false);

    _file.close();
}

void logging::Logger::setLevel(const LogLevel level)
{
    this->_level = level;
}

logging::LogLevel logging::Logger::level() const
{
    return this->_level;
}

void logging::Logger::registerCallback(const LogCallback& callback)
{
    _callbacks.push_back(callback);
}

bool logging::Logger::setConsole(const bool value)
{
    if (value && _hConsole)
        return true;
    if (!value && !_hConsole)
        return true;

    if (value)
    {
        if (!_consoleCreated)
        {
            _consoleCreated = AllocConsole();
            if (_consoleCreated)
            {
                SetConsoleOutputCP(CP_UTF8);
                SetConsoleTitleA(_name.c_str());
            }
        }

        _hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (_hConsole)
        {
            for (const auto& entry : _recentEntries)
            {
                printLogEntry(entry, false);
            }
        }

        return _hConsole;
    }

    if (_consoleCreated)
    {
        _consoleCreated = !FreeConsole();
    }

    _hConsole = nullptr;
    return true;
}

void logging::Logger::runCallbacks(const LogLevel level, const std::string& message) const
{
    for (const auto& callback : _callbacks)
    {
        callback(level, message);
    }
}

void logging::Logger::printLogEntry(const LogEntry& entry, const bool toFile)
{
    const auto formatted = fmt::format("{} [{}] {}\n", entry.timestamp, logLevelToString(entry.level), entry.message);

    if (toFile)
    {
        _file << formatted.c_str();
        _file.flush();
    }

    WriteConsole(_hConsole, formatted.c_str(), formatted.size(), nullptr, nullptr);
}
