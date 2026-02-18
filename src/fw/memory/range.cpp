#include "range.hpp"

memory::Range::Range(const char* moduleName)
{
    const auto hModule = GetModuleHandleA(moduleName);
    MODULEINFO moduleInfo {};

    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo)))
    {
        return;
    }

    this->_start = Handle(moduleInfo.lpBaseOfDll);
    this->_end   = _start.add(moduleInfo.SizeOfImage);
    this->_size  = moduleInfo.SizeOfImage;
}

memory::Range::Range(const Handle& start, const ptrdiff_t size) : _start(start), _size(size)
{
    _end = Handle(start).add(size);
}

memory::Range::Range(const Handle& start, const Handle& end) : _start(start), _end(end)
{
    _size = static_cast<std::ptrdiff_t>(_end.raw()) - static_cast<std::ptrdiff_t>(_start.raw());
}

memory::Range::Range(const uintptr_t start, const ptrdiff_t size) : _start(start), _size(size)
{
    _end = Handle(start).add(size);
}

const memory::Handle& memory::Range::start() const
{
    return _start;
}

const memory::Handle& memory::Range::end() const
{
    return _end;
}

const ptrdiff_t& memory::Range::size() const
{
    return _size;
}

bool memory::Range::contains(const Handle& address) const
{
    return _start <= address && address < _end.raw();
}

bool memory::Range::contains(const uintptr_t address) const
{
    return _start <= address && address < _end.raw();
}
