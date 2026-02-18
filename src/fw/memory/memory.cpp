#include "memory.hpp"

#include "../logger.hpp"
#include "fw/util.hpp"

bool memory::tryNearAlloc(const Handle& target, const size_t size, Handle& result)
{
    LOG_DBG("Attempting near allocation (0x{:08X}, 0x{:04X})", target.raw(), size);
    constexpr SIZE_T    granularity = 0x10000;
    constexpr uintptr_t maxOffset  = 0x7FFF0000;
    const uintptr_t     base        = target.raw();

    for (uintptr_t offset = 0; offset < maxOffset; offset += granularity)
    {
        for (const int sign : { -1, 1 })
        {
            const uintptr_t tryAddr = base + sign * offset;

            if (sign == -1 && tryAddr > base)
            {
                continue;
            }
            if (sign == 1 && tryAddr < base)
            {
                continue;
            }

            if (void* p = VirtualAlloc(
                reinterpret_cast<void*>(tryAddr),
                size,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE
            ))
            {
                result = Handle(p);
                LOG_DBG("Successful allocation at {:08X}", result.raw());
                return true;
            }
        }
    }

    LOG_DBG("Failed");
    return false;
}

bool memory::locateAllPointers(
    const Handle&        base,
    const size_t         largestOffset,
    const Handle&        target,
    std::vector<Handle>& results
)
{
    results.clear();

    if (!base.raw() || largestOffset < sizeof(uintptr_t))
    {
        LOG_ERR("Invalid base or largest offset");
        return false;
    }

    const auto address = reinterpret_cast<void*>(base.raw());

    DWORD oldProtect = 0;
    if (!VirtualProtect(address, largestOffset, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        LOG_ERR("VirtualProtect failed");
        return false;
    }

    std::vector<uint8_t> buffer(largestOffset);
    std::memcpy(buffer.data(), address, largestOffset);

    DWORD tmp;
    VirtualProtect(address, largestOffset, oldProtect, &tmp);

    const uintptr_t target_value = target.raw();

    for (ptrdiff_t i = 0; i <= largestOffset - sizeof(uintptr_t); ++i)
    {
        uintptr_t value;
        std::memcpy(&value, buffer.data() + i, sizeof(value));

        if (value == target_value)
        {
            results.emplace_back(base.add(i));
        }
    }

    return !results.empty();
}
