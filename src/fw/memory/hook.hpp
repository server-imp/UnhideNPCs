#ifndef UNHIDENPCS_HOOK_HPP
#define UNHIDENPCS_HOOK_HPP
#pragma once
#include "pch.hpp"

namespace memory
{
    class hook
    {
    protected:
        std::string _name {};

        bool _enabled {};

        void* _target {};
        void* _original {};
        void* _ownFunction {};

        hook(std::string name, void* target, void* original, void* ownFunction);

        ~hook() = default;

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
    T hook::original() const
    {
        return reinterpret_cast<T>(_original);
    }

    class detour final : public hook
    {
    public:
        detour(std::string name, void* target, void* ownFunction);

        bool enable() override;

        bool disable(bool uninitialize) override;
    };
}

#endif //UNHIDENPCS_HOOK_HPP
