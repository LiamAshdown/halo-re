#pragma once

#include "halo/shell/platform.hpp"
#include "interface.h"

namespace halo::shell {

/**
 * The config.txt settings. Each property setter writes one global the renderer or sound code reads;
 * the setters return 1 when the value was accepted.
 */
class Config {
public:
    static uint8_t set_flag(int32_t *flag);
    static uint8_t set_float(float *target, const char *value);
    static uint8_t set_maximum_resolution(const char *value);
    static uint8_t set_force_shader(const char *value);
    static void reset_system_requirements();
    static uint8_t compute_uma_video_memory();
};

/**
 * The registry of config.txt properties: the table of property names and their setters that
 * config.txt lines are matched against.
 */
class ConfigPropertyTable {
public:
    static const shell_config_property *find(const char *name);
    static uint8_t apply(const shell_config_property *property, const char *value);
};

/**
 * The hardware requirements parser seen through its function table, so calls reach whatever
 * implementation the table holds.
 */
class HardwareRequirements {
public:
    explicit HardwareRequirements(hwreq_parser *value) : parser(value), table((hwreq_parser_vtable *)value->vtable) {}

    uint8_t parse(const char *path, shell_sound_device *sound_device, d3d_adapter_identifier9 *adapter, d3d_caps9 *caps,
                  uint32_t memory, uint32_t video_memory, uint32_t cpu_speed) const;
    char *error_message() const;
    char *graphics_vendor_name() const;
    char *graphics_device_name() const;
    uint32_t flag_count() const;
    char *flag_name(uint32_t index) const;
    char *flag_value(uint32_t index) const;
    uint32_t requirement_count() const;
    char *requirement_name(uint32_t index) const;
    char *requirement_value(uint32_t index) const;

    hwreq_parser *parser;
    hwreq_parser_vtable *table;
};

/**
 * Loads config.txt for an adapter and applies it to the configuration globals.
 */
class ConfigLoader {
public:
    static char *parse(uint32_t adapter_index, d3d9_interface *d3d);
};

}
