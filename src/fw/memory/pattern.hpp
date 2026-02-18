#ifndef UNHIDENPCS_PATTERN_HPP
#define UNHIDENPCS_PATTERN_HPP
#pragma once

namespace memory
{
    class Pattern
    {
    private:
        std::vector<uint8_t> _data;
        std::vector<uint8_t> _mask;

    public:
        explicit Pattern(const std::vector<uint8_t>& data, const std::vector<uint8_t>& mask);

        explicit Pattern(const std::string& ida);

        [[nodiscard]] const std::vector<uint8_t>& data() const;

        [[nodiscard]] const std::vector<uint8_t>& mask() const;
    };
}

#endif //UNHIDENPCS_PATTERN_HPP
