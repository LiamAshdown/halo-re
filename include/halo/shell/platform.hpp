#pragma once

#include "halo/shell/types.hpp"
#include "halo/shell/settings.hpp"

namespace halo::shell {

/**
 * Owning wrapper around an open registry key; the key is closed when the wrapper goes out of scope.
 */
class RegistryKey {
public:
    RegistryKey() : handle(0) {}
    ~RegistryKey() { close(); }
    RegistryKey(const RegistryKey &) = delete;
    RegistryKey &operator=(const RegistryKey &) = delete;

    uint32_t open(void *hive, const char *subkey, uint32_t access);
    uint32_t create(void *hive, const char *subkey, uint32_t options, uint32_t access);
    uint32_t query(const char *name, uint32_t *type, void *data, uint32_t *size) const;
    uint32_t set_string(const char *name, const char *text, uint32_t size) const;
    uint32_t set_dword(const char *name, uint32_t value) const;
    void close();

    void *handle;
};

/**
 * Owning wrapper around a loaded dynamic library; the library is released when the wrapper goes out of scope.
 */
class DynamicLibrary {
public:
    explicit DynamicLibrary(const char *file_name);
    ~DynamicLibrary();
    DynamicLibrary(const DynamicLibrary &) = delete;
    DynamicLibrary &operator=(const DynamicLibrary &) = delete;

    bool loaded() const { return module != 0; }
    void *symbol(const char *name) const;

    void *module;
};

}
