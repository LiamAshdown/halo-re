/**
 * @file include/halo/core/com.hpp
 * Access to the method table (vtable) of a COM object that is held as a plain pointer or as a 32-bit handle.
 */
#pragma once

#include <cstdint>
#include <type_traits>

namespace halo {

/** The method table (first dword) of a COM object held as a pointer or as a 32-bit handle. */
template <typename T>
inline void **com_methods(T object) noexcept
{
    if constexpr (std::is_pointer_v<T>) {
        return *reinterpret_cast<void ***>(const_cast<std::remove_cv_t<std::remove_pointer_t<T>> *>(object));
    } else {
        return *reinterpret_cast<void ***>(static_cast<uintptr_t>(object));
    }
}

/** Method of a COM object by its method table slot (byte offset / 4), typed as the given function pointer. */
template <typename Fn, typename T>
inline Fn com_method(T object, uint32_t slot) noexcept
{
    return reinterpret_cast<Fn>(com_methods(object)[slot]);
}

}  // namespace halo
