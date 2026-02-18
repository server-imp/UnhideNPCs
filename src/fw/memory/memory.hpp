#ifndef UNHIDENPCS_MEMORY_HPP
#define UNHIDENPCS_MEMORY_HPP
#pragma once

#include "handle.hpp"
#include "hook.hpp"
#include "range.hpp"
#include "pattern.hpp"
#include "scanner.hpp"
#include "module.hpp"
#include "pointer_validator.hpp"
#include "protection.hpp"

namespace memory
{
    bool tryNearAlloc(const Handle& target, size_t size, Handle& result);

    bool locateAllPointers(
        const Handle&        base,
        size_t               largestOffset,
        const Handle&        target,
        std::vector<Handle>& results
    );
}

#endif //UNHIDENPCS_MEMORY_HPP
