/**
 * @file include/halo/dialogs/globals.hpp
 * The dialogs module's engine globals as one service object. The variables live at fixed addresses in the data
 * image (standalone/data) under their original link names; Globals holds a reference to each, so no other file
 * declares them.
 */
#pragma once

#include <stdint.h>

namespace halo::dialogs {

struct Globals {
    int32_t &dialog_hyperlink_hovered;
    uint32_t &shell_language_id;
    char (&fatal_error_text)[k_shell_fatal_error_text_length];
    char (&fatal_error_help_file)[k_shell_fatal_error_readme_length];
    char (&fatal_error_title)[k_shell_fatal_error_title_length];
    int32_t &fatal_error_is_fatal;
    char *&graphics_vendor_name;
    char *&graphics_device_name;
    uint32_t &graphics_device_id;
    uint32_t &cpu_speed;
    uint32_t &physical_memory;
    uint32_t &video_memory;
    int32_t &fatal_error_remember_choice;
    char (&fatal_error_system_specs)[halo::dialogs::k_system_specs_capacity];
};

/**
 * The dialogs service singleton. instance() builds the Globals reference table on first use (Meyers singleton); the state it
 * refers to lives in the data image. globals() is the short form every caller uses.
 */
class Service {
public:
    static Globals &instance();
};

inline Globals &globals() { return Service::instance(); }

}  // namespace halo::dialogs
