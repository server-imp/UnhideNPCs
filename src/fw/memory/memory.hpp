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
}

#endif //UNHIDENPCS_MEMORY_HPP
