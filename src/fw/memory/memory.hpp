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

namespace memory
{
    bool try_near_alloc(const handle& target, size_t size, handle& result);

    bool locate_all_pointers(
        const handle&        base,
        size_t               largest_offset,
        const handle&        target,
        std::vector<handle>& results
    );
}

#endif //UNHIDENPCS_MEMORY_HPP
