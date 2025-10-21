#ifndef UNHIDENPCS_UTIL_HPP
#define UNHIDENPCS_UTIL_HPP
#pragma once
#include "pch.hpp"

namespace util
{
    void ltrim(std::string& s);

    void rtrim(std::string& s);

    void trim(std::string& s);

    void replace(std::string& str, const std::string& a, const std::string& b);

    bool strtob(const std::string& value);

    std::vector<std::string> readLines(const std::filesystem::path& filePath);

    std::string tolower(std::string s);

    bool empty_or_whitespace(const std::string& s);

    bool getModuleFilePath(HMODULE hModule, std::filesystem::path& path);

    bool isModuleInDir(HMODULE hModule, const std::filesystem::path& directory);

    bool isModuleInExeDir(HMODULE hModule);

    std::string getModuleFileName(HMODULE hModule);

    bool equalsIgnoreCase(const std::string& a, const std::string& b);

    bool closeHandle(HANDLE& hObject);

    bool freeLibrary(HMODULE& hModule);

    bool checkMutex(const char* name, HANDLE& hMutex);

    std::string wstringToString(const std::wstring& wstring);

    bool shmExists(const std::string& name);

    void fmt_msgbox(HWND hWnd, const char* caption, UINT uType, const char* fmt, ...);

    void dbgbox(const char* fmt, ...);

    constexpr const char* getFileName(const char* path)
    {
        const char* last_slash = path;
        for (std::size_t i = 0; path[i] != '\0'; ++i)
        {
            if (path[i] == '/' || path[i] == '\\')
            {
                last_slash = path + i + 1;
            }
        }
        return last_slash;
    }
}

#endif //UNHIDENPCS_UTIL_HPP
