#include "range.hpp"

memory::range::range(const char* moduleName)
{
    const auto hModule = GetModuleHandleA(moduleName);
    MODULEINFO moduleInfo{};

    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo)))
    {
        return;
    }

    this->_start = handle(moduleInfo.lpBaseOfDll);
    this->_end   = _start.add(moduleInfo.SizeOfImage);
    this->_size  = moduleInfo.SizeOfImage;
}

memory::range::range(const handle& start, const ptrdiff_t size) : _start(start), _size(size)
{
    _end = handle(start).add(size);
}

memory::range::range(const handle& start, const handle& end) : _start(start), _end(end)
{
    _size = static_cast<std::ptrdiff_t>(_start.raw()) - static_cast<std::ptrdiff_t>(_start.raw());
}

memory::range::range(const uintptr_t start, const ptrdiff_t size) : _start(start), _size(size)
{
    _end = handle(start).add(size);
}

const memory::handle& memory::range::start() const
{
    return _start;
}

const memory::handle& memory::range::end() const
{
    return _end;
}

const ptrdiff_t& memory::range::size() const
{
    return _size;
}

bool memory::range::contains(const handle& address) const
{
    return _start <= address && _end >= address;
}

bool memory::range::contains(const uintptr_t address) const
{
    return _start <= address && _end >= address;
}
