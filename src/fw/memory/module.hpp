#ifndef UNHIDENPCS_MODULE_HPP
#define UNHIDENPCS_MODULE_HPP
#pragma once
#include "range.hpp"

namespace memory
{
    class module : public range
    {
    private:
        HMODULE               _hModule {};
        std::string           _name {};
        std::filesystem::path _path {};

    public:
        [[nodiscard]] HMODULE handle() const;

        const std::string& name();

        const std::filesystem::path& path();

        bool find_pattern(const std::string& pattern, memory::handle& result) const;

        bool find_string(const std::string& string, memory::handle& result) const;

        bool find_wstring(const std::wstring& string, memory::handle& result) const;

        static module getFromHandle(HMODULE hModule);

        static bool tryGetByName(const std::string& name, module& result);

        static module getByName(const std::string& name);

        static bool tryGetByAddr(const memory::handle& addr, module& result);

        static module getMain();

        static module getThis();
    };
}

#endif //UNHIDENPCS_MODULE_HPP
