/**
 * @file include/halo/networking/net2_util.hpp
 * Small string, time, mutex and pointer-array helpers.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Small string, time, mutex and pointer-array helpers.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class NetworkUtil {
public:
    /**
     * 0x4b7080, this module
     *
     * @address 0x4ba8a0
     */
    static int32_t add_unique(void *value, server_list_globals *array);

    /**
     * Linear search for `value` in `array`, returning its index or -1 if it is not present (or the array is empty, or value is NULL).
     *
     * @address 0x4ba870
     */
    static int32_t find_index(server_list_globals *array, void *value);

    /**
     * Original engine function `dynamic_pointer_array_remove_at`.
     *
     * @address 0x4ba940
     */
    static void remove_at(int32_t index, server_list_globals *array);

    /**
     * this batch, 0x4e5320 Converts time_value to local time and formats it into time_dest ("HH:MM:SS") and date_dest ("YYYY-MM-DD") via format_time_and_date_strings, falling back to an all-zero broken-down time if localtime() fails.
     *
     * @address 0x4e52c0
     */
    static void local_time_and_date(char *date_dest, int32_t max_len, int32_t time_value, char *time_dest);

    /**
     * VERIFIED against disassembly 0x4e5320..0x4e5381 (2026-09-30); fixed: the original calls the CRT _snprintf (0x623a2d), which writes count characters and does NOT NUL-terminate on truncation (the C99 snprintf the draft used writes a NUL at count-1); the forced NUL at max_len-1 is the only terminator, as here. Formats time_value's hour:min:sec into time_dest and its year-mon-mday into date_dest, each truncated to at most max_len-1 characters with a forced NUL. Either destination may be NULL (skipped), and nothing is written if max_len is 0.
     *
     * @address 0x4e5320
     */
    static void time_and_date_strings(char *date_dest, struct tm *time_value, int32_t max_len,
    char *time_dest);

    /**
     * Original engine function `mutex_create`.
     *
     * @address 0x440510
     */
    static int32_t create(network_mutex_record **out_handle);

    /**
     * 0x00699568, used when unit_table is NULL Parses a leading unsigned integer from `string`, then an optional one-character unit suffix (falling back to default_unit if the suffix is not one strchr recognizes in unit_table, or NULL selects the built-in table). Returns the value in seconds for d/h/m/s, or -1 if the string has no leading digits or its resolved unit is not one of those four.
     *
     * @address 0x4e51c0
     */
    static int32_t time_duration_string(char *string, char default_unit, uint8_t *unit_table);

    /**
     * Returns 1 if `string` is NULL, empty, or consists solely of digits and/or minus signs; returns 0 as soon as any other character is found.
     *
     * @address 0x4e3f30
     */
    static uint8_t is_numeric(char *string);

    /**
     * Trims *string_ptr in place: walks back from the end, replacing trailing whitespace/'\n'/'\r' with NUL, then walks forward from the start, replacing each leading whitespace/'\n'/'\r' with NUL and advancing *string_ptr to the first character that is none of those.
     *
     * @address 0x4e4040
     */
    static void trim_whitespace(char **string_ptr);

};

}  // namespace halo::networking
