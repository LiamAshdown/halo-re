#pragma once

#include "halo/shell/types.hpp"

namespace halo::shell {

/**
 * Where a persistent setting lives: the machine wide store or the current user's store.
 */
enum class SettingsScope : uint8_t {
    machine,
    user
};

/**
 * Persistent settings of the game (language, first run flag, exit flag, remembered dialog answers).
 * The Windows implementation keeps them under the registry key Software\Microsoft\Microsoft Games\Halo;
 * other platforms can supply their own store behind the same interface.
 */
class SettingsStore {
public:
    /**
     * Reads the value called name. On success the data buffer receives the bytes, size the byte count
     * and, when not null, type the platform's value type. Returns false when the value is missing or
     * does not fit.
     */
    virtual bool read_value(SettingsScope scope, const char *name, uint32_t *type, void *data, uint32_t *size) const = 0;

    /**
     * Writes a string value (size includes the terminating NUL), creating the store if needed.
     */
    virtual void write_string(SettingsScope scope, const char *name, const char *text, uint32_t size) const = 0;

    /**
     * The settings store of the running platform.
     */
    static const SettingsStore &current();
};

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
