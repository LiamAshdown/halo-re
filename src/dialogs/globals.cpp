/**
 * @file src/dialogs/globals.cpp
 * Binds halo::dialogs::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/dialogs/dialogs.hpp"
#include "halo/dialogs/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/core/crt.hpp"
#include "halo/dialogs/globals.hpp"
#include "link/dialogs.hpp"

namespace halo::dialogs {

Globals &Service::instance()
{
    static Globals state{
        ::dialog_hyperlink_hovered,
        ::shell_language_id,
        ::fatal_error_text,
        ::fatal_error_help_file,
        ::fatal_error_title,
        ::fatal_error_is_fatal,
        ::graphics_vendor_name,
        ::graphics_device_name,
        ::graphics_device_id,
        ::cpu_speed,
        ::physical_memory,
        ::video_memory,
        ::fatal_error_remember_choice,
        ::fatal_error_system_specs,
    };
    return state;
}

}  // namespace halo::dialogs
