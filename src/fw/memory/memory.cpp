#include "memory.hpp"

#include "../logger.hpp"
#include "fw/util.hpp"

bool memory::try_near_alloc(const handle& target, const size_t size, handle& result)
{
    LOG_DBG("Attempting near allocation (0x{:08X}, 0x{:04X})", target.raw(), size);
    constexpr SIZE_T    granularity = 0x10000;
    constexpr uintptr_t max_offset  = 0x7FFF0000;
    const uintptr_t     base        = target.raw();

    for (uintptr_t offset = 0; offset < max_offset; offset += granularity)
    {
        for (const int sign : { -1, 1 })
        {
            const uintptr_t try_addr = base + sign * offset;

            if (sign == -1 && try_addr > base)
                continue;
            if (sign == 1 && try_addr < base)
                continue;

            if (void* p = VirtualAlloc(
                reinterpret_cast<void*>(try_addr),
                size,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE
            ))
            {
                result = handle(p);
                LOG_DBG("Successful allocation at {:08X}", result.raw());
                return true;
            }
        }
    }

    LOG_DBG("Failed");
    return false;
}

bool memory::locate_all_pointers(
    const handle&        base,
    const size_t         largest_offset,
    const handle&        target,
    std::vector<handle>& results
)
{
    results.clear();

    if (!base.raw() || largest_offset < sizeof(uintptr_t))
    {
        LOG_ERR("Invalid base or largest offset");
        return false;
    }

    const auto address = reinterpret_cast<void*>(base.raw());

    DWORD oldProtect = 0;
    if (!VirtualProtect(address, largest_offset, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        LOG_ERR("VirtualProtect failed");
        return false;
    }

    std::vector<uint8_t> buffer(largest_offset);
    std::memcpy(buffer.data(), address, largest_offset);

    DWORD tmp;
    VirtualProtect(address, largest_offset, oldProtect, &tmp);

    const uintptr_t target_value = target.raw();

    for (ptrdiff_t i = 0; i <= largest_offset - sizeof(uintptr_t); ++i)
    {
        uintptr_t value;
        std::memcpy(&value, buffer.data() + i, sizeof(value));

        if (value == target_value)
            results.emplace_back(base.add(i));
    }

    return !results.empty();
}
