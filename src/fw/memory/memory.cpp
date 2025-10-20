#include "memory.hpp"

#include "../logger.hpp"

bool memory::try_near_alloc(const handle& target, const size_t size, handle& result)
{
    LOG_DBG("Attempting near allocation (0x{:08X}, 0x{:04X})");
    constexpr SIZE_T    granularity = 0x10000;
    constexpr uintptr_t max_offset  = 0x7FFF0000;
    const uintptr_t     base        = target.raw();

    for (uintptr_t offset = 0; offset < max_offset; offset += granularity)
    {
        for (const int sign : {-1, 1})
        {
            const uintptr_t try_addr = base + sign * offset;

            if (sign == -1 && try_addr > base)
                continue;
            if (sign == 1 && try_addr < base)
                continue;

            if (void* p = VirtualAlloc
                (reinterpret_cast<void*>(try_addr), size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))
            {
                result = handle(p);
                LOG_DBG("Successfull allocation at {:08X}", result.raw());
                return true;
            }
        }
    }

    LOG_DBG("Failed");
    return false;
}
