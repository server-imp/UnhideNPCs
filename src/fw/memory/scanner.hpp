#ifndef UNHIDENPCS_SCANNER_HPP
#define UNHIDENPCS_SCANNER_HPP
#pragma once

#include "range.hpp"
#include "pattern.hpp"

namespace memory
{
    class scanner
    {
    public:
        scanner() = delete;

        ~scanner() = delete;

        scanner& operator=(const scanner&) = delete;

        static bool find_pattern(const std::string& pattern, const range& range, handle& result);

        static bool find_string(const std::string& string, const range& range, handle& result);

        static bool find_wstring(const std::wstring& string, const range& range, handle& result);

        static bool find_string_reference(const std::string& string, handle& result);

        static bool find_wstring_reference(const std::wstring& string, handle& result);
    };
}

#endif //UNHIDENPCS_SCANNER_HPP
