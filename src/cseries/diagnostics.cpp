#include "halo/cseries/cseries.hpp"

#include "crt.h"
#include "tags.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

extern "C" {
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
extern char *network_log_path_resolve(char *requested_path);
extern void write_to_error_file(char *message, uint8_t with_timestamp);
extern char profile_directory[k_profile_directory_storage_size];
typedef int32_t (__stdcall *sh_get_folder_path_proc)(void *owner, int32_t csidl, void *token, uint32_t flags, char *out_path);
extern void *sh_get_folder_path;
extern uint8_t command_line_check_flag(const char *flag_name, const char **out_value);
extern int32_t shell_display_fatal_error_dialog(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal);
}

namespace halo::cseries {

/**
 * Appends message to debug.txt, optionally prefixed with a MM.DD.YY HH:MM:SS timestamp, and does
 * nothing while the shell debug level is below 2 or the error file is disabled. The first logged
 * message also writes a one-time header block (blank line, version banner and this function's own name
 * and address).
 *
 * The file name is resolved through the networking module's log path helper and opened for append.
 *
 * @address 0x449450
 */
void error_log::write(char *message, uint8_t with_timestamp)
{
    char formatted[0x400];
    void *file;
    char *path;
    __time32_t time_value;
    struct tm *local_time;

    if (debug_log_level < k_error_file_minimum_level) {
        return;
    }

    if (error_file_needs_header != 0) {
        error_file_needs_header = 0;
        halo::cseries::error_log::write(error_file_spacer, 0);
        halo::cseries::error_log::write(error_file_banner, 1);
        sprintf(formatted, error_file_function_format, error_file_function_name);
        halo::cseries::error_log::write(formatted, 1);
        sprintf(formatted, error_file_address_format, (uint32_t)(size_t)write_to_error_file);
        halo::cseries::error_log::write(formatted, 1);
    }

    if (error_file_enabled != 0) {
        path = network_log_path_resolve(error_file_name);
        file = fopen(path, error_file_open_mode);
        if (file != 0) {
            if (with_timestamp != 0) {
                _time32(&time_value);
                local_time = _localtime32(&time_value);
                if (local_time == 0) {
                    fprintf((FILE *)file, error_file_no_timestamp);
                } else {
                    fprintf((FILE *)file, error_file_timestamp_format, local_time->tm_mon + 1,
                             local_time->tm_mday, local_time->tm_year % 100,
                             local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
                }
            }
            fprintf((FILE *)file, "%s", message);
            fclose((FILE *)file);
        }
    }
}

/**
 * Determines and stores the player's profile directory path into the profile_directory global: a
 * "-path" command-line override if present, otherwise "<CSIDL_PERSONAL>\My Games\Halo"; if even the
 * special-folder lookup fails, falls back to "." and raises a fatal error dialog.
 *
 * @address 0x449390
 */
void profile_path::initialize()
{
    const char *path_argument;
    char documents_path[k_cseries_path_length];
    int32_t result;

    if (command_line_check_flag("-path", &path_argument) != 0 && path_argument != 0) {
        strncpy(profile_directory, path_argument, k_cseries_path_length);
        return;
    }

    printf("Using profile path %s.\n", profile_directory);
    result = ((sh_get_folder_path_proc)sh_get_folder_path)(0, k_csidl_personal, 0, 0,
                                                             documents_path);
    if (result >= 0) {
        _snprintf(profile_directory, k_cseries_path_length, "%s\\My Games\\Halo",
                   documents_path);
        return;
    }

    strncpy(profile_directory, ".", k_cseries_path_length);
    shell_display_fatal_error_dialog(k_profile_path_error_title, (uint32_t)((const char *)k_profile_path_error_message), 1);
}

} // namespace halo::cseries
