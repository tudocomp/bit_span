#pragma once

#include <tudocomp/ds/dynamic_t.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/util/metaprogramming.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/IntRepr.hpp>

namespace tdc {namespace cbp {

template<typename T>
struct cbp_width_trait{
    class type {
    public:
        constexpr type(uint8_t) {}
        constexpr uint8_t get_width() const {
            return sizeof(T) * CHAR_BIT;
        }
        constexpr bool needs_alignment() const {
            return true;
        }
    };
};

template<size_t N>
struct cbp_width_trait<uint_impl_t<N>>{
    class type {
    public:
        constexpr type(uint8_t) {}
        constexpr uint8_t get_width() const {
            return N;
        }
        constexpr bool needs_alignment() const {
            return false;
        }
    };
};

template<>
struct cbp_width_trait<dynamic_t>{
    class type {
        uint8_t m_width;
    public:
        constexpr type(uint8_t w): m_width(w) {}
        constexpr uint8_t get_width() const {
            return m_width;
        }
        constexpr bool needs_alignment() const {
            return false;
        }
    };
};

template<typename T, typename X = tdc::void_t<>>
struct cbp_pointer_trait_t {
    using pointer_t = T*;
    using reference_t = T&;

    static constexpr pointer_t construct_relative_to(uint64_t* base_ptr,
                                                     uint64_t bit_offset,
                                                     uint64_t bit_element_size) {
        DCHECK_EQ(bit_offset % 8, 0);
        DCHECK_EQ(bit_element_size % 8, 0);
        auto offset = bit_offset / 8;
        return reinterpret_cast<T*>(reinterpret_cast<char*>(base_ptr) + offset);
    }

    static inline void call_destructor(pointer_t p) {
        p->~T();
    }

    static inline void construct_val_from_ptr(pointer_t dst, pointer_t src) {
        new(dst) T(std::move(*src));
    }
    static inline void construct_val_from_rval(pointer_t dst, T&& src) {
        new(dst) T(std::move(src));
    }
};

template<typename T>
struct cbp_pointer_trait_t<T, tdc::void_t<typename int_vector::IntRepr<T>::IntPtrBase>> {
    using pointer_t = int_vector::IntPtr<T>;
    using reference_t = int_vector::IntRef<T>;

    static constexpr pointer_t construct_relative_to(uint64_t* base_ptr,
                                                     uint64_t bit_offset,
                                                     uint64_t bit_element_size) {
        // Find the uint64_t in which the int pointer starts
        base_ptr += bit_offset / 64;
        bit_offset = bit_offset % 64;

        using ptr_base_t = int_vector::IntPtrBase<pointer_t>;

        auto int_ptr_base = ptr_base_t(base_ptr, bit_offset, bit_element_size);
        auto int_ptr = pointer_t(int_ptr_base);

        return int_ptr;
    }

    static inline void call_destructor(pointer_t p) {
        // NOP (also not implementable as is)
    }
    static inline void construct_val_from_ptr(pointer_t dst, pointer_t src) {
        *dst = *src;
    }
    static inline void construct_val_from_rval(pointer_t dst, T&& src) {
        *dst = src;
    }
};

template<typename T>
using cbp_width_t = typename cbp_width_trait<T>::type;

template<typename T>
using cbp_pointer_t = typename cbp_pointer_trait_t<T>::pointer_t;

template<typename T>
using cbp_reference_t = typename cbp_pointer_trait_t<T>::reference_t;

template<typename T>
inline void call_destructor(cbp_pointer_t<T> ptr) {
    cbp_pointer_trait_t<T>::call_destructor(ptr);
}
template<typename T>
inline void construct_val_from_ptr(cbp_pointer_t<T> dst, cbp_pointer_t<T> src) {
    cbp_pointer_trait_t<T>::construct_val_from_ptr(dst, src);
}
template<typename T>
inline void construct_val_from_rval(cbp_pointer_t<T> dst, T&& src) {
    cbp_pointer_trait_t<T>::construct_val_from_rval(dst, std::move(src));
}

}}