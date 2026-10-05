#include "halo/cseries/cseries.hpp"

#include "win32.h"
#include "tags.h"
#include "math.h"
#include <string.h>
#include <ctype.h>
#include "memory.h"
#include "halo/cseries/api.hpp"
#include "halo/platform/time.hpp"
#include "halo/platform/file.hpp"
#include "halo/platform/memory.hpp"
#include "halo/platform/system.hpp"


namespace halo::cseries {

namespace {
constexpr uint32_t k_invalid_file_attributes = 0xffffffffu;
}

/**
 * Reads the performance counter and converts it to milliseconds using the frequency the shell cached
 * at start-up.
 *
 * @address 0x449210
 */
uint32_t performance_clock::milliseconds()
{
    large_integer counter;

    halo::platform::read_performance_counter(&counter);
    return (uint32_t)((counter.quad_part * 1000) / globals().performance_frequency);
}

/**
 * Allocates a fixed block of size bytes with GlobalAlloc.
 *
 * @address 0x449370
 */
void *global_memory::alloc(uint32_t size)
{
    return halo::platform::heap_allocate(0, size);
}

/**
 * Frees a block returned by alloc with GlobalFree.
 *
 * @address 0x449380
 */
void *global_memory::release(void *handle)
{
    return halo::platform::heap_free(handle);
}

/**
 * Creates path and every missing parent directory. Returns 1 when the path exists or was created and 0
 * otherwise; a UNC share prefix is skipped.
 *
 * SetErrorMode(SEM_NOOPENFILEERRORBOX) is held during the walk. As in the retail binary it is not
 * restored on the early return taken when the path already exists.
 *
 * @address 0x449250
 */
char halo::cseries::directory_create_recursive(const char *path)
{
    char buffer[k_directory_create_buffer_size];
    uint8_t all_created;
    uint32_t previous_error_mode;
    uint32_t attributes;
    char *cursor;
    char saved_char;

    all_created = 1;
    cursor = buffer;
    previous_error_mode = halo::platform::set_error_mode(k_sem_noopenfileerrorbox);
    attributes = halo::platform::file_attributes(path);
    if (attributes != k_invalid_file_attributes) {
        return 1;
    }

    strcpy(buffer, path);

    if (buffer[0] != '\\' || buffer[1] != '\\' ||
        ((cursor = strpbrk(buffer + 2, "\\")) != 0 &&
         (cursor = strpbrk(cursor + 1, "\\")) != 0)) {
        do {
            cursor = strpbrk(cursor + 1, "\\");
            if (cursor == 0) {
                if (all_created == 1 && halo::platform::directory_create(path) != 0) {
                    goto done;
                }
                break;
            }
            saved_char = *cursor;
            *cursor = '\0';
            attributes = halo::platform::file_attributes(buffer);
            if (attributes == k_invalid_file_attributes && halo::platform::directory_create(buffer) == 0) {
                all_created = 0;
            }
            *cursor = saved_char;
        } while (all_created == 1);
    }
    all_created = 0;
done:
    halo::platform::set_error_mode(previous_error_mode);
    return (char)all_created;
}

/**
 * Lowercases a null-terminated string in place (ASCII, via the CRT tolower()) and returns the same
 * pointer.
 *
 * @address 0x4491e0
 */
char *halo::cseries::string_to_lowercase(char *string)
{
    char *cursor;

    cursor = string;
    while (*cursor != '\0') {
        *cursor = (char)tolower((uint8_t)*cursor);
        cursor = cursor + 1;
    }
    return string;
}

/**
 * Returns immediately; used wherever a callback slot needs a harmless default.
 *
 * @address 0x44ad80
 */
void halo::cseries::function_do_nothing()
{

}

} // namespace halo::cseries
