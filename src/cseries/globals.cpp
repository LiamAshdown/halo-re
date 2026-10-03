/**
 * @file src/cseries/globals.cpp
 * Binds halo::cseries::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/cseries/cseries.hpp"
#include "halo/cseries/api.hpp"
#include "crt.h"
#include "tags.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "halo/shell/api.hpp"
#include "memory.h"
#include "math.h"
#include "win32.h"
#include <ctype.h>
#include "halo/core/crt.hpp"
#include "halo/cseries/api.hpp"
#include "link/cseries.hpp"

static_assert(k_profile_directory_storage_size == 0x105);

namespace halo::cseries {

Globals &Service::instance()
{
    static Globals state{
        ::performance_frequency,
        ::debug_log_level,
        ::error_file_enabled,
        ::error_file_needs_header,
        ::error_file_spacer,
        ::error_file_banner,
        ::error_file_function_name,
        ::error_file_function_format,
        ::error_file_address_format,
        ::error_file_open_mode,
        ::error_file_name,
        ::error_file_timestamp_format,
        ::error_file_no_timestamp,
        ::profile_directory,
        ::sh_get_folder_path,
        ::md5_padding,
        ::md5_hex_byte_format,
    };
    return state;
}

}  // namespace halo::cseries
