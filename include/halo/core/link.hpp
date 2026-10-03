/**
 * @file include/halo/core/link.hpp
 * Typed views of engine variables. A module publishes the addresses of its variables in halo::<module>::vars(); a file that
 * uses one binds a reference of the type it needs with ref<T>().
 */
#pragma once

namespace halo::link {

/** Reinterprets the address of an engine variable as a T (T may be an array or function-pointer type). */
template <class T> inline T &ref(void *address)
{
    return *static_cast<T *>(address);
}

}  // namespace halo::link
