/**
 * Entry functions for the shell module: one function per original symbol, forwarding to the
 * C++ classes in namespace halo::shell (the std/hwreq view classes, Application, GameWindow,
 * CrashReporter, Config and the rest).
 */

#include "halo/shell/hwreq.hpp"
#include "halo/shell/application.hpp"
#include "halo/shell/config.hpp"
#include "halo/shell/diagnostics.hpp"
#include "halo/shell/hardware.hpp"
#include "halo/shell/system.hpp"
#include "halo/shell/window.hpp"
#include "halo/shell/api.hpp"
#include "halo/core/link.hpp"
#include "halo/shell/vars.hpp"

static auto &shell_argc = halo::link::ref<int32_t>(halo::shell::vars().shell_argc);
static auto &shell_argv = halo::link::ref<char **>(halo::shell::vars().shell_argv);
static auto &shell_command_line = halo::link::ref<char *>(halo::shell::vars().shell_command_line);
static auto &shell_window = halo::link::ref<void *>(halo::shell::vars().shell_window);
static auto &shell_instance = halo::link::ref<void *>(halo::shell::vars().shell_instance);
static auto &shell_nosound = halo::link::ref<int32_t>(halo::shell::vars().shell_nosound);
static auto &shell_module_handle = halo::link::ref<void *>(halo::shell::vars().shell_module_handle);
static auto &config_force_shader = halo::link::ref<int32_t>(halo::shell::vars().config_force_shader);
static auto &config_maximum_resolution = halo::link::ref<int32_t>(halo::shell::vars().config_maximum_resolution);
static auto &config_disable_alpha_render_targets = halo::link::ref<int32_t>(halo::shell::vars().config_disable_alpha_render_targets);
static auto &config_disable_buffering = halo::link::ref<int32_t>(halo::shell::vars().config_disable_buffering);
static auto &config_disable_driver_management = halo::link::ref<int32_t>(halo::shell::vars().config_disable_driver_management);
static auto &config_disable_render_targets = halo::link::ref<int32_t>(halo::shell::vars().config_disable_render_targets);
static auto &config_disable_specular = halo::link::ref<int32_t>(halo::shell::vars().config_disable_specular);
static auto &config_enable_stop_start = halo::link::ref<int32_t>(halo::shell::vars().config_enable_stop_start);
static auto &config_head_relative_speech = halo::link::ref<int32_t>(halo::shell::vars().config_head_relative_speech);
static auto &config_invalid_driver = halo::link::ref<int32_t>(halo::shell::vars().config_invalid_driver);
static auto &config_invalid_sound_driver = halo::link::ref<int32_t>(halo::shell::vars().config_invalid_sound_driver);
static auto &config_linear_texture_addressing = halo::link::ref<int32_t>(halo::shell::vars().config_linear_texture_addressing);
static auto &config_linear_texture_addressing_sun = halo::link::ref<int32_t>(halo::shell::vars().config_linear_texture_addressing_sun);
static auto &config_linear_texture_addressing_zoom = halo::link::ref<int32_t>(halo::shell::vars().config_linear_texture_addressing_zoom);
static auto &config_min_max_blend_op_is_broken = halo::link::ref<int32_t>(halo::shell::vars().config_min_max_blend_op_is_broken);
static auto &config_old_driver = halo::link::ref<int32_t>(halo::shell::vars().config_old_driver);
static auto &config_old_sound_driver = halo::link::ref<int32_t>(halo::shell::vars().config_old_sound_driver);
static auto &config_prototype_card = halo::link::ref<int32_t>(halo::shell::vars().config_prototype_card);
static auto &config_safe_mode = halo::link::ref<int32_t>(halo::shell::vars().config_safe_mode);
static auto &config_unsupported_card = halo::link::ref<int32_t>(halo::shell::vars().config_unsupported_card);
static auto &config_use_alternate_convolve_mask = halo::link::ref<int32_t>(halo::shell::vars().config_use_alternate_convolve_mask);
static auto &config_use_anisotropic_filter = halo::link::ref<int32_t>(halo::shell::vars().config_use_anisotropic_filter);
static auto &config_use_fixed_function = halo::link::ref<int32_t>(halo::shell::vars().config_use_fixed_function);
static auto &config_decal_z_bias = halo::link::ref<float>(halo::shell::vars().config_decal_z_bias);
static auto &config_decal_slope_z_bias = halo::link::ref<float>(halo::shell::vars().config_decal_slope_z_bias);
static auto &config_transparent_decal_z_bias = halo::link::ref<float>(halo::shell::vars().config_transparent_decal_z_bias);
static auto &config_transparent_decal_slope_z_bias = halo::link::ref<float>(halo::shell::vars().config_transparent_decal_slope_z_bias);


namespace halo::shell {

Globals &globals()
{
    static Globals instance{::shell_argc, ::shell_argv, ::shell_command_line, ::shell_window, ::shell_instance, ::shell_nosound, ::shell_module_handle, ::config_disable_alpha_render_targets, ::config_disable_buffering, ::config_disable_driver_management, ::config_disable_render_targets, ::config_disable_specular, ::config_enable_stop_start, ::config_head_relative_speech, ::config_invalid_driver, ::config_invalid_sound_driver, ::config_linear_texture_addressing, ::config_linear_texture_addressing_sun, ::config_linear_texture_addressing_zoom, ::config_min_max_blend_op_is_broken, ::config_old_driver, ::config_old_sound_driver, ::config_prototype_card, ::config_safe_mode, ::config_unsupported_card, ::config_use_alternate_convolve_mask, ::config_use_anisotropic_filter, ::config_use_fixed_function, ::config_force_shader, ::config_maximum_resolution, ::config_decal_z_bias, ::config_decal_slope_z_bias, ::config_transparent_decal_z_bias, ::config_transparent_decal_slope_z_bias};
    return instance;
}



























































































hwreq_parse_exception * hwreq_parse_exception_copy_construct(hwreq_parse_exception *self, const hwreq_parse_exception *other)
{
    return halo::shell::ParseException(self).copy_construct(other);
}



void hwreq_parse_exception_scalar_deleting_destruct(hwreq_parse_exception *self, uint8_t free_flag)
{
    halo::shell::ParseException(self).scalar_deleting_destruct(free_flag);
}

void hwreq_length_error_destruct(hwreq_parse_exception *self)
{
    halo::shell::ParseException(self).length_error_destruct();
}

void hwreq_length_error_scalar_deleting_destruct(hwreq_parse_exception *self, uint8_t free_flag)
{
    halo::shell::ParseException(self).length_error_scalar_deleting_destruct(free_flag);
}

void hwreq_out_of_range_destruct(hwreq_parse_exception *self)
{
    halo::shell::ParseException(self).out_of_range_destruct();
}

void hwreq_out_of_range_scalar_deleting_destruct(hwreq_parse_exception *self, uint8_t free_flag)
{
    halo::shell::ParseException(self).out_of_range_scalar_deleting_destruct(free_flag);
}

























void hwreq_parser_scalar_deleting_destructor(hwreq_parser *this_parser)
{
    halo::shell::HwreqParser(this_parser).scalar_deleting_destruct();
}

char * hwreq_parser_get_error_message(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).error_message_text();
}

uint32_t hwreq_parser_get_flag_count(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).flag_count();
}

char * hwreq_parser_get_flag_name(hwreq_parser *parser, uint32_t index)
{
    return halo::shell::HwreqParser(parser).flag_name(index);
}

char * hwreq_parser_get_flag_value(hwreq_parser *parser, uint32_t index)
{
    return halo::shell::HwreqParser(parser).flag_value(index);
}

hwreq_property_set * hwreq_parser_get_flags(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).flags_set();
}

char * hwreq_parser_get_graphics_device_name(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).graphics_device_name_text();
}

char * hwreq_parser_get_graphics_vendor_name(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).graphics_vendor_name_text();
}

uint32_t hwreq_parser_get_requirement_count(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).requirement_count();
}

char * hwreq_parser_get_requirement_name(hwreq_parser *parser, uint32_t index)
{
    return halo::shell::HwreqParser(parser).requirement_name(index);
}

char * hwreq_parser_get_requirement_value(hwreq_parser *parser, uint32_t index)
{
    return halo::shell::HwreqParser(parser).requirement_value(index);
}

char * hwreq_parser_get_sound_device_name(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).sound_device_name_text();
}

char * hwreq_parser_get_sound_vendor_name(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).sound_vendor_name_text();
}

uint8_t hwreq_parser_has_error(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).has_error();
}

hwreq_property_set * hwreq_parser_find_property_set(hwreq_parser *self, const char *name)
{
    return halo::shell::HwreqParser(self).find_property_set(name);
}



uint8_t hwreq_parser_parse(hwreq_parser *parser, const char *path, const shell_sound_device *sound_device, const d3d_adapter_identifier9 *adapter, const d3d_caps9 *caps, uint32_t memory, uint32_t video_memory, uint32_t cpu_speed)
{
    return halo::shell::HwreqParser(parser).parse(path, sound_device, adapter, caps, memory, video_memory, cpu_speed);
}



















void *exception_copy_construct(void *self_, const void *other)
{
    return halo::shell::StdException(self_).copy_construct(other);
}



const char *__fastcall std_exception_what(uint8_t *self, void *unused_edx)
{
    (void)unused_edx;
    return halo::shell::StdRuntimeError(self).what();
}

hwreq_parse_exception *__fastcall std_length_error_copy_construct(hwreq_parse_exception *self, void *unused_edx,
    const hwreq_parse_exception *other)
{
    (void)unused_edx;
    return halo::shell::ParseException(self).length_error_copy_construct(other);
}

hwreq_parse_exception *__fastcall std_out_of_range_copy_construct(hwreq_parse_exception *self, void *unused_edx,
    const hwreq_parse_exception *other)
{
    (void)unused_edx;
    return halo::shell::ParseException(self).out_of_range_copy_construct(other);
}











uint32_t clipboard_get_text(char *buffer, uint32_t capacity)
{
    return halo::shell::Clipboard::get_text(buffer, capacity);
}

uint8_t command_line_check_flag(const char *flag_name, const char **out_value)
{
    return halo::shell::CommandLine::has_flag(flag_name, out_value);
}







uint8_t config_compute_uma_video_memory(void)
{
    return halo::shell::Config::compute_uma_video_memory();
}



uint8_t config_set_decal_slope_z_bias(char *value)
{
    return halo::shell::Config::set_float(&config_decal_slope_z_bias, value);
}

uint8_t config_set_decal_z_bias(char *value)
{
    return halo::shell::Config::set_float(&config_decal_z_bias, value);
}

uint8_t config_set_transparent_decal_slope_z_bias(char *value)
{
    return halo::shell::Config::set_float(&config_transparent_decal_slope_z_bias, value);
}

uint8_t config_set_transparent_decal_z_bias(char *value)
{
    return halo::shell::Config::set_float(&config_transparent_decal_z_bias, value);
}

uint8_t config_set_maximum_resolution(char *value)
{
    return halo::shell::Config::set_maximum_resolution(value);
}

uint8_t config_set_force_shader(const char *value)
{
    return halo::shell::Config::set_force_shader(value);
}

uint8_t config_set_disable_alpha_render_targets(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_disable_alpha_render_targets);
}

uint8_t config_set_disable_buffering(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_disable_buffering);
}

uint8_t config_set_disable_driver_management(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_disable_driver_management);
}

uint8_t config_set_disable_render_targets(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_disable_render_targets);
}

uint8_t config_set_disable_specular(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_disable_specular);
}

uint8_t config_set_enable_stop_start(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_enable_stop_start);
}

uint8_t config_set_head_relative_speech(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_head_relative_speech);
}

uint8_t config_set_invalid_driver(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_invalid_driver);
}

uint8_t config_set_invalid_sound_driver(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_invalid_sound_driver);
}

uint8_t config_set_linear_texture_addressing(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_linear_texture_addressing);
}

uint8_t config_set_linear_texture_addressing_sun(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_linear_texture_addressing_sun);
}

uint8_t config_set_linear_texture_addressing_zoom(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_linear_texture_addressing_zoom);
}

uint8_t config_set_min_max_blend_op_is_broken(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_min_max_blend_op_is_broken);
}

uint8_t config_set_old_driver(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_old_driver);
}

uint8_t config_set_old_sound_driver(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_old_sound_driver);
}

uint8_t config_set_prototype_card(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_prototype_card);
}

uint8_t config_set_safe_mode(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_safe_mode);
}

uint8_t config_set_unsupported_card(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_unsupported_card);
}

uint8_t config_set_use_alternate_convolve_mask(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_use_alternate_convolve_mask);
}

uint8_t config_set_use_anisotropic_filter(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_use_anisotropic_filter);
}

uint8_t config_set_use_fixed_function(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_use_fixed_function);
}
int32_t cpu_get_type(int32_t mode)
{
    return halo::shell::Cpu::get_type(mode);
}





















void keystone_library_unload(void)
{
    halo::shell::KeystoneLibrary::unload();
}

void os_platform_identify(void)
{
    halo::shell::OperatingSystem::identify();
}



int32_t security_check_write_access(void)
{
    return halo::shell::WriteAccessCheck::run();
}









int32_t shell_display_fatal_error_dialog(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal)
{
    return halo::shell::FatalError::show(resource_id, help_text_or_id, is_fatal);
}





int32_t shell_load_localized_string(uint32_t buffer_capacity, void *module, char *buffer, uint32_t id)
{
    return halo::shell::Localization::load_localized_string(buffer_capacity, module, buffer, id);
}



char *shell_parse_config_txt(uint32_t adapter_index, d3d9_interface *d3d)
{
    return halo::shell::ConfigLoader::parse(adapter_index, d3d);
}

void shell_pump_windows_messages(void)
{
    halo::shell::GameWindow::pump_messages();
}





int32_t __stdcall shell_winmain(void *hInstance, void *hPrevInstance, char *lpCmdLine, int32_t nCmdShow)
{
    return halo::shell::Application::winmain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

}
