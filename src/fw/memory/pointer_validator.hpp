#ifndef UNHIDENPCS_POINTER_VALIDATOR_HPP
#define UNHIDENPCS_POINTER_VALIDATOR_HPP
#pragma once

namespace memory
{
    class PointerValidator
    {
    private:
        struct CacheEntry
        {
            uint64_t expireTime {};
            bool     valid {};
        };

        uint64_t                                  _currentTick {};
        uint32_t                                  _cacheDurationMs {};
        std::unordered_map<uintptr_t, CacheEntry> _cache {};
        size_t                                    _pageSize {};
        uintptr_t                                 _minimumApplicableAddress {};
        uintptr_t                                 _maximumApplicableAddress {};

        static bool probe(uintptr_t pointer);

        bool updateCacheItem(uintptr_t pointer, uint64_t expireTime, bool valid);

        PointerValidator();

    public:
        void updateTick();

        bool validate(uintptr_t pointer);

        bool validate(void* pointer);

        template <typename T>
        std::enable_if_t<std::is_pointer_v<T>, bool> dereference(T pointer, T* result);

        void clearCache();

        static PointerValidator& instance();

        static void UpdateTick();

        static bool Validate(uintptr_t pointer);

        static bool Validate(void* pointer);

        template <typename T>
        static std::enable_if_t<std::is_pointer_v<T>, bool> Dereference(T pointer, T* result);

        static void ClearCache();
    };

    template <typename T>
    std::enable_if_t<std::is_pointer_v<T>, bool> PointerValidator::dereference(T pointer, T* result)
    {
        if (!validate(pointer))
            return false;

        *result = pointer;
        return true;
    }

    template <typename T>
    std::enable_if_t<std::is_pointer_v<T>, bool> PointerValidator::Dereference(T pointer, T* result)
    {
        return instance().dereference(pointer, result);
    }
}

#define VALIDATE(pointer) memory::PointerValidator::Validate(reinterpret_cast<uintptr_t>(pointer))
#define DEREFERENCE(pointer, result) memory::PointerValidator::Dereference(pointer, result)

#endif //UNHIDENPCS_POINTER_VALIDATOR_HPP
