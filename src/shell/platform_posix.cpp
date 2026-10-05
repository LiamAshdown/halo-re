/**
 * @file src/shell/platform_posix.cpp
 * The settings store off Windows: the values the registry holds on Windows (the exit flag, the digital product id) in
 * a small file of name / type / size / bytes records. Machine and user settings share it. On Emscripten the file is
 * in the persistent home folder, so it survives a page reload.
 */

#include "halo/shell/platform.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace halo::shell {

namespace {

constexpr uint32_t k_reg_sz = 1;
constexpr uint32_t k_reg_dword = 4;
constexpr uint32_t k_max_values = 32;
constexpr uint32_t k_max_value_size = 512;

struct value_record {
    char name[64];
    uint32_t type;
    uint32_t size;
    uint8_t data[k_max_value_size];
};

const char *settings_path()
{
    static char path[512];

    if (path[0] == '\0') {
        const char *home = getenv("HOME");

        snprintf(path, sizeof(path), "%s/halo_settings.bin", home != nullptr ? home : ".");
    }
    return path;
}

uint32_t load(value_record *values)
{
    FILE *file = fopen(settings_path(), "rb");
    uint32_t count = 0;

    if (file == nullptr) {
        return 0;
    }
    while (count < k_max_values && fread(&values[count], sizeof(value_record), 1, file) == 1) {
        count++;
    }
    fclose(file);
    return count;
}

void store(const char *name, uint32_t type, const void *data, uint32_t size)
{
    value_record *values = static_cast<value_record *>(calloc(k_max_values, sizeof(value_record)));
    uint32_t count = load(values);
    uint32_t index = count;
    FILE *file;

    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(values[i].name, name) == 0) index = i;
    }
    if (index == k_max_values || size > k_max_value_size) {
        free(values);
        return;
    }
    memset(&values[index], 0, sizeof(value_record));
    strncpy(values[index].name, name, sizeof(values[index].name) - 1);
    values[index].type = type;
    values[index].size = size;
    memcpy(values[index].data, data, size);
    if (index == count) count++;
    file = fopen(settings_path(), "wb");
    if (file != nullptr) {
        fwrite(values, sizeof(value_record), count, file);
        fclose(file);
    }
    free(values);
}

class FileSettingsStore final : public SettingsStore {
public:
    bool read_value(SettingsScope, const char *name, uint32_t *type, void *data, uint32_t *size) const override
    {
        value_record *values = static_cast<value_record *>(calloc(k_max_values, sizeof(value_record)));
        uint32_t count = load(values);
        bool found = false;

        for (uint32_t i = 0; i < count && !found; i++) {
            if (strcmp(values[i].name, name) == 0 && values[i].size <= *size) {
                memcpy(data, values[i].data, values[i].size);
                *size = values[i].size;
                if (type != nullptr) *type = values[i].type;
                found = true;
            }
        }
        free(values);
        return found;
    }

    void write_string(SettingsScope, const char *name, const char *text, uint32_t size) const override
    {
        store(name, k_reg_sz, text, size);
    }

    void write_dword(SettingsScope, const char *name, uint32_t value) const override
    {
        store(name, k_reg_dword, &value, sizeof(value));
    }
};

constexpr FileSettingsStore k_file_settings_store{};

}  // namespace

const SettingsStore &SettingsStore::current()
{
    return k_file_settings_store;
}

}  // namespace halo::shell
