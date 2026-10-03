/**
 * @file standalone/data/link/dialogs.hpp
 * Link names of the engine variables the dialogs module binds in halo::dialogs::Globals (src/dialogs/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern int32_t dialog_hyperlink_hovered;
extern uint32_t shell_language_id;
extern char fatal_error_text[k_shell_fatal_error_text_length];
extern char fatal_error_help_file[k_shell_fatal_error_readme_length];
extern char fatal_error_title[k_shell_fatal_error_title_length];
extern int32_t fatal_error_is_fatal;
extern char *graphics_vendor_name;
extern char *graphics_device_name;
extern uint32_t graphics_device_id;
extern uint32_t cpu_speed;
extern uint32_t physical_memory;
extern uint32_t video_memory;
extern int32_t fatal_error_remember_choice;
extern char fatal_error_system_specs[halo::dialogs::k_system_specs_capacity];
}
