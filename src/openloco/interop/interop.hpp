#pragma once

#include "../compat.h"
#include "../utility/string.hpp"
#include "i386.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

#if defined(__GNUC__)
#define FORCE_ALIGN_ARG_POINTER __attribute__((force_align_arg_pointer))
#else
#define FORCE_ALIGN_ARG_POINTER
#endif

namespace openloco::interop
{

#ifndef USE_MMAP
    constexpr uintptr_t GOOD_PLACE_FOR_DATA_SEGMENT = 0x008A4000;
#else
#if defined(PLATFORM_32BIT)
    constexpr uintptr_t GOOD_PLACE_FOR_DATA_SEGMENT = 0x09000000;
#elif defined(PLATFORM_64BIT)
    constexpr uintptr_t GOOD_PLACE_FOR_DATA_SEGMENT = 0x200000000;
#else
#error "Unknown platform"
#endif
#endif

    constexpr uintptr_t remap_address(uintptr_t locoAddress)
    {
        return GOOD_PLACE_FOR_DATA_SEGMENT - 0x008A4000 + locoAddress;
    }

    template<uint32_t TAddress, typename T>
    constexpr T& addr()
    {
        return *((T*)remap_address(TAddress));
    }

    /**
    * Returns the flags register
    *
    * Flags register is as follows:
    * 0bSZ0A_0P0C_0000_0000
    * S = Signed flag
    * Z = Zero flag
    * C = Carry flag
    * A = Adjust flag
    * P = Parity flag
    * All other bits are undefined.
    */
    int32_t call(int32_t address);
    int32_t call(int32_t address, registers& registers);

    template<typename T, uintptr_t TAddress>
    struct loco_global
    {
    public:
        typedef T type;
        typedef type* pointer;
        typedef type& reference;
        typedef const type& const_reference;

    private:
        pointer _Myptr;

    public:
        loco_global()
        {
            _Myptr = &(addr<TAddress, T>());
        }

        operator reference()
        {
            return addr<TAddress, T>();
        }

        loco_global& operator=(const_reference v)
        {
            addr<TAddress, T>() = v;
            return *this;
        }

        loco_global& operator+=(const_reference v)
        {
            addr<TAddress, T>() += v;
            return *this;
        }

        loco_global& operator|=(const_reference v)
        {
            addr<TAddress, T>() |= v;
            return *this;
        }

        loco_global& operator&=(const_reference v)
        {
            addr<TAddress, T>() &= v;
            return *this;
        }

        loco_global& operator-=(const_reference v)
        {
            addr<TAddress, T>() -= v;
            return *this;
        }

        loco_global& operator++()
        {
            addr<TAddress, T>()++;
            return *this;
        }

        T operator++(int)
        {
            reference ref = addr<TAddress, T>();
            T temp = ref;
            ref++;
            return temp;
        }

        loco_global& operator--()
        {
            addr<TAddress, T>()--;
            return *this;
        }

        T operator--(int)
        {
            reference ref = addr<TAddress, T>();
            T temp = ref;
            ref--;
            return temp;
        }

        reference operator*()
        {
            return addr<TAddress, T>();
        }

        pointer operator->()
        {
            return &(addr<TAddress, T>());
        }

        constexpr size_t size() const
        {
            return sizeof(T);
        }
    };

    template<typename T>
    struct loco_global_iterator
    {
    private:
        T* _ptr;

    public:
        loco_global_iterator(T* p)
            : _ptr(p)
        {
        }
        loco_global_iterator& operator++()
        {
            ++_ptr;
            return *this;
        }
        loco_global_iterator operator++(int)
        {
            auto temp = *this;
            ++_ptr;
            return temp;
        }
        loco_global_iterator& operator--()
        {
            --_ptr;
            return *this;
        }
        loco_global_iterator operator--(int)
        {
            auto temp = *this;
            --_ptr;
            return temp;
        }
        bool operator==(const loco_global_iterator& rhs)
        {
            return _ptr == rhs._ptr;
        }
        bool operator!=(const loco_global_iterator& rhs)
        {
            return _ptr != rhs._ptr;
        }
        T& operator*()
        {
            return *_ptr;
        }
    };

    template<typename T, size_t TCount, uintptr_t TAddress>
    struct loco_global<T[TCount], TAddress>
    {
    public:
        typedef T type;
        typedef type* pointer;
        typedef type& reference;
        typedef const type& const_reference;
        typedef loco_global_iterator<T> iterator;

    private:
        pointer _Myfirst;
        pointer _Mylast;

    public:
        loco_global()
        {
            _Myfirst = get();
            _Mylast = _Myfirst + TCount;
        }

        operator pointer()
        {
            return get();
        }

        pointer get() const
        {
            return reinterpret_cast<pointer>(&addr<TAddress, type>());
        }

        reference operator[](int idx)
        {
#ifndef NDEBUG
            if (idx < 0 || static_cast<size_t>(idx) >= size())
            {
                throw std::out_of_range("loco_global: bounds check violation!");
            }
#endif
            return get()[idx];
        }

        constexpr size_t size() const
        {
            return TCount;
        }

        iterator begin() const
        {
            return iterator(&addr<TAddress, T>());
        }

        iterator end() const
        {
            const pointer ptrEnd = (&addr<TAddress, T>()) + TCount;
            return iterator(ptrEnd);
        }
    };

    enum
    {
        X86_FLAG_CARRY = 1 << 0,
        X86_FLAG_PARITY = 1 << 2,
        X86_FLAG_ADJUST = 1 << 4,
        X86_FLAG_ZERO = 1 << 6,
        X86_FLAG_SIGN = 1 << 7,
    };

    class save_state
    {
    private:
        uintptr_t begin = 0;
        uintptr_t end = 0;
        std::vector<std::byte> state;

    public:
        const std::vector<std::byte>& get_state() const
        {
            return state;
        }

        save_state(uintptr_t begin, uintptr_t end);
        void reset();

        static void log_diff(const save_state& lhs, const save_state& rhs);
    };

    bool operator==(const save_state& lhs, const save_state& rhs);
    bool operator!=(const save_state& lhs, const save_state& rhs);

    void read_memory(uint32_t address, void* data, size_t size);
    void write_memory(uint32_t address, const void* data, size_t size);

    void register_hooks();
    void load_sections();
}

// these safe string function convenience overloads are located in this header, rather than in utility/string.hpp,
// so that utility/string.hpp doesn't needlessly have to include this header just for the definition of loco_global
// (and so that we don't have to use type traits SFINAE template wizardry to get around not having the definition available)
namespace openloco::utility
{
    template<size_t TCount, uintptr_t TAddress>
    void strcpy_safe(openloco::interop::loco_global<char[TCount], TAddress>& dest, const char* src)
    {
        (void)strlcpy(dest, src, dest.size());
    }

    template<size_t TCount, uintptr_t TAddress>
    void strcat_safe(openloco::interop::loco_global<char[TCount], TAddress>& dest, const char* src)
    {
        (void)strlcat(dest, src, dest.size());
    }

    template<size_t TCount, uintptr_t TAddress, typename... Args>
    int sprintf_safe(openloco::interop::loco_global<char[TCount], TAddress>& dest, const char* fmt, Args&&... args)
    {
        return std::snprintf(dest, TCount, fmt, std::forward<Args>(args)...);
    }
}
