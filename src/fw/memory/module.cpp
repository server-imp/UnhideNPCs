#include "module.hpp"

#include "scanner.hpp"
#include "fw/logger.hpp"

HMODULE memory::Module::handle() const
{
    return _hModule;
}

const std::string& memory::Module::name()
{
    return _name;
}

const std::filesystem::path& memory::Module::path()
{
    return _path;
}

bool memory::Module::findPattern(const std::string& pattern, memory::Handle& result) const
{
    return Scanner::findPattern(pattern, *this, result);
}

bool memory::Module::findString(const std::string& string, memory::Handle& result) const
{
    return Scanner::findString(string, *this, result);
}

bool memory::Module::findWstring(const std::wstring& string, memory::Handle& result) const
{
    return Scanner::findWstring(string, *this, result);
}

memory::Module memory::Module::getFromHandle(const HMODULE hModule)
{
    LOG_DBG("Getting module from handle: {:08X}", reinterpret_cast<uintptr_t>(hModule));

    if (!hModule)
    {
        LOG_DBG("Invalid module handle");
        return {};
    }

    MODULEINFO moduleInfo {};
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo)))
    {
        LOG_DBG("GetModuleInformation failed: {}", GetLastError());
        return {};
    }
    LOG_DBG("Module info: {:08X} - {:04X}", reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll), moduleInfo.SizeOfImage);

    char dllPath[MAX_PATH] {};
    if (!GetModuleFileName(hModule, dllPath, MAX_PATH))
    {
        LOG_DBG("GetModuleFileName failed: {}", GetLastError());
        return {};
    }
    LOG_DBG("Module path: \"{}\"", dllPath);

    Module result {};
    result._hModule = hModule;
    result._path    = dllPath;
    result._name    = result._path.filename().string();

    result._start = memory::Handle(moduleInfo.lpBaseOfDll);
    result._end   = result._start.add(moduleInfo.SizeOfImage);
    result._size  = moduleInfo.SizeOfImage;

    return result;
}

bool memory::Module::tryGetByName(const std::string& name, Module& result)
{
    if (name.empty())
    {
        LOG_DBG("Getting main module");
    }
    else
    {
        LOG_DBG("Getting module by name \"{}\"", name);
    }

    const auto hModule = GetModuleHandleA(name.empty() ? nullptr : name.c_str());
    if (!hModule)
    {
        LOG_DBG("GetModuleHandle failed: {}", GetLastError());
        return false;
    }

    result = getFromHandle(hModule);
    if (!result.size())
    {
        return false;
    }

    LOG_DBG("Found module at {:08X}", result._start.raw());
    return true;
}

memory::Module memory::Module::getByName(const std::string& name)
{
    Module result {};
    if (!tryGetByName(name, result))
    {
        LOG_DBG("tryGetModuleByName failed");
    }
    return result;
}

bool memory::Module::tryGetByAddr(const memory::Handle& addr, Module& result)
{
    if (!addr.raw())
    {
        return false;
    }

    LOG_DBG("Attempting to find module that holds address {:08X}", addr.raw());
    DWORD needed = 0;

    // Get the required size
    if (!EnumProcessModules(GetCurrentProcess(), nullptr, 0, &needed) || needed == 0)
    {
        LOG_DBG("EnumProcessModules[1] failed: {}", GetLastError());
        return false;
    }
    LOG_DBG("Needed: {}", needed / sizeof(HMODULE));

    std::vector<HMODULE> mods(needed / sizeof(HMODULE));
    if (!EnumProcessModules(GetCurrentProcess(), mods.data(), needed, &needed))
    {
        LOG_DBG("EnumProcessModules[2] failed: {}", GetLastError());
        return false;
    }

    LOG_DBG("Iterating over {} modules", mods.size());
    int done = 0;
    for (const HMODULE mod : mods)
    {
        LOG_DBG("Checking module at {:08X} [{}/{}]", reinterpret_cast<uintptr_t>(mod), done++, mods.size());
        result = getFromHandle(mod);
        if (!result.size())
        {
            LOG_DBG("Module is invalid");
            continue;
        }

        LOG_DBG("Checking module range");
        if (addr >= result.start() && addr < result.end())
        {
            LOG_DBG("Found module: {}", result.name());
            return true;
        }
        LOG_DBG("Module range unmatched");
    }

    LOG_DBG("Not found");
    return false;
}

memory::Module memory::Module::getMain()
{
    return getByName("");
}

memory::Module memory::Module::getThis()
{
    HMODULE hModule = nullptr;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCTSTR>(&getThis), &hModule);

    return getFromHandle(hModule);
}
