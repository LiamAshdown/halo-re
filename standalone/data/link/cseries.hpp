/**
 * @file standalone/data/link/cseries.hpp
 * Link names of the engine variables the cseries module binds in halo::cseries::Globals (src/cseries/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern int64_t performance_frequency;
extern uint8_t debug_log_level;
extern uint8_t error_file_enabled;
extern uint8_t error_file_needs_header;
extern char error_file_spacer[];
extern char error_file_banner[];
extern char error_file_function_name[];
extern char error_file_function_format[];
extern char error_file_address_format[];
extern char error_file_open_mode[];
extern char error_file_name[];
extern char error_file_timestamp_format[];
extern char error_file_no_timestamp[];
extern char profile_directory[k_profile_directory_storage_size];
extern void *sh_get_folder_path;
extern const uint8_t md5_padding[64];
extern const char md5_hex_byte_format[];
}
