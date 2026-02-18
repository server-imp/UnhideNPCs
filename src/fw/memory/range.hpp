#ifndef UNHIDENPCS_RANGE_HPP
#define UNHIDENPCS_RANGE_HPP
#pragma once
#include "handle.hpp"

namespace memory
{
    class Range
    {
    protected:
        Handle    _start {};
        Handle    _end {};
        ptrdiff_t _size {};

    public:
        Range() = default;

        explicit Range(const char* moduleName);

        explicit Range(const Handle& start, ptrdiff_t size);

        explicit Range(const Handle& start, const Handle& end);

        explicit Range(uintptr_t start, ptrdiff_t size);

        [[nodiscard]] const Handle& start() const;

        [[nodiscard]] const Handle& end() const;

        [[nodiscard]] const ptrdiff_t& size() const;

        [[nodiscard]] bool contains(const Handle& address) const;

        [[nodiscard]] bool contains(uintptr_t address) const;
    };
}

#endif //UNHIDENPCS_RANGE_HPP
