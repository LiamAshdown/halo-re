// path_split_components  (not a Ghidra function; splits a file path in place into directory / name / extension)
// address 0x556000, size 202 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x556000..0x5560c9. Every out pointer starts at the path's end (length truncated
//   to int16). Walking backwards by character (text_find_character_boundary 0x5576d0, multi-byte aware): with
//   split_extension, the last '.' before any backslash is cut (NUL) and *ext_start_out points after it, and the last
//   backslash is cut and *ext_fallback_out points after it (the file name); without split_extension (or once that
//   is set) the last backslash only sets *dir_start_out after it. At the end, with split_extension and no backslash
//   the name is the whole path (*ext_fallback_out = path); otherwise, when the name does not start the path,
//   *name_end_out = path. The parameter names follow the existing callers' prototype; the roles are as described.
// blam-cc: EBX dir_start_out, ESI path, EDI ext_fallback_out, stack -> name_end_out, ext_start_out, split_extension

#include "tags.h"
#include "memory.h"
#include "math.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint16_t text_find_character_boundary(uint8_t *string, int16_t *length_inout); // 0x5576d0

void path_split_components(char **dir_start_out, char *path, char **ext_fallback_out,
    char **name_end_out, char **ext_start_out, uint8_t split_extension)
{
    int16_t length = (int16_t)strlen(path);
    char *end = path + length;

    *name_end_out = end;
    *dir_start_out = end;
    *ext_fallback_out = end;
    *ext_start_out = end;
    while (length != 0) {
        uint16_t character = text_find_character_boundary((uint8_t *)path, &length);

        if (character == '.') {
            if (split_extension && **ext_fallback_out == 0 && **ext_start_out == 0) {
                path[length] = 0;
                *ext_start_out = path + length + 1;
            }
        } else if (character == '\\') {
            if (split_extension && **ext_fallback_out == 0) {
                path[length] = 0;
                *ext_fallback_out = path + length + 1;
            } else if (**dir_start_out == 0) {
                *dir_start_out = path + length + 1;
            }
        }
    }
    if (split_extension && **ext_fallback_out == 0) {
        *ext_fallback_out = path;
        return;
    }
    if (*ext_fallback_out != path) {
        *name_end_out = path;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
