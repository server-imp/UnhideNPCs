#include "pattern.hpp"
#include "../logger.hpp"
#include "fw/util.hpp"

memory::pattern::pattern(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& mask
) : _data(data), _mask(mask) {}

memory::pattern::pattern(const std::string& ida)
{
    if (ida.empty())
    {
        LOG_DBG("Empty pattern");
        return;
    }

    std::istringstream ss(ida);
    std::string        token;

    while (ss >> token)
    {
        if (token == "??" || token == "?")
        {
            _data.push_back(0x00);
            _mask.push_back(0x00);
        }
        else
        {
            _data.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
            _mask.push_back(0xFF);
        }
    }
}

const std::vector<uint8_t>& memory::pattern::data() const
{
    return _data;
}

const std::vector<uint8_t>& memory::pattern::mask() const
{
    return _mask;
}
