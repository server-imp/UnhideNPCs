#include "protection.hpp"

#include "fw/logger.hpp"
#include "fw/util.hpp"

memory::Handle memory::ProtectedRegion::start() const
{
    return range.start();
}

memory::Handle memory::ProtectedRegion::end() const
{
    return range.end();
}

ptrdiff_t memory::ProtectedRegion::size() const
{
    return range.size();
}

memory::Protection::Protection(const memory::Handle& base, const size_t size, const DWORD protection)
{
    if (!base.raw())
    {
        LOG_DBG("Invalid base pointer");
        return;
    }

    memory::Handle       current = base;
    const memory::Handle end     = base.add(static_cast<std::ptrdiff_t>(size));

    while (current < end)
    {
        MEMORY_BASIC_INFORMATION info {};
        if (!VirtualQuery(current.to_ptr<void*>(), &info, sizeof(info)))
        {
            LOG_DBG("VirtualQuery failed: {:08X} [{}]", current.raw(), GetLastError());
            break;
        }

        memory::Handle regionStart(info.BaseAddress);
        const auto     regionEnd = regionStart.add(static_cast<std::ptrdiff_t>(info.RegionSize));

        if (info.State != MEM_COMMIT)
        {
            LOG_DBG("Memory not committed: {:08X}", current.raw());
            current = regionEnd;
            break;
        }

        auto protectionStart = current;
        auto protectionEnd   = (end < regionEnd) ? end : regionEnd;

        const size_t protectionSize = protectionEnd.raw() - protectionStart.raw();

        DWORD oldProtect;
        if (!VirtualProtect(protectionStart.to_ptr<void*>(), protectionSize, protection, &oldProtect))
        {
            LOG_DBG("VirtualProtect failed: {:08X} [{}]", protectionStart.raw(), GetLastError());
            current = regionEnd;
            break;
        }

        _regions.push_back({ Range(protectionStart, protectionEnd), oldProtect });
        current = regionEnd;
    }
}

memory::Protection::Protection(const memory::Range& range, const DWORD protection) : memory::Protection(
    range.start(),
    range.size(),
    protection
) {}

memory::Protection::~Protection()
{
    for (auto& region : _regions)
    {
        DWORD temp;
        VirtualProtect(region.start().to_ptr<void*>(), region.size(), region.oldProtect, &temp);
    }
}

const std::vector<memory::ProtectedRegion>& memory::Protection::regions() const
{
    return _regions;
}
