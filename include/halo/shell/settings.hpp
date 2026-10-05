#pragma once

/**
 * @file include/halo/shell/settings.hpp
 * The game's persistent settings store. Self-contained (no shell types), so any module can include it.
 */

#include <cstdint>

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
     * Writes a 32-bit number value, creating the store if needed.
     */
    virtual void write_dword(SettingsScope scope, const char *name, uint32_t value) const = 0;

    /**
     * The settings store of the running platform.
     */
    static const SettingsStore &current();
};

}  // namespace halo::shell
