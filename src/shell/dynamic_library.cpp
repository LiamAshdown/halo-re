/**
 * @file src/shell/dynamic_library.cpp
 * DynamicLibrary (include/halo/shell/platform.hpp) on halo::platform's library calls.
 */

#include "halo/shell/platform.hpp"
#include "halo/platform/system.hpp"

namespace halo::shell {

/**
 * Loads the library; check loaded() for the result.
 */
DynamicLibrary::DynamicLibrary(const char *file_name)
    : module(halo::platform::library_open(file_name))
{
}

/**
 * Releases the library if it was loaded.
 */
DynamicLibrary::~DynamicLibrary()
{
    if (module != 0) {
        halo::platform::library_close(module);
    }
}

/**
 * Address of an exported function, or null.
 */
void *DynamicLibrary::symbol(const char *name) const
{
    return (void *)halo::platform::library_symbol(module, name);
}

}  // namespace halo::shell
