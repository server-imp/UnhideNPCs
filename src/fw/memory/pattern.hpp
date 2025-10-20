#ifndef UNHIDENPCS_PATTERN_HPP
#define UNHIDENPCS_PATTERN_HPP
#pragma once

namespace memory
{
    class pattern
    {
    private:
        std::vector<uint8_t> _data;
        std::vector<uint8_t> _mask;

    public:
        explicit pattern(const std::vector<uint8_t>& data, const std::vector<uint8_t>& mask);

        explicit pattern(const std::string& ida);

        const std::vector<uint8_t>& data() const;

        const std::vector<uint8_t>& mask() const;
    };
}

#endif //UNHIDENPCS_PATTERN_HPP
