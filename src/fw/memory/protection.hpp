#ifndef UNHIDENPCS_PROTECTION_HPP
#define UNHIDENPCS_PROTECTION_HPP
#include "memory.hpp"

namespace memory
{
    struct ProtectedRegion
    {
        memory::Range range {};
        DWORD         oldProtect {};

        [[nodiscard]] Handle    start() const;
        [[nodiscard]] Handle    end() const;
        [[nodiscard]] ptrdiff_t size() const;
    };

    class Protection
    {
    private:
        std::vector<ProtectedRegion> _regions;

    public:
        Protection(const memory::Handle& base, size_t size, DWORD protection);
        Protection(const memory::Range& range, DWORD protection);

        ~Protection();
        Protection(const Protection&)            = delete;
        Protection& operator=(const Protection&) = delete;

        [[nodiscard]] const std::vector<ProtectedRegion>& regions() const;
    };
}

#endif //UNHIDENPCS_PROTECTION_HPP
