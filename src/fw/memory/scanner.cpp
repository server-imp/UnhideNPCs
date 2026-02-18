#include "scanner.hpp"

#include "pattern.hpp"
#include "../logger.hpp"
#include "fw/util.hpp"

bool memory::Scanner::findPattern(const std::string& pattern, const Range& range, Handle& result)
{
    LOG_DBG(
        "Looking for \"{}\" in range {:X}-{:X} [{:04X}]",
        pattern,
        range.start().raw(),
        range.end().raw(),
        range.size()
    );
    const auto  parsed = memory::Pattern(pattern);
    const auto& data   = parsed.data();
    const auto& mask   = parsed.mask();

    const size_t patternSize = data.size();
    const size_t rangeSize   = range.size();

    if (patternSize == 0 || rangeSize < patternSize)
    {
        LOG_DBG("Invalid pattern");
        return false;
    }

    const uint8_t* start  = range.start().to_ptr<uint8_t*>();
    const size_t   stopAt = rangeSize - patternSize;

    for (size_t i = 0; i <= stopAt; ++i)
    {
        if (mask[0] && start[i] != data[0])
        {
            continue;
        }

        bool match = true;
        for (size_t j = 1; j < patternSize; ++j)
        {
            if (mask[j] && start[i + j] != data[j])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            result = Handle(reinterpret_cast<uintptr_t>(start) + i);
            LOG_DBG("Found pattern at {:08X}", result.raw());
            return true;
        }
    }

    LOG_DBG("Could not find pattern");
    return false;
}

bool memory::Scanner::findString(const std::string& string, const Range& range, Handle& result)
{
    LOG_DBG("Looking for {}", string);
    if (string.empty())
    {
        return false;
    }

    const char*  start = range.start().to_ptr<char*>();
    const size_t n     = range.size();
    const size_t len   = string.size();

    if (len > n)
    {
        LOG_DBG("String length exceeds memory range");
        return false;
    }

    for (size_t i = 0; i <= n - len; ++i)
    {
        if (start[i] == string[0])
        {
            bool match = true;
            for (size_t j = 1; j < len; ++j)
            {
                if (start[i + j] != string[j])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                result = Handle(reinterpret_cast<uintptr_t>(start) + i);
                LOG_DBG("Found string at {:08X}", result.raw());
                return true;
            }
        }
    }

    LOG_DBG("Could not find string");
    return false;
}

bool memory::Scanner::findWstring(const std::wstring& string, const Range& range, Handle& result)
{
    LOG_DBG("Looking for {}", util::wstringToString(string));
    if (string.empty())
    {
        return false;
    }

    const auto   start = range.start().to_ptr<wchar_t*>();
    const size_t n     = range.size() / sizeof(wchar_t);
    const size_t len   = string.size();

    if (len > n)
    {
        LOG_DBG("String length exceeds memory range");
        return false;
    }

    for (size_t i = 0; i <= n - len; ++i)
    {
        if (start[i] == string[0])
        {
            bool match = true;
            for (size_t j = 1; j < len; ++j)
            {
                if (start[i + j] != string[j])
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                result = Handle(start + i);
                LOG_DBG("Found string at {:08X}", result.raw());
                return true;
            }
        }
    }

    LOG_DBG("Could not find string");
    return false;
}

bool memory::Scanner::findStringReference(const std::string& string, Handle& result)
{
    LOG_DBG("Looking for {}", string);

    if (string.empty())
    {
        LOG_DBG("Invalid string");
        return false;
    }

    const auto hModule = GetModuleHandleA(nullptr);
    MODULEINFO mi {};
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &mi, sizeof(mi)))
    {
        LOG_DBG("Could not get module information: {:08X}", GetLastError());
        return false;
    }

    const auto   base    = reinterpret_cast<uint8_t*>(hModule);
    const size_t imgSize = mi.SizeOfImage;
    const size_t len     = string.size();
    const auto*  text    = reinterpret_cast<const uint8_t*>(string.data());

    for (size_t i = 0; i + len <= imgSize; ++i)
    {
        if (memcmp(base + i, text, len) != 0)
        {
            continue;
        }

        const uint8_t* strAddr = base + i;

        // Scan for LEA instructions referencing strAddr
        for (size_t j = 0; j + 7 < imgSize; ++j)
        {
            const uint8_t* insn = base + j;

            // Quick byte check for REX.W + LEA
            if (insn[0] != 0x48 || insn[1] != 0x8D)
            {
                continue;
            }

            if (const uint8_t modrm = insn[2]; (modrm & 0xC7) != 0x05) // mod=00, rm=101
            {
                continue;
            }

            if (const int32_t disp = *reinterpret_cast<const int32_t*>(insn + 3); insn + 7 + disp == strAddr)
            {
                result = Handle(const_cast<uint8_t*>(insn));
                LOG_DBG("Found reference at {:08X}", result.raw());
                return true;
            }
        }
        // no break: continue checking other occurrences
    }

    LOG_DBG("Could not find reference");
    return false;
}

bool memory::Scanner::findWstringReference(const std::wstring& string, Handle& result)
{
    LOG_DBG("Looking for {}", util::wstringToString(string));

    if (string.empty())
    {
        LOG_DBG("Invalid string");
        return false;
    }

    const auto hModule = GetModuleHandleA(nullptr);
    MODULEINFO mi {};
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &mi, sizeof(mi)))
    {
        LOG_DBG("Could not get module information: {:08X}", GetLastError());
        return false;
    }

    const uint8_t* base    = reinterpret_cast<uint8_t*>(hModule);
    const size_t   imgSize = mi.SizeOfImage;
    const auto*    text    = reinterpret_cast<const uint8_t*>(string.data());
    const size_t   len     = string.size() * sizeof(wchar_t);

    if (len == 0 || len > imgSize)
    {
        return false;
    }

    for (size_t i = 0; i <= imgSize - len; ++i)
    {
        if (memcmp(base + i, text, len) != 0)
        {
            continue;
        }

        const uint8_t* strAddr = base + i;

        // scan for LEA instructions referencing strAddr
        for (size_t j = 0; j + 7 < imgSize; ++j)
        {
            const uint8_t* insn = base + j;

            if (insn[0] != 0x48 || insn[1] != 0x8D) // REX.W + LEA
            {
                continue;
            }

            if (const uint8_t modrm = insn[2]; (modrm & 0xC7) != 0x05) // mod=00, rm=101
            {
                continue;
            }

            if (const int32_t disp = *reinterpret_cast<const int32_t*>(insn + 3); insn + 7 + disp == strAddr)
            {
                result = Handle(const_cast<uint8_t*>(insn));
                LOG_DBG("Found reference at {:08X}", result.raw());
                return true;
            }
        }
        // continue checking other occurrences if no reference found
    }

    LOG_DBG("Could not find reference");
    return false;
}
