#include "pointer_validator.hpp"

#include "fw/logger.hpp"

memory::PointerValidator::PointerValidator()
{
    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);

    _pageSize                 = sysInfo.dwPageSize;
    _minimumApplicableAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    _maximumApplicableAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

    _cacheDurationMs = 1000;
}

void memory::PointerValidator::updateTick()
{
    _currentTick = GetTickCount64();
}

memory::PointerValidator& memory::PointerValidator::instance()
{
    static PointerValidator instance{};
    return instance;
}

void memory::PointerValidator::UpdateTick()
{
    instance().updateTick();
}

bool memory::PointerValidator::Validate(const uintptr_t pointer)
{
    return instance().validate(pointer);
}

bool memory::PointerValidator::Validate(void* pointer)
{
    return instance().validate(pointer);
}

void memory::PointerValidator::ClearCache()
{
    instance().clearCache();
}

bool memory::PointerValidator::probe(const uintptr_t pointer)
{
    MEMORY_BASIC_INFORMATION mbi{};
    if (const auto result = VirtualQuery(reinterpret_cast<LPCVOID>(pointer), &mbi, sizeof(mbi)); result == 0)
        return false;

    if (const auto protect = mbi.Protect; protect & PAGE_NOACCESS || protect & PAGE_GUARD)
        return false;

    return true;
}

bool memory::PointerValidator::updateCacheItem(const uintptr_t pointer, const uint64_t expireTime, const bool valid)
{
    if (const auto it = _cache.find(pointer); it != _cache.end())
    {
        it->second.expireTime = expireTime;
        it->second.valid      = valid;
    }
    else
    {
        _cache.emplace(pointer, CacheEntry{expireTime, valid});
    }

    return valid;
}

bool memory::PointerValidator::validate(const uintptr_t pointer)
{
    if (!pointer)
        return false;

    if (pointer < _minimumApplicableAddress || pointer > _maximumApplicableAddress)
    {
        LOG_DBG("Rejected pointer {:08X} - out of range", pointer);
        return false;
    }

    const auto alignedPointer = pointer & ~(_pageSize - 1);
    if (const auto it = _cache.find(alignedPointer); it != _cache.end())
    {
        if (const auto& [expireTime, valid] = it->second; _currentTick < expireTime)
            return valid;
    }

    const bool result = probe(alignedPointer);
    if (!result)
    {
        LOG_DBG("Rejected pointer {:08X} - probe failed", pointer);
    }

    return updateCacheItem(alignedPointer, _currentTick + _cacheDurationMs, result);
}

bool memory::PointerValidator::validate(void* pointer)
{
    return validate(reinterpret_cast<uintptr_t>(pointer));
}

void memory::PointerValidator::clearCache()
{
    _cache.clear();
}
