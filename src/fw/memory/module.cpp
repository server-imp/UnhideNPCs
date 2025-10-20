#include "module.hpp"

#include "scanner.hpp"
#include "fw/logger.hpp"

HMODULE memory::module::handle() const
{
    return _hModule;
}

const std::string& memory::module::name()
{
    return _name;
}

const std::filesystem::path& memory::module::path()
{
    return _path;
}

bool memory::module::find_pattern(const std::string& pattern, memory::handle& result) const
{
    return scanner::find_pattern(pattern, *this, result);
}

bool memory::module::find_string(const std::string& string, memory::handle& result) const
{
    return scanner::find_string(string, *this, result);
}

bool memory::module::find_wstring(const std::wstring& string, memory::handle& result) const
{
    return scanner::find_wstring(string, *this, result);
}

memory::module memory::module::getFromHandle(const HMODULE hModule)
{
    if (!hModule)
    {
        LOG_DBG("Invalid module handle");
        return {};
    }

    MODULEINFO moduleInfo{};
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo)))
    {
        LOG_DBG("GetModuleInformation failed: {}", GetLastError());
        return {};
    }

    char dllPath[MAX_PATH]{};
    if (!GetModuleFileName(hModule, dllPath, MAX_PATH))
    {
        LOG_DBG("GetModuleFileName failed: {}", GetLastError());
        return {};
    }

    module result{};
    result._hModule = hModule;
    result._path    = dllPath;
    result._name    = result._path.filename().string();

    result._start = memory::handle(moduleInfo.lpBaseOfDll);
    result._end   = result._start.add(moduleInfo.SizeOfImage);
    result._size  = moduleInfo.SizeOfImage;

    return result;
}

memory::module memory::module::getThis()
{
    HMODULE hModule = nullptr;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCTSTR>(&getThis), &hModule);

    return getFromHandle(hModule);
}

bool memory::module::tryGetByName(const std::string& name, module& result)
{
    if (name.empty())
        LOG_DBG("Getting main module");
    else
        LOG_DBG("Getting module by name \"{}\"", name);

    const auto hModule = GetModuleHandleA(name.empty() ? nullptr : name.c_str());
    if (!hModule)
    {
        LOG_DBG("GetModuleHandle failed: {}", GetLastError());
        return false;
    }

    result = getFromHandle(hModule);
    if (!result.size())
        return false;

    LOG_DBG("Found module at {:08X}", result._start.raw());
    return true;
}

memory::module memory::module::getByName(const std::string& name)
{
    module result{};
    if (!tryGetByName(name, result))
        LOG_DBG("tryGetModuleByName failed");
    return result;
}

bool memory::module::tryGetByAddr(const memory::handle& addr, module& result)
{
    LOG_DBG("Attempting to find module that holds address {:08X}", addr.raw());
    DWORD needed = 0;

    // Get required size
    if (!EnumProcessModules(GetCurrentProcess(), nullptr, 0, &needed) || needed == 0)
    {
        LOG_DBG("EnumProcessModules[1] failed: {}", GetLastError());
        return false;
    }

    std::vector<HMODULE> mods(needed / sizeof(HMODULE));
    if (!EnumProcessModules(GetCurrentProcess(), mods.data(), needed, &needed))
    {
        LOG_DBG("EnumProcessModules[2] failed: {}", GetLastError());
        return false;
    }

    for (const HMODULE mod : mods)
    {
        result = getFromHandle(mod);
        if (!result.size())
            continue;

        if (addr >= result.start() && addr < result.end())
        {
            LOG_DBG("Found module: {}", result.name());
            return true;
        }
    }

    LOG_DBG("Not found");
    return false;
}

memory::module memory::module::getMain()
{
    return getByName("");
}
