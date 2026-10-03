#include "halo/shell/config.hpp"
#include "halo/shell/diagnostics.hpp"
#include "halo/shell/hwreq.hpp"

extern "C" {
extern int32_t config_maximum_resolution;
extern int32_t config_linear_texture_addressing;
extern int32_t config_linear_texture_addressing_zoom;
extern int32_t config_linear_texture_addressing_sun;
extern int32_t config_use_fixed_function;
extern int32_t config_disable_driver_management;
extern int32_t config_unsupported_card;
extern int32_t config_prototype_card;
extern int32_t config_old_driver;
extern int32_t config_old_sound_driver;
extern int32_t config_invalid_driver;
extern int32_t config_invalid_sound_driver;
extern int32_t config_disable_buffering;
extern int32_t config_enable_stop_start;
extern int32_t config_head_relative_speech;
extern int32_t config_safe_mode;
extern int32_t config_force_shader;
extern int32_t config_use_anisotropic_filter;
extern int32_t config_disable_specular;
extern int32_t config_disable_render_targets;
extern int32_t config_disable_alpha_render_targets;
extern int32_t config_use_alternate_convolve_mask;
extern int32_t config_min_max_blend_op_is_broken;
extern float config_decal_z_bias;
extern float config_transparent_decal_z_bias;
extern float config_decal_slope_z_bias;
extern float config_transparent_decal_slope_z_bias;

extern uint32_t physical_memory;
extern uint32_t video_memory;
extern large_integer graphics_driver_version;
extern uint32_t display_adapter_count;
extern shell_display_adapter display_adapters[k_shell_maximum_display_adapters];
extern hwreq_parser *hardware_requirements;
extern shell_sound_device sound_devices[k_shell_maximum_sound_devices];
extern int32_t selected_sound_device;
extern uint32_t cpu_speed;
extern char *graphics_vendor_name;
extern char *graphics_device_name;
extern uint32_t graphics_device_id;
extern uint32_t graphics_vendor_id;
extern shell_config_property config_properties[k_shell_config_property_count];
extern char config_unknown_property_text[k_shell_config_message_length];
extern char config_error_text[k_shell_config_message_length];
extern int32_t required_cpu_speed;
extern int32_t required_memory;
extern int32_t required_video_memory;
extern int32_t required_directx_build;
extern int32_t required_disk_space;
extern int32_t safe_mode;
}

namespace halo::shell {

/**
 * Presence setter: the property has no value, naming it in config.txt sets the flag.
 */
uint8_t Config::set_flag(int32_t *flag)
{
    *flag = 1;
    return 1;
}

/**
 * Parses a float and stores it unconditionally.
 */
uint8_t Config::set_float(float *target, const char *value)
{
    sscanf(value, "%f", target);
    return 1;
}

/**
 * MaximumResolution: parses a decimal number and stores it when it lies within [0x280, 0x1000];
 * otherwise the current value stays. Returns 1 when stored, 0 when out of range.
 *
 * @address 0x57d080
 */
uint8_t Config::set_maximum_resolution(const char *value)
{
    uint32_t parsed;

    sscanf(value, "%d", &parsed);
    if (parsed >= k_shell_config_maximum_resolution_minimum && parsed <= k_shell_config_maximum_resolution_default) {
        config_maximum_resolution = parsed;
        return 1;
    }
    return 0;
}

/**
 * ForceShader: "2a" or "2A" forces shader 9998; otherwise a decimal shader id, with 9999 when the
 * parse produces zero. When nothing is parsed the result keeps the low 32 bits of the value pointer, as
 * the original did.
 *
 * @address 0x57d0c0
 */
uint8_t Config::set_force_shader(const char *value)
{
    int32_t parsed = (int32_t)(uintptr_t)value;

    if (*(const uint16_t *)value == 0x6132 || *(const uint16_t *)value == 0x4132) {
        config_force_shader = 0x270e;
    } else {
        sscanf(value, "%d", &parsed);
        config_force_shader = parsed;
        if (parsed == 0) {
            config_force_shader = 0x270f;
            return 1;
        }
    }
    return 1;
}

/**
 * Resets every config.txt controlled global to its default: MaximumResolution to 0x1000, every flag
 * to 0, the decal z-bias floats to their bit-exact defaults and the slope biases to 0.
 *
 * @address 0x57cfe0
 */
void Config::reset_system_requirements()
{
    uint32_t bits;

    config_maximum_resolution = k_shell_config_maximum_resolution_default;
    config_linear_texture_addressing = 0;
    config_linear_texture_addressing_zoom = 0;
    config_linear_texture_addressing_sun = 0;
    config_use_fixed_function = 0;
    config_prototype_card = 0;
    config_disable_driver_management = 0;
    config_use_anisotropic_filter = 0;
    config_disable_specular = 0;
    config_unsupported_card = 0;
    config_enable_stop_start = 0;
    config_head_relative_speech = 0;
    config_disable_render_targets = 0;
    config_disable_alpha_render_targets = 0;
    config_use_alternate_convolve_mask = 0;
    config_old_driver = 0;
    config_old_sound_driver = 0;
    config_invalid_driver = 0;
    config_invalid_sound_driver = 0;
    config_safe_mode = 0;
    config_force_shader = 0;
    config_min_max_blend_op_is_broken = 0;
    config_disable_buffering = 0;

    bits = 0xb866afcd;
    config_decal_z_bias = *(float *)&bits;
    config_decal_slope_z_bias = 0.0f;
    bits = 0xb6a7c5ac;
    config_transparent_decal_z_bias = *(float *)&bits;
    config_transparent_decal_slope_z_bias = 0.0f;
}

/**
 * Derives the default video memory size of shared-memory graphics from the installed physical
 * memory: 8 MB below 64 MB of RAM, 16 MB below 128 MB, 32 MB below 256 MB, else 64 MB.
 *
 * @address 0x57d250
 */
uint8_t Config::compute_uma_video_memory()
{
    video_memory = 0x800000;
    if (physical_memory > 0x3f) {
        video_memory = 0x1000000;
    }
    if (physical_memory > 0x7f) {
        video_memory = 0x2000000;
    }
    if (physical_memory > 0xff) {
        video_memory = 0x4000000;
    }
    return 1;
}

/**
 * Looks name up case-insensitively in the property table. Returns the entry, or null.
 */
const shell_config_property *ConfigPropertyTable::find(const char *name)
{
    uint32_t property_index;

    for (property_index = 0; property_index < k_shell_config_property_count; property_index++) {
        if (_stricmp((const char *)config_properties[property_index].name, name) == 0) {
            return &config_properties[property_index];
        }
    }
    return 0;
}

/**
 * Runs the property's setter on value. A setter that faults counts as a failure. Returns the setter's
 * result.
 */
uint8_t ConfigPropertyTable::apply(const shell_config_property *property, const char *value)
{
    uint8_t ok;

    __try {
        ok = ((shell_config_property_setter)property->setter)(value);
    } __except (1) {
        ok = 0;
    }
    return ok;
}

/**
 * Runs the parser's parse function through its function table.
 */
uint8_t HardwareRequirements::parse(const char *path, shell_sound_device *sound_device, d3d_adapter_identifier9 *adapter,
                                    d3d_caps9 *caps, uint32_t memory, uint32_t video_memory, uint32_t cpu_speed) const
{
    return ((hwreq_parse_fn)table->parse)(parser, path, sound_device, adapter, caps, memory, video_memory, cpu_speed);
}

/**
 * Character data of the parser's error message.
 */
char *HardwareRequirements::error_message() const
{
    return ((hwreq_get_string_fn)table->get_error_message)(parser);
}

/**
 * Name of the graphics vendor the parser matched.
 */
char *HardwareRequirements::graphics_vendor_name() const
{
    return ((hwreq_get_string_fn)table->get_graphics_vendor_name)(parser);
}

/**
 * Name of the graphics device the parser matched.
 */
char *HardwareRequirements::graphics_device_name() const
{
    return ((hwreq_get_string_fn)table->get_graphics_device_name)(parser);
}

/**
 * Number of flag = value lines the parser collected.
 */
uint32_t HardwareRequirements::flag_count() const
{
    return ((hwreq_get_count_fn)table->get_flag_count)(parser);
}

/**
 * Name of flag index.
 */
char *HardwareRequirements::flag_name(uint32_t index) const
{
    return ((hwreq_get_indexed_string_fn)table->get_flag_name)(parser, index);
}

/**
 * Value of flag index.
 */
char *HardwareRequirements::flag_value(uint32_t index) const
{
    return ((hwreq_get_indexed_string_fn)table->get_flag_value)(parser, index);
}

/**
 * Number of lines in the Requirements section.
 */
uint32_t HardwareRequirements::requirement_count() const
{
    return ((hwreq_get_count_fn)table->get_requirement_count)(parser);
}

/**
 * Name of requirement index.
 */
char *HardwareRequirements::requirement_name(uint32_t index) const
{
    return ((hwreq_get_indexed_string_fn)table->get_requirement_name)(parser, index);
}

/**
 * Value of requirement index.
 */
char *HardwareRequirements::requirement_value(uint32_t index) const
{
    return ((hwreq_get_indexed_string_fn)table->get_requirement_value)(parser, index);
}

/**
 * Loads and parses config.txt against the given Direct3D adapter: queries the adapter identity and
 * caps, parses the script, applies every flag through the property table (a fatal error on parse
 * failure), forces the fixed safe mode configuration when -safemode is active and reads the
 * Requirements section into the required_* globals. Returns null on success, or a message describing
 * an unknown or malformed property.
 *
 * @address 0x57d410
 */
char *ConfigLoader::parse(uint32_t adapter_index, d3d9_interface *d3d)
{
    d3d_adapter_identifier9 identifier;
    d3d_caps9 caps;
    uint32_t i;
    uint32_t flag_count;
    uint32_t requirement_count;
    char *name;
    char *value;
    const shell_config_property *property;
    uint8_t ok;
    int32_t directx_scratch;
    void **d3d_vtable;

    d3d_vtable = (void **)d3d->vtable;
    ((d3d9_get_adapter_identifier_fn)d3d_vtable[5])(d3d, adapter_index, 0, &identifier);
    ((d3d9_get_device_caps_fn)d3d_vtable[14])(d3d, adapter_index, 1, &caps);

    graphics_driver_version = identifier.driver_version;
    video_memory = display_adapters[0].video_memory;

    for (i = 0; i < display_adapter_count; i++) {
        if (_stricmp(display_adapters[i].driver_name, identifier.device_name) == 0) {
            video_memory = display_adapters[i].video_memory;
        }
    }

    hardware_requirements = HwreqParser::create();
    HardwareRequirements requirements(hardware_requirements);

    ok = requirements.parse("config.txt", &sound_devices[selected_sound_device], &identifier, &caps, physical_memory,
                            video_memory, cpu_speed);
    if (ok == 0) {
        FatalError::show(0xffffffff, (uint32_t)requirements.error_message(), 1);
    }

    graphics_vendor_name = requirements.graphics_vendor_name();
    graphics_device_name = requirements.graphics_device_name();
    graphics_device_id = identifier.device_id;
    graphics_vendor_id = identifier.vendor_id;

    Config::reset_system_requirements();

    flag_count = requirements.flag_count();
    for (i = 0; i < flag_count; i++) {
        name = requirements.flag_name(i);
        value = requirements.flag_value(i);

        property = ConfigPropertyTable::find(name);
        if (property == 0) {
            sprintf(config_unknown_property_text, "Unknown property in config.txt '%s'", name);
            return config_unknown_property_text;
        }

        ok = ConfigPropertyTable::apply(property, value);
        if (!ok) {
            sprintf(config_error_text, "Error in config.txt '%s'='%s'", name, value);
            return config_error_text;
        }
    }

    if (safe_mode != 0) {
        config_use_fixed_function = 1;
        config_disable_driver_management = 1;
        config_disable_buffering = 1;
        config_force_shader = 9999;
        config_min_max_blend_op_is_broken = 1;
        config_maximum_resolution = 800;
        config_disable_render_targets = 1;
        config_disable_specular = 1;
    }

    required_cpu_speed = k_shell_required_cpu_speed_default;
    required_memory = k_shell_required_memory_default;
    required_video_memory = k_shell_required_video_memory_default;
    required_directx_build = k_shell_required_directx_build_default;
    required_disk_space = k_shell_required_disk_space_default;

    requirement_count = requirements.requirement_count();
    for (i = 0; i < requirement_count; i++) {
        name = requirements.requirement_name(i);
        value = requirements.requirement_value(i);

        if (_stricmp(name, "CpuSpeed") == 0) {
            sscanf(value, "%d", &required_cpu_speed);
        }
        if (_stricmp(name, "Memory") == 0) {
            sscanf(value, "%d", &required_memory);
        }
        if (_stricmp(name, "VideoMemory") == 0) {
            sscanf(value, "%d", &required_video_memory);
        }
        if (_stricmp(name, "DirectX") == 0) {
            sscanf(value, "%d.%d.%d.%d", &directx_scratch, &directx_scratch, &directx_scratch, &required_directx_build);
        }
        if (_stricmp(name, "DiskSpace") == 0) {
            sscanf(value, "%d", &required_disk_space);
        }
    }

    return 0;
}

}
