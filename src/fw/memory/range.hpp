#ifndef UNHIDENPCS_RANGE_HPP
#define UNHIDENPCS_RANGE_HPP
#pragma once
#include "handle.hpp"

namespace memory
{
    class range
    {
    protected:
        handle    _start{};
        handle    _end{};
        ptrdiff_t _size{};

        range() = default;

    public:
        explicit range(const char* moduleName);

        explicit range(const handle& start, ptrdiff_t size);

        explicit range(const handle& start, const handle& end);

        explicit range(uintptr_t start, ptrdiff_t size);

        const handle& start() const;

        const handle& end() const;

        const ptrdiff_t& size() const;

        bool contains(const handle& address) const;

        bool contains(uintptr_t address) const;
    };
}

#endif //UNHIDENPCS_RANGE_HPP
