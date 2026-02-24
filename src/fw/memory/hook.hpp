#ifndef UNHIDENPCS_HOOK_HPP
#define UNHIDENPCS_HOOK_HPP
#pragma once
#include "pch.hpp"

namespace memory
{
    class Hook
    {
    protected:
        std::string _name {};

        bool _enabled {};

        void* _target {};
        void* _original {};
        void* _ownFunction {};

        Hook(std::string name, void* target, void* original, void* ownFunction);

        ~Hook() = default;

    public:
        [[nodiscard]] const std::string& name() const;

        [[nodiscard]] bool enabled() const;

        [[nodiscard]] void* target() const;

        template <typename T>
        T original() const;

        virtual bool enable() = 0;

        virtual bool disable(bool uninitialize) = 0;
    };

    template <typename T>
    T Hook::original() const
    {
        return reinterpret_cast<T>(_original);
    }

    class Detour final : public Hook
    {
    public:
        Detour(std::string name, void* target, void* ownFunction);

        bool enable() override;

        bool disable(bool uninitialize) override;
    };

    struct HookScope {
        std::atomic_uint32_t& counter;

        explicit HookScope(std::atomic_uint32_t& counter);
        ~HookScope();
    };
}

#endif //UNHIDENPCS_HOOK_HPP
