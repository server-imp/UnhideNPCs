#include "module.hpp"

#include "scanner.hpp"
#include "fw/logger.hpp"

void memory::Module::initSections()
{
    _sectionsInitialized = true;

    LOG_DBG("Initializing sections of module \"{}\"", _name);

    auto* dos = _start.to_ptr<PIMAGE_DOS_HEADER>();
    auto* nt  = _start.add(dos->e_lfanew).to_ptr<PIMAGE_NT_HEADERS>();

    auto* section = IMAGE_FIRST_SECTION(nt);

    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
    {
        auto*  secBase = _start.to_ptr<uint8_t*>() + section->VirtualAddress;
        size_t secSize = section->Misc.VirtualSize;

        if (memcmp(section->Name, ".text", 5) == 0)
        {
            _textSections.emplace_back(Handle(secBase), secSize);
            LOG_DBG(
                "Found text section at {:08X}-{:08X}",
                _textSections.back().start().raw(),
                _textSections.back().end().raw()
            );
        }
        else if (memcmp(section->Name, ".rdata", 6) == 0 || memcmp(section->Name, ".data", 5) == 0)
        {
            _dataSections.emplace_back(Handle(secBase), secSize);
            LOG_DBG(
                "Found data section at {:08X}-{:08X}",
                _dataSections.back().start().raw(),
                _dataSections.back().end().raw()
            );
        }
    }

    if (_textSections.empty())
        LOG_DBG("No text sections found");

    if (_dataSections.empty())
        LOG_DBG("No data sections found");
}

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

bool memory::Module::findStringReference(const std::string& string, Handle& result)
{
    std::vector<Handle> results {};

    if (!findStringReferences(string, results, 1))
    {
        return false;
    }

    result = results.front();
    return true;
}

bool memory::Module::findStringReferences(const std::string& string, std::vector<Handle>& results, const int max)
{
    results.clear();

    if (string.empty())
    {
        return false;
    }

    LOG_DBG("Looking for instances of \"{}\"", string);

    std::unordered_set<uint8_t*> stringInstances(4);
    for (const auto& section : dataSections())
    {
        auto* base = section.start().to_ptr<uint8_t*>();

        for (size_t i = 0; i + string.size() + 1 <= section.size(); ++i)
        {
            auto* address = base + i;

            if (memcmp(address, string.data(), string.size()) != 0)
                continue;

            if (address[string.size()] != '\0')
                continue;

            stringInstances.emplace(address);
        }
    }

    if (stringInstances.empty())
    {
        LOG_DBG("Did not find any instances");
        return false;
    }

    LOG_DBG("Found {} instance(s), proceeding to look for references", stringInstances.size());

    for (const auto& section : textSections())
    {
        auto* base = section.start().to_ptr<uint8_t*>();

        for (size_t i = 0; i + 7 <= section.size(); ++i)
        {
            auto* insn = base + i;

            // check for REX prefix + LEA instruction
            if ((insn[0] & 0xF0) != 0x40 || insn[1] != 0x8D)
                continue;

            // check ModRM byte for RIP-relative addressing mode
            // mod=00, r/m=101 means the operand is [RIP + disp32]
            if ((insn[2] & 0xC7) != 0x05)
                continue;

            // read the 32-bit relative offset stored after the opcode
            int32_t offset;
            memcpy(&offset, insn + 3, sizeof(offset));

            auto* target = insn + 7 + offset;

            if (stringInstances.find(target) != stringInstances.end())
            {
                results.emplace_back(insn);

                if (max > 0 && results.size() >= max)
                {
                    goto retn;
                }
            }
        }
    }

    if (results.empty())
    {
        LOG_DBG("Could not find any reference");
        return false;
    }

retn: LOG_DBG("Found {} references", results.size());
    return true;
}

bool memory::Module::findWstringReference(const std::wstring& string, Handle& result)
{
    std::vector<Handle> results {};

    if (!findWstringReferences(string, results, 1))
    {
        return false;
    }

    result = results.front();
    return true;
}

bool memory::Module::findWstringReferences(const std::wstring& string, std::vector<Handle>& results, int max)
{
    results.clear();

    if (string.empty())
    {
        return false;
    }

    LOG_DBG("Looking for instances of L\"{}\"", util::wstringToString(string));

    std::unordered_set<uint8_t*> stringInstances(4);
    for (const auto& section : dataSections())
    {
        auto* base = section.start().to_ptr<uint8_t*>();

        for (size_t i = 0; i + string.size() + 1 <= section.size(); ++i)
        {
            auto* address = base + i;

            if (memcmp(address, string.data(), string.size()) != 0)
                continue;

            if (address[string.size()] != '\0')
                continue;

            stringInstances.emplace(address);
        }
    }

    if (stringInstances.empty())
    {
        LOG_DBG("Did not find any instances");
        return false;
    }

    LOG_DBG("Found {} instance(s), proceeding to look for references", stringInstances.size());

    for (const auto& section : textSections())
    {
        auto* base = section.start().to_ptr<uint8_t*>();

        for (size_t i = 0; i + 7 <= section.size(); ++i)
        {
            auto* insn = base + i;

            // check for REX prefix + LEA instruction
            if ((insn[0] & 0xF0) != 0x40 || insn[1] != 0x8D)
                continue;

            // check ModRM byte for RIP-relative addressing mode
            // mod=00, r/m=101 means the operand is [RIP + disp32]
            if ((insn[2] & 0xC7) != 0x05)
                continue;

            // read the 32-bit relative offset stored after the opcode
            int32_t offset;
            memcpy(&offset, insn + 3, sizeof(offset));

            auto* target = insn + 7 + offset;

            if (stringInstances.find(target) != stringInstances.end())
            {
                results.emplace_back(insn);

                if (max > 0 && results.size() >= max)
                {
                    goto retn;
                }
            }
        }
    }

    if (results.empty())
    {
        LOG_DBG("Could not find any reference");
        return false;
    }

retn: LOG_DBG("Found {} references", results.size());
    return true;
}

std::vector<memory::Range> memory::Module::textSections()
{
    if (!_sectionsInitialized)
    {
        initSections();
    }

    return _textSections;
}

std::vector<memory::Range> memory::Module::dataSections()
{
    if (!_sectionsInitialized)
    {
        initSections();
    }

    return _dataSections;
}

bool memory::Module::isCode(const Handle& handle)
{
    return contains(textSections(), handle);
}

bool memory::Module::isData(const Handle& handle)
{
    return contains(dataSections(), handle);
}

memory::Module memory::Module::getFromHandle(const HMODULE hModule)
{
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
    char dllPath[MAX_PATH] {};
    if (!GetModuleFileName(hModule, dllPath, MAX_PATH))
    {
        LOG_DBG("GetModuleFileName failed: {}", GetLastError());
        return {};
    }

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

    std::vector<HMODULE> mods(needed / sizeof(HMODULE));
    if (!EnumProcessModules(GetCurrentProcess(), mods.data(), needed, &needed))
    {
        LOG_DBG("EnumProcessModules[2] failed: {}", GetLastError());
        return false;
    }

    int done = 0;
    for (const HMODULE mod : mods)
    {
        //LOG_DBG("Checking module at {:08X} [{}/{}]", reinterpret_cast<uintptr_t>(mod), done++, mods.size());
        result = getFromHandle(mod);
        if (!result.size())
        {
            continue;
        }

        if (addr >= result.start() && addr < result.end())
        {
            LOG_DBG("Found module: {}", result.name());
            return true;
        }
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
