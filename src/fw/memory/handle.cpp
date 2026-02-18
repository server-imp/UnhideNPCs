#include "handle.hpp"

memory::Handle::Handle(void* pointer)
{
    this->_pointer = reinterpret_cast<uintptr_t>(pointer);
}

memory::Handle::Handle(const uint64_t pointer)
{
    this->_pointer = pointer;
}

memory::Handle::Handle(const Handle& other)
{
    this->_pointer = other._pointer;
}

uintptr_t memory::Handle::raw() const
{
    return this->_pointer;
}

memory::Handle memory::Handle::add(const ptrdiff_t offset) const
{
    return Handle(this->_pointer + offset);
}

memory::Handle memory::Handle::add(const Handle& other) const
{
    return Handle(this->_pointer + other._pointer);
}

memory::Handle memory::Handle::sub(const ptrdiff_t offset) const
{
    return Handle(this->_pointer - offset);
}

memory::Handle memory::Handle::sub(const Handle& other) const
{
    return Handle(this->_pointer - other._pointer);
}

memory::Handle memory::Handle::rip()
{
    return add(deref<int32_t>()).add(4);
}

memory::Handle memory::Handle::resolve_relative_call() const
{
    const auto offset          = add(1).deref<int32_t>();
    const auto nextInstruction = add(5);

    return nextInstruction.add(offset);
}

bool memory::Handle::operator==(const Handle& other) const noexcept
{
    return _pointer == other._pointer;
}

bool memory::Handle::operator==(const uintptr_t other) const noexcept
{
    return _pointer == other;
}

bool memory::Handle::operator!=(const uintptr_t other) const noexcept
{
    return _pointer != other;
}

bool memory::Handle::operator!=(const Handle& other) const noexcept
{
    return _pointer != other._pointer;
}

bool memory::Handle::operator<(const Handle& other) const noexcept
{
    return _pointer < other._pointer;
}

bool memory::Handle::operator<(const uintptr_t other) const noexcept
{
    return _pointer < other;
}

bool memory::Handle::operator<=(const Handle& other) const noexcept
{
    return _pointer <= other._pointer;
}

bool memory::Handle::operator<=(const uintptr_t other) const noexcept
{
    return _pointer <= other;
}

bool memory::Handle::operator>(const Handle& other) const noexcept
{
    return _pointer > other._pointer;
}

bool memory::Handle::operator>(const uintptr_t other) const noexcept
{
    return _pointer > other;
}

bool memory::Handle::operator>=(const Handle& other) const noexcept
{
    return _pointer >= other._pointer;
}

bool memory::Handle::operator>=(const uintptr_t other) const noexcept
{
    return _pointer >= other;
}
