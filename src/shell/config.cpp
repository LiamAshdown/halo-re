#include "halo/shell/config.hpp"
#include "halo/shell/layout.hpp"
#include "halo/shell/diagnostics.hpp"
#include "halo/shell/hwreq.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/shell/api.hpp"

static auto &config_maximum_resolution = halo::link::ref<int32_t>(halo::shell::vars().config_maximum_resolution);
static auto &config_linear_texture_addressing = halo::link::ref<int32_t>(halo::shell::vars().config_linear_texture_addressing);
static auto &config_linear_texture_addressing_zoom = halo::link::ref<int32_t>(halo::shell::vars().config_linear_texture_addressing_zoom);
static auto &config_linear_texture_addressing_sun = halo::link::ref<int32_t>(halo::shell::vars().config_linear_texture_addressing_sun);
static auto &config_use_fixed_function = halo::link::ref<int32_t>(halo::shell::vars().config_use_fixed_function);
static auto &config_disable_driver_management = halo::link::ref<int32_t>(halo::shell::vars().config_disable_driver_management);
static auto &config_unsupported_card = halo::link::ref<int32_t>(halo::shell::vars().config_unsupported_card);
static auto &config_prototype_card = halo::link::ref<int32_t>(halo::shell::vars().config_prototype_card);
static auto &config_old_driver = halo::link::ref<int32_t>(halo::shell::vars().config_old_driver);
static auto &config_old_sound_driver = halo::link::ref<int32_t>(halo::shell::vars().config_old_sound_driver);
static auto &config_invalid_driver = halo::link::ref<int32_t>(halo::shell::vars().config_invalid_driver);
static auto &config_invalid_sound_driver = halo::link::ref<int32_t>(halo::shell::vars().config_invalid_sound_driver);
static auto &config_disable_buffering = halo::link::ref<int32_t>(halo::shell::vars().config_disable_buffering);
static auto &config_enable_stop_start = halo::link::ref<int32_t>(halo::shell::vars().config_enable_stop_start);
static auto &config_head_relative_speech = halo::link::ref<int32_t>(halo::shell::vars().config_head_relative_speech);
static auto &config_safe_mode = halo::link::ref<int32_t>(halo::shell::vars().config_safe_mode);
static auto &config_force_shader = halo::link::ref<int32_t>(halo::shell::vars().config_force_shader);
static auto &config_use_anisotropic_filter = halo::link::ref<int32_t>(halo::shell::vars().config_use_anisotropic_filter);
static auto &config_disable_specular = halo::link::ref<int32_t>(halo::shell::vars().config_disable_specular);
static auto &config_disable_render_targets = halo::link::ref<int32_t>(halo::shell::vars().config_disable_render_targets);
static auto &config_disable_alpha_render_targets = halo::link::ref<int32_t>(halo::shell::vars().config_disable_alpha_render_targets);
static auto &config_use_alternate_convolve_mask = halo::link::ref<int32_t>(halo::shell::vars().config_use_alternate_convolve_mask);
static auto &config_min_max_blend_op_is_broken = halo::link::ref<int32_t>(halo::shell::vars().config_min_max_blend_op_is_broken);
static auto &config_decal_z_bias = halo::link::ref<float>(halo::shell::vars().config_decal_z_bias);
static auto &config_transparent_decal_z_bias = halo::link::ref<float>(halo::shell::vars().config_transparent_decal_z_bias);
static auto &config_decal_slope_z_bias = halo::link::ref<float>(halo::shell::vars().config_decal_slope_z_bias);
static auto &config_transparent_decal_slope_z_bias = halo::link::ref<float>(halo::shell::vars().config_transparent_decal_slope_z_bias);
static auto &physical_memory = halo::link::ref<uint32_t>(halo::shell::vars().physical_memory);
static auto &video_memory = halo::link::ref<uint32_t>(halo::ui::vars().video_memory);
static auto &graphics_driver_version = halo::link::ref<large_integer>(halo::main::vars().graphics_driver_version);
static auto &display_adapter_count = halo::link::ref<uint32_t>(halo::shell::vars().display_adapter_count);
static auto &display_adapters = halo::link::ref<shell_display_adapter [k_shell_maximum_display_adapters]>(halo::shell::vars().display_adapters);
static auto &hardware_requirements = halo::link::ref<hwreq_parser *>(halo::shell::vars().hardware_requirements);
static auto &sound_devices = halo::link::ref<shell_sound_device [k_shell_maximum_sound_devices]>(halo::shell::vars().sound_devices);
static auto &selected_sound_device = halo::link::ref<int32_t>(halo::shell::vars().selected_sound_device);
static auto &cpu_speed = halo::link::ref<uint32_t>(halo::shell::vars().cpu_speed);
static auto &graphics_vendor_name = halo::link::ref<char *>(halo::shell::vars().graphics_vendor_name);
static auto &graphics_device_name = halo::link::ref<char *>(halo::shell::vars().graphics_device_name);
static auto &graphics_device_id = halo::link::ref<uint32_t>(halo::shell::vars().graphics_device_id);
static auto &graphics_vendor_id = halo::link::ref<uint32_t>(halo::rasterizer::vars().graphics_vendor_id);
static auto &config_properties = halo::link::ref<shell_config_property [k_shell_config_property_count]>(halo::shell::vars().config_properties);
static auto &config_unknown_property_text = halo::link::ref<char [k_shell_config_message_length]>(halo::shell::vars().config_unknown_property_text);
static auto &config_error_text = halo::link::ref<char [k_shell_config_message_length]>(halo::shell::vars().config_error_text);
static auto &required_cpu_speed = halo::link::ref<int32_t>(halo::shell::vars().required_cpu_speed);
static auto &required_memory = halo::link::ref<int32_t>(halo::shell::vars().required_memory);
static auto &required_video_memory = halo::link::ref<int32_t>(halo::rasterizer::vars().required_video_memory);
static auto &required_directx_build = halo::link::ref<int32_t>(halo::shell::vars().required_directx_build);
static auto &required_disk_space = halo::link::ref<int32_t>(halo::shell::vars().required_disk_space);
static auto &safe_mode = halo::link::ref<int32_t>(halo::shell::vars().safe_mode);

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
        config_force_shader = k_force_shader_2_0a;
    } else {
        sscanf(value, "%d", &parsed);
        config_force_shader = parsed;
        if (parsed == 0) {
            config_force_shader = k_force_shader_none;
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

    bits = k_decal_z_bias_default_bits;
    config_decal_z_bias = *(float *)&bits;
    config_decal_slope_z_bias = 0.0f;
    bits = k_transparent_decal_z_bias_default_bits;
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
    video_memory = k_video_memory_8mb;
    if (physical_memory > k_memory_class_small_mb / 2 - 1) {
        video_memory = k_video_memory_16mb;
    }
    if (physical_memory > k_memory_class_small_mb - 1) {
        video_memory = k_video_memory_32mb;
    }
    if (physical_memory > k_memory_class_medium_mb - 1) {
        video_memory = k_video_memory_64mb;
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
        FatalError::show(k_dword_none, (uint32_t)requirements.error_message(), 1);
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
