#ifndef UNHIDENPCS_MODULE_HPP
#define UNHIDENPCS_MODULE_HPP
#pragma once
#include "range.hpp"

namespace memory
{
    class Module : public Range
    {
    private:
        HMODULE               _hModule {};
        std::string           _name {};
        std::filesystem::path _path {};

    public:
        [[nodiscard]] HMODULE handle() const;

        const std::string& name();

        const std::filesystem::path& path();

        bool findPattern(const std::string& pattern, memory::Handle& result) const;

        bool findString(const std::string& string, memory::Handle& result) const;

        bool findWstring(const std::wstring& string, memory::Handle& result) const;

        static Module getFromHandle(HMODULE hModule);

        static bool tryGetByName(const std::string& name, Module& result);

        static Module getByName(const std::string& name);

        static bool tryGetByAddr(const memory::Handle& addr, Module& result);

        static Module getMain();

        static Module getThis();
    };
}

#endif //UNHIDENPCS_MODULE_HPP
