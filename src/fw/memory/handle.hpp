#ifndef UNHIDENPCS_HANDLE_HPP
#define UNHIDENPCS_HANDLE_HPP
#pragma once

namespace memory
{
    class handle
    {
    private:
        uintptr_t pointer{};

    public:
        handle() = default;

        explicit handle(void* pointer);

        explicit handle(uint64_t pointer);

        handle(const handle& other);

        uintptr_t raw() const;

        handle add(ptrdiff_t offset) const;

        handle add(const handle& other) const;

        handle sub(ptrdiff_t offset) const;

        handle sub(const handle& other) const;

        handle rip();

        handle resolve_relative_call() const;

        template<typename T>
        std::enable_if_t<std::is_pointer_v<T>, T> to_ptr() const
        {
            return reinterpret_cast<T>(pointer);
        }

        template<typename T>
        std::enable_if_t<std::is_object_v<T>, T&> deref()
        {
            return *to_ptr<T*>();
        }

        bool operator==(const handle& other) const noexcept;

        bool operator==(uintptr_t other) const noexcept;

        bool operator<(const handle& other) const noexcept;

        bool operator<(uintptr_t other) const noexcept;

        bool operator<=(const handle& other) const noexcept;

        bool operator<=(uintptr_t other) const noexcept;

        bool operator>(const handle& other) const noexcept;

        bool operator>(uintptr_t other) const noexcept;

        bool operator>=(const handle& other) const noexcept;

        bool operator>=(uintptr_t other) const noexcept;
    };
}

#endif //UNHIDENPCS_HANDLE_HPP
