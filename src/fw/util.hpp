#ifndef UNHIDENPCS_UTIL_HPP
#define UNHIDENPCS_UTIL_HPP
#pragma once
#include "pch.hpp"
#include "memory/handle.hpp"

/// Evaluates to true only once per call site.
#define TRIGGER_ONCE []() -> bool { static bool once = false; if (once) return false; once = true; return true; }()

/// Evaluates to true every `ms` milliseconds.
/// The first call returns true immediately.
#define TRIGGER_EVERY_MS_IMMEDIATE(ms) \
([]() -> bool { \
static std::atomic<ULONGLONG> last{0}; \
ULONGLONG now = GetTickCount64(); \
ULONGLONG prev = last.load(std::memory_order_relaxed); \
if (now - prev >= (ms)) { \
if (last.compare_exchange_strong(prev, now, std::memory_order_relaxed)) { \
return true; \
} \
} \
return false; \
}())

/// Evaluates to true every `ms` milliseconds.
/// The first call waits until the interval has elapsed.
#define TRIGGER_EVERY_MS(ms) \
([]() -> bool { \
static std::atomic<ULONGLONG> last{GetTickCount64()}; \
ULONGLONG now = GetTickCount64(); \
ULONGLONG prev = last.load(std::memory_order_relaxed); \
if (now - prev >= (ms)) { \
if (last.compare_exchange_strong(prev, now, std::memory_order_relaxed)) { \
return true; \
} \
} \
return false; \
}())

namespace util
{
    void ltrim(std::string& s);

    void rtrim(std::string& s);

    void trim(std::string& s);

    std::string trim(const std::string& s);

    void replace(std::string& str, const std::string& a, const std::string& b);

    bool strtob(const std::string& value, bool defaultValue);

    std::vector<std::string> readLines(const std::filesystem::path& filePath);

    std::string tolower(std::string s);

    bool emptyOrWhitespace(const std::string& s);

    bool getModuleFilePath(HMODULE hModule, std::filesystem::path& path);

    bool isModuleInDir(HMODULE hModule, const std::filesystem::path& directory);

    bool isModuleInExeDir(HMODULE hModule);

    std::string getModuleFileName(HMODULE hModule);

    bool equalsIgnoreCase(const std::string& a, const std::string& b);

    bool equalsIgnoreCase(const std::wstring& a, const std::wstring& b);

    bool closeHandle(HANDLE& hObject);

    bool freeLibrary(HMODULE& hModule);

    bool checkMutex(const char* name, HANDLE& hMutex);

    std::string wstringToString(const std::wstring& wstring);

    bool shmExists(const std::string& name);

    void fmtMsgBox(HWND hWnd, const char* caption, UINT uType, const char* fmt, ...);

    void dbgbox(const char* fmt, ...);

    std::vector<std::string> getStartupArgs();

    std::string getStartupArgValue(const std::string& argName);

    bool isModuleInAnyDirsRelativeToExe(HMODULE hModule, const std::initializer_list<std::string>& relativeDirs);

    constexpr const char* getFileName(const char* path)
    {
        const char* lastSlash = path;
        for (std::size_t i = 0; path[i] != '\0'; ++i)
        {
            if (path[i] == '/' || path[i] == '\\')
            {
                lastSlash = path + i + 1;
            }
        }
        return lastSlash;
    }

    memory::Handle getVirtualFunctionAddress(void* object, std::size_t offset);

    bool isValidGuildWars2Name(const wchar_t* name);
}

#endif //UNHIDENPCS_UTIL_HPP
