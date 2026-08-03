#ifndef UNHIDENPCS_PCH_HPP
#define UNHIDENPCS_PCH_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <cstdint>
#include <format>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <windows.h>

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;

#endif // UNHIDENPCS_PCH_HPP
