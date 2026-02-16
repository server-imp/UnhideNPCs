#include "handle.hpp"

memory::handle::handle(void* pointer)
{
    this->pointer = reinterpret_cast<uintptr_t>(pointer);
}

memory::handle::handle(const uint64_t pointer)
{
    this->pointer = pointer;
}

memory::handle::handle(const handle& other)
{
    this->pointer = other.pointer;
}

uintptr_t memory::handle::raw() const
{
    return this->pointer;
}

memory::handle memory::handle::add(const ptrdiff_t offset) const
{
    return handle(this->pointer + offset);
}

memory::handle memory::handle::add(const handle& other) const
{
    return handle(this->pointer + other.pointer);
}

memory::handle memory::handle::sub(const ptrdiff_t offset) const
{
    return handle(this->pointer - offset);
}

memory::handle memory::handle::sub(const handle& other) const
{
    return handle(this->pointer - other.pointer);
}

memory::handle memory::handle::rip()
{
    return add(deref<int32_t>()).add(4);
}

memory::handle memory::handle::resolve_relative_call() const
{
    const auto offset          = add(1).deref<int32_t>();
    const auto nextInstruction = add(5);

    return nextInstruction.add(offset);
}

bool memory::handle::operator==(const handle& other) const noexcept
{
    return pointer == other.pointer;
}

bool memory::handle::operator==(const uintptr_t other) const noexcept
{
    return pointer == other;
}

bool memory::handle::operator!=(const uintptr_t other) const noexcept
{
    return pointer != other;
}

bool memory::handle::operator!=(const handle& other) const noexcept
{
    return pointer != other.pointer;
}

bool memory::handle::operator<(const handle& other) const noexcept
{
    return pointer < other.pointer;
}

bool memory::handle::operator<(const uintptr_t other) const noexcept
{
    return pointer < other;
}

bool memory::handle::operator<=(const handle& other) const noexcept
{
    return pointer <= other.pointer;
}

bool memory::handle::operator<=(const uintptr_t other) const noexcept
{
    return pointer <= other;
}

bool memory::handle::operator>(const handle& other) const noexcept
{
    return pointer > other.pointer;
}

bool memory::handle::operator>(const uintptr_t other) const noexcept
{
    return pointer > other;
}

bool memory::handle::operator>=(const handle& other) const noexcept
{
    return pointer >= other.pointer;
}

bool memory::handle::operator>=(const uintptr_t other) const noexcept
{
    return pointer >= other;
}
