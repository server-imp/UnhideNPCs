#include "proxy.hpp"

#include "util.hpp"

bool proxy::check(const std::initializer_list<std::string>& candidates, std::string& proxyName)
{
    HMODULE hOurModule = nullptr;
    if (!GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&check),
        &hOurModule
    ))
    {
        return false;
    }

    std::filesystem::path ourPath {};
    if (!util::getModuleFilePath(hOurModule, ourPath))
    {
        return false;
    }

    const auto hMainModule = GetModuleHandleA(nullptr);
    if (!hMainModule)
    {
        return false;
    }

    std::filesystem::path exePath {};
    if (!util::getModuleFilePath(hMainModule, exePath))
    {
        return false;
    }

    if (ourPath.parent_path() != exePath.parent_path())
    {
        return false;
    }

    const auto ourName = ourPath.filename().string();
    for (const auto& candidate : candidates)
    {
        if (util::equalsIgnoreCase(candidate, ourName))
        {
            proxyName = candidate;
            return true;
        }
    }

    return false;
}
