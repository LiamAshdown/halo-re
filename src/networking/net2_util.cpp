/**
 * @file src/networking/net2_util.cpp
 * Small string, time, mutex and pointer-array helpers.
 */
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "crt.h"
#include <ctype.h>
#include <stdlib.h>
#include "halo/networking/net2_util.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"

static auto &network_mutex_name_counter = halo::link::ref<int32_t>(halo::networking::vars().network_mutex_name_counter);
static auto &default_time_unit_table = halo::link::ref<uint8_t []>(halo::networking::vars().default_time_unit_table);


namespace halo::networking {

int32_t NetworkUtil::add_unique(void *value, server_list_globals *array)
{
    int32_t index;

    if (!halo::networking::server_browser_server_passes_filter(value)) {
        return 0;
    }

    if (value != 0 && 0 < array->result_count) {
        void **element = array->list;

        index = 0;
        do {
            if (*element == value) {
                if (index != -1) {
                    return 0;
                }
                break;
            }
            index = index + 1;
            element = element + 1;
        } while (index < array->result_count);
    }

    if (array->result_count == array->capacity) {
        int32_t new_capacity = array->capacity + 0x20;
        uint32_t new_bytes = (uint32_t)new_capacity * 4;
        void *new_data = array->list;

        if (new_data == 0) {
            new_data = GlobalAlloc(0, new_bytes);
        } else if (new_bytes == 0) {
            GlobalFree(new_data);
            new_data = 0;
        } else {
            new_data = GlobalReAlloc(new_data, new_bytes, 2);
        }
        array->capacity = new_capacity;
        array->list = (void **)new_data;
    }

    array->list[array->result_count] = value;
    array->pending_count = array->pending_count + 1;
    array->result_count = array->result_count + 1;
    return 1;
}

int32_t NetworkUtil::find_index(server_list_globals *array, void *value)
{
    int32_t index = -1;

    if (value != 0 && 0 < array->result_count) {
        void **element = array->list;

        index = 0;
        while (*element != value) {
            index = index + 1;
            element = element + 1;
            if (array->result_count <= index) {
                return -1;
            }
        }
    }
    return index;
}

void NetworkUtil::remove_at(int32_t index, server_list_globals *array)
{
    if (index < array->result_count - 1) {
        void **dst = array->list + index;

        memmove(dst, dst + 1, (uint32_t)(array->result_count - index) * 4 - 4);
    }
    array->result_count = array->result_count - 1;
}

void NetworkUtil::local_time_and_date(char *date_dest, int32_t max_len, int32_t time_value, char *time_dest)
{
    time_t t = time_value;
    struct tm zero_tm;
    struct tm *tm_now;

    tm_now = localtime(&t);
    if (tm_now == 0) {
        zero_tm.tm_sec = 0;
        zero_tm.tm_min = 0;
        zero_tm.tm_hour = 0;
        zero_tm.tm_mday = 0;
        zero_tm.tm_mon = 0;
        zero_tm.tm_year = 0;
        zero_tm.tm_wday = 0;
        zero_tm.tm_yday = 0;
        zero_tm.tm_isdst = 0;
        tm_now = &zero_tm;
    }
    halo::networking::format_time_and_date_strings(date_dest, tm_now, max_len, time_dest);
}

void NetworkUtil::time_and_date_strings(char *date_dest, struct tm *time_value, int32_t max_len,
    char *time_dest)
{
    if (time_dest != 0 && max_len != 0) {
        _snprintf(time_dest, max_len - 1, "%02d:%02d:%02d", time_value->tm_hour, time_value->tm_min, time_value->tm_sec);
        time_dest[max_len - 1] = 0;
    }
    if (date_dest != 0 && max_len != 0) {
        _snprintf(date_dest, max_len - 1, "%04d-%02d-%02d", time_value->tm_year + 0x76c, time_value->tm_mon + 1, time_value->tm_mday);
        date_dest[max_len - 1] = 0;
    }
}

int32_t NetworkUtil::create(network_mutex_record **out_handle)
{
    network_mutex_record *slot;
    int32_t name_index;

    slot = halo::networking::network_mutex_slot_allocate();
    name_index = network_mutex_name_counter;
    if (slot == 0) {
        *out_handle = 0;
        return 0;
    }
    network_mutex_name_counter = network_mutex_name_counter + 1;
    snprintf(slot->name, 0x20, "mutex_%ld", name_index);
    slot->handle = CreateMutexA(0, 0, 0);
    if (slot->handle != 0) {
        *out_handle = slot;
        return 1;
    }
    *out_handle = 0;
    return 0;
}

int32_t NetworkUtil::time_duration_string(char *string, char default_unit, uint8_t *unit_table)
{
    char resolved_unit;
    char unit_from_table;
    int32_t value;
    char *p;

    if (unit_table == 0) {
        unit_table = default_time_unit_table;
    }
    resolved_unit = (char)tolower((uint8_t)default_unit);
    value = atol(string);
    if (value == 0) {
        if (!isdigit((uint8_t)*string)) {
            return -1;
        }
    } else {
        unit_from_table = resolved_unit;
        p = string;
        if (*p != 0) {
            while (isdigit((uint8_t)*p)) {
                p = p + 1;
                if (*p == 0) {
                    goto have_unit;
                }
            }
        }
        if (*p != 0) {
            int32_t lowered = tolower((uint8_t)*p);
            unit_from_table = (char)lowered;
            if (strchr((char *)unit_table, lowered) == 0) {
                unit_from_table = resolved_unit;
            }
        }
    have_unit:
        resolved_unit = unit_from_table;
        if (value == -1) {
            return -1;
        }
    }
    switch (resolved_unit) {
    case 'd': return value * 0x15180;
    case 'h': return value * 0xe10;
    case 'm': return value * 0x3c;
    case 's': return value;
    default: return -1;
    }
}

uint8_t NetworkUtil::is_numeric(char *string)
{
    while (1) {
        if (string == 0 || *string == 0) {
            return 1;
        }
        if (!isdigit((uint8_t)*string) && *string != '-') {
            break;
        }
        string = string + 1;
    }
    return 0;
}

void NetworkUtil::trim_whitespace(char **string_ptr)
{
    char *end = *string_ptr;
    char *p;
    char *start;

    while (*end != 0) {
        end = end + 1;
    }
    while (1) {
        end = end - 1;
        if (!isspace((uint8_t)*end) && *end != '\n' && *end != '\r') {
            break;
        }
        *end = 0;
    }
    for (start = *string_ptr; isspace((uint8_t)*start) || *start == '\n' || *start == '\r'; start = start + 1) {
        *start = 0;
    }
    *string_ptr = start;
}

}  // namespace halo::networking

namespace halo::networking {
int32_t dynamic_pointer_array_add_unique(void *value, server_list_globals *array)
{
    return halo::networking::NetworkUtil::add_unique(value, array);
}

int32_t dynamic_pointer_array_find_index(server_list_globals *array, void *value)
{
    return halo::networking::NetworkUtil::find_index(array, value);
}

void dynamic_pointer_array_remove_at(int32_t index, server_list_globals *array)
{
    halo::networking::NetworkUtil::remove_at(index, array);
}

void format_local_time_and_date(char *date_dest, int32_t max_len, int32_t time_value, char *time_dest)
{
    halo::networking::NetworkUtil::local_time_and_date(date_dest, max_len, time_value, time_dest);
}

void format_time_and_date_strings(char *date_dest, struct tm *time_value, int32_t max_len,
    char *time_dest)
{
    halo::networking::NetworkUtil::time_and_date_strings(date_dest, time_value, max_len, time_dest);
}

int32_t mutex_create(network_mutex_record **out_handle)
{
    return halo::networking::NetworkUtil::create(out_handle);
}

int32_t parse_time_duration_string(char *string, char default_unit, uint8_t *unit_table)
{
    return halo::networking::NetworkUtil::time_duration_string(string, default_unit, unit_table);
}

uint8_t string_is_numeric(char *string)
{
    return halo::networking::NetworkUtil::is_numeric(string);
}

void string_trim_whitespace(char **string_ptr)
{
    halo::networking::NetworkUtil::trim_whitespace(string_ptr);
}

}
