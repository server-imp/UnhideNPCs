#include "util.hpp"

#include <algorithm>
#include <cwctype>

#include "logger.hpp"
#include <shellapi.h>

void util::ltrim(std::string& s)
{
    s.erase(
        s.begin(),
        std::find_if(
            s.begin(),
            s.end(),
            [](const unsigned char ch)
            {
                return !::isspace(ch);
            }
        )
    );
}

void util::rtrim(std::string& s)
{
    s.erase(
        std::find_if(
            s.rbegin(),
            s.rend(),
            [](const unsigned char ch)
            {
                return !::isspace(ch);
            }
        ).base(),
        s.end()
    );
}

void util::trim(std::string& s)
{
    rtrim(s);
    ltrim(s);
}

std::string util::trim(const std::string& s)
{
    std::string copy(s);
    trim(copy);
    return copy;
}

void util::replace(std::string& str, const std::string& a, const std::string& b)
{
    if (a.empty())
        return;

    size_t start_pos = 0;
    while ((start_pos = str.find(a, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, a.length(), b);
        start_pos += b.length();
    }
}

bool util::strtob(const std::string& value, const bool defaultValue = false)
{
    const std::string text = tolower(trim(value));

    if (text == "true" || text == "1")
        return true;
    if (text == "false" || text == "0")
        return false;

    return defaultValue;
}

std::vector<std::string> util::readLines(const std::filesystem::path& filePath)
{
    if (std::filesystem::exists(filePath))
    {
        std::ifstream            file(filePath);
        std::vector<std::string> lines;
        std::string              line;

        while (std::getline(file, line))
        {
            lines.push_back(line);
        }

        return lines;
    }
    return {};
}

std::string util::tolower(std::string s)
{
    std::transform(
        s.begin(),
        s.end(),
        s.begin(),
        [](const unsigned char c) -> char
        {
            return static_cast<char>(std::tolower(c));
        }
    );

    return s;
}

bool util::empty_or_whitespace(const std::string& s)
{
    if (s.empty())
        return true;

    return std::all_of(s.begin(), s.end(), isspace);
}

bool util::getModuleFilePath(const HMODULE hModule, std::filesystem::path& path)
{
    char dllPath[MAX_PATH] {};
    if (!GetModuleFileName(hModule, dllPath, MAX_PATH))
    {
        LOG_DBG("GetModuleFileName failed: {:08X}", GetLastError());
        return false;
    }

    path = std::filesystem::path(dllPath);
    return true;
}

bool util::isModuleInDir(const HMODULE hModule, const std::filesystem::path& directory)
{
    if (!std::filesystem::is_directory(directory) || !std::filesystem::exists(directory))
    {
        LOG_DBG("\"{}\" is not a directory", directory.string());
        return false;
    }

    std::filesystem::path dllPath;
    if (!getModuleFilePath(hModule, dllPath))
        return false;

    return dllPath.parent_path() == directory;
}

bool util::isModuleInExeDir(const HMODULE hModule)
{
    if (!hModule)
        return true;

    std::filesystem::path exePath;
    if (!getModuleFilePath(nullptr, exePath))
        return false;

    return isModuleInDir(hModule, exePath.parent_path());
}

std::string util::getModuleFileName(const HMODULE hModule)
{
    std::filesystem::path dllPath;
    if (!getModuleFilePath(hModule, dllPath))
        return "";

    return dllPath.filename().string();
}

bool util::equalsIgnoreCase(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;

    return std::equal(
        a.begin(),
        a.end(),
        b.begin(),
        [](const char c1, const char c2)
        {
            return std::tolower(c1) == std::tolower(c2);
        }
    );
}

bool util::equalsIgnoreCase(const std::wstring& a, const std::wstring& b)
{
    if (a.size() != b.size())
        return false;

    return std::equal(
        a.begin(),
        a.end(),
        b.begin(),
        [](const wchar_t c1, const wchar_t c2)
        {
            return std::towlower(c1) == std::towlower(c2);
        }
    );
}

bool util::closeHandle(HANDLE& hObject)
{
    if (!hObject)
        return true;

    if (!CloseHandle(hObject))
    {
        return false;
    }

    hObject = nullptr;
    return true;
}

bool util::freeLibrary(HMODULE& hModule)
{
    if (!hModule)
        return true;

    if (!FreeLibrary(hModule))
    {
        return false;
    }

    hModule = nullptr;
    return true;
}

bool util::checkMutex(const char* name, HANDLE& hMutex)
{
    hMutex = CreateMutexA(nullptr, FALSE, name);
    if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        closeHandle(hMutex);
        return false;
    }

    return true;
}

std::string util::wstringToString(const std::wstring& wstring)
{
    if (wstring.empty())
        return {};
    const int   size = WideCharToMultiByte(CP_UTF8, 0, wstring.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstring.c_str(), -1, out.data(), size, nullptr, nullptr);
    return out;
}

bool util::shmExists(const std::string& name)
{
    if (name.empty())
        return false;

    if (const auto h = OpenFileMappingA(FILE_MAP_READ, FALSE, name.c_str()))
    {
        CloseHandle(h);
        return true;
    }

    return false;
}

void util::fmt_msgbox(HWND hWnd, const char* caption, const UINT uType, const char* fmt, ...)
{
    char    buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    MessageBoxA(hWnd, buffer, caption, uType);
}

void util::dbgbox(const char* fmt, ...)
{
    char    buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    MessageBoxA(nullptr, buffer, "Dbg", MB_OK);
}

std::vector<std::string> util::getStartupArgs()
{
    auto    argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::vector<std::string> args;

    if (argv)
    {
        if (argc > 1)
        {
            args.reserve(argc - 1);

            for (auto i = 1; i < argc; ++i)
            {
                const int size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);

                std::string arg(size - 1, '\0');

                WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, arg.data(), size, nullptr, nullptr);

                args.emplace_back(std::move(arg));
            }
        }

        LocalFree(argv);
    }

    return args;
}

std::string util::getStartupArgValue(const std::string& argName)
{
    static auto args = getStartupArgs();

    // iterate over the args until we find the argName, then check if we have a value after it and return it
    for (size_t i = 0; i < args.size(); ++i)
    {
        if (equalsIgnoreCase(args[i], argName) && i + 1 < args.size())
        {
            return args[i + 1];
        }
    }

    return {};
}

bool util::isModuleInAnyDirsRelativeToExe(const HMODULE hModule, const std::initializer_list<std::string>& relativeDirs)
{
    std::filesystem::path path;
    if (!getModuleFilePath(nullptr, path))
        return false;
    path = path.parent_path();

    return std::any_of(
        relativeDirs.begin(),
        relativeDirs.end(),
        [&](const std::string& dir)
        {
            return isModuleInDir(hModule, dir.empty() ? path : path / dir);
        }
    );
}
