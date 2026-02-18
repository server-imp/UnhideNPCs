#ifndef UNHIDENPCS_PROTECTION_HPP
#define UNHIDENPCS_PROTECTION_HPP
#include "memory.hpp"

namespace memory
{
    struct ProtectedRegion
    {
        memory::range range {};
        DWORD         oldProtect {};

        handle    start() const;
        handle    end() const;
        ptrdiff_t size() const;
    };

    class protection
    {
    private:
        std::vector<ProtectedRegion> _regions;

    public:
        protection(const memory::handle& base, size_t size, DWORD protection);
        protection(const memory::range& range, DWORD protection);

        ~protection();
        protection(const protection&)            = delete;
        protection& operator=(const protection&) = delete;

        const std::vector<ProtectedRegion>& regions() const;
    };
}

#endif //UNHIDENPCS_PROTECTION_HPP
