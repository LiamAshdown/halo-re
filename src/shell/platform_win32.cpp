#include "halo/shell/platform.hpp"
#include "halo/shell/layout.hpp"
#include "halo/platform/system.hpp"

namespace halo::shell {

namespace {

const char *const k_settings_path = "Software\\Microsoft\\Microsoft Games\\Halo";

void *hive_of(SettingsScope scope)
{
    return scope == SettingsScope::machine ? (void *)k_hkey_local_machine : (void *)k_hkey_current_user;
}

/**
 * Settings store backed by the Windows registry.
 */
class RegistrySettingsStore final : public SettingsStore {
public:
    /**
     * Opens the settings key for reading and queries one value; the key is closed again before returning.
     */
    bool read_value(SettingsScope scope, const char *name, uint32_t *type, void *data, uint32_t *size) const override
    {
        RegistryKey key;

        key.open(hive_of(scope), k_settings_path, k_key_read);
        return key.query(name, type, data, size) == 0;
    }

    /**
     * Creates the settings key and sets one REG_SZ value. The key is created with the argument order of
     * the original call: the 0x20006 mask lands in the options slot and the access mask stays zero.
     */
    void write_string(SettingsScope scope, const char *name, const char *text, uint32_t size) const override
    {
        RegistryKey key;

        key.create(hive_of(scope), k_settings_path, k_key_write, 0);
        key.set_string(name, text, size);
    }

    /**
     * Creates the settings key with write access and sets one REG_DWORD value.
     */
    void write_dword(SettingsScope scope, const char *name, uint32_t value) const override
    {
        RegistryKey key;

        key.create(hive_of(scope), k_settings_path, 0, k_key_write);
        key.set_dword(name, value);
    }
};

constexpr RegistrySettingsStore k_registry_settings_store{};

}

/**
 * The settings store of the running platform.
 */
const SettingsStore &SettingsStore::current()
{
    return k_registry_settings_store;
}

/**
 * Opens an existing key below the given hive; the wrapper stays empty when the call fails.
 */
uint32_t RegistryKey::open(void *hive, const char *subkey, uint32_t access)
{
    uint32_t result;

    close();
    result = RegOpenKeyExA((HKEY)hive, subkey, 0, access, (PHKEY)&handle);
    if (result != 0) {
        handle = 0;
    }
    return result;
}

/**
 * Creates (or opens) a key below the given hive; the wrapper stays empty when the call fails.
 */
uint32_t RegistryKey::create(void *hive, const char *subkey, uint32_t options, uint32_t access)
{
    uint32_t result;

    close();
    result = RegCreateKeyExA((HKEY)hive, subkey, 0, 0, options, access, 0, (PHKEY)&handle, 0);
    if (result != 0) {
        handle = 0;
    }
    return result;
}

/**
 * Reads one value of the open key.
 */
uint32_t RegistryKey::query(const char *name, uint32_t *type, void *data, uint32_t *size) const
{
    return RegQueryValueExA((HKEY)handle, name, 0, (LPDWORD)type, (uint8_t *)data, (LPDWORD)size);
}

/**
 * Sets one REG_SZ value of the open key; size counts the terminating NUL.
 */
uint32_t RegistryKey::set_string(const char *name, const char *text, uint32_t size) const
{
    return RegSetValueExA((HKEY)handle, name, 0, 1, (const uint8_t *)text, size);
}

/**
 * Sets one REG_DWORD value of the open key.
 */
uint32_t RegistryKey::set_dword(const char *name, uint32_t value) const
{
    return RegSetValueExA((HKEY)handle, name, 0, REG_DWORD, (const uint8_t *)&value, sizeof(value));
}

/**
 * Closes the key if one is open.
 */
void RegistryKey::close()
{
    if (handle != 0) {
        RegCloseKey((HKEY)handle);
        handle = 0;
    }
}

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

}
