#include <utility>

#include "hook.hpp"

#include "handle.hpp"
#include "module.hpp"
#include "../logger.hpp"

#include "MinHook.h"
#include "fw/util.hpp"

memory::Hook::Hook(std::string name, void* target, void* original, void* ownFunction) : _name(std::move(name)),
    _target(target), _original(original), _ownFunction(ownFunction)
{
    const auto  from = Handle(_target);
    std::string fromStr {};
    const auto  to = Handle(_ownFunction);
    std::string toStr {};

    Module module {};
    if (Module::tryGetByAddr(from, module))
    {
        fromStr = fmt::format("{}+{:X}", module.name(), from.sub(module.start()).raw());
    }
    else
    {
        fromStr = fmt::format("{:08X}", from.raw());
    }

    if (memory::Module::tryGetByAddr(to, module))
    {
        toStr = fmt::format("{}+{:X}", module.name(), to.sub(module.start()).raw());
    }
    else
    {
        toStr = fmt::format("{:08X}", to.raw());
    }

    LOG_DBG("Created hook \"{}\" {} -> {}", _name, fromStr, toStr);
}

const std::string& memory::Hook::name() const
{
    return _name;
}

bool memory::Hook::enabled() const
{
    return _enabled;
}

void* memory::Hook::target() const
{
    return _target;
}

memory::Detour::Detour(std::string name, void* target, void* ownFunction) : Hook(
    std::move(name),
    target,
    nullptr,
    ownFunction
) {}

bool memory::Detour::enable()
{
    LOG_DBG("Enabling \"{}\"", _name);

    if (_enabled)
    {
        LOG_DBG("Already enabled");
        return false;
    }

    auto status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
    {
        LOG_DBG("MH_Initialize failed: {}", MH_StatusToString(status));
        return false;
    }

    status = MH_CreateHook(_target, _ownFunction, &_original);
    if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED)
    {
        LOG_DBG("MH_CreateHook failed: {}", MH_StatusToString(status));
        return false;
    }

    status = MH_EnableHook(_target);
    if (status != MH_OK && status != MH_ERROR_ENABLED)
    {
        LOG_DBG("MH_EnableHook failed: {}", MH_StatusToString(status));
        return false;
    }

    LOG_DBG("Enabled \"{}\"", _name);
    _enabled = true;
    return true;
}

bool memory::Detour::disable(const bool uninitialize)
{
    LOG_DBG("Disabling \"{}\"", _name);
    if (!_enabled)
    {
        LOG_DBG("Already disabled");
        return false;
    }

    auto status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
    {
        LOG_DBG("MH_Initialize failed: {}", MH_StatusToString(status));
        return false;
    }

    status = MH_DisableHook(_target);
    if (status != MH_OK && status != MH_ERROR_DISABLED && status != MH_ERROR_NOT_CREATED)
    {
        LOG_DBG("MH_DisableHook failed: {}", MH_StatusToString(status));
        return false;
    }

    if (uninitialize)
    {
        status = MH_Uninitialize();
        if (status != MH_OK)
        {
            LOG_DBG("MH_Uninitialize failed: {}", MH_StatusToString(status));
            return false;
        }
    }

    LOG_DBG("Disabled \"{}\"", _name);
    _enabled = false;
    return true;
}

memory::HookScope::HookScope(std::atomic_uint32_t& counter)
    : counter(counter)
{
    counter.fetch_add(1, std::memory_order_acq_rel);
}

memory::HookScope::~HookScope()
{
    counter.fetch_sub(1, std::memory_order_acq_rel);
}
