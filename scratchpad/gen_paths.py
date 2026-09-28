import os
os.chdir(r'C:\Users\Liam-\halo-re')
HEAD = '''#include "tags.h"
#include "memory.h"
#include "math.h"
#include <string.h>
'''
open('src/saved_games/path_split_components.c', 'w', encoding='utf-8').write('''// path_split_components  (not a Ghidra function; splits a file path in place into directory / name / extension)
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

''' + HEAD + '''
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
        } else if (character == '\\\\') {
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
''')
open('src/saved_games/path_build_full.c', 'w', encoding='utf-8').write('''// path_build_full  (not a Ghidra function; builds the full path of a file reference)
// address 0x5560d0, size 155 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x5560d0..0x55616a: location 2 copies the path as is; a positive location prefixes
//   the root template (0x0069fa50, "?:\\\\" with the drive patched in) and appends at most 0xfb characters (strncat);
//   otherwise a path that is not rooted (second character neither '\\\\' nor ':') gets ".\\\\" (0x00671f94) in front.
// blam-cc: EAX source, EDX destination, CX location

''' + HEAD + '''
extern char file_root_template[4]; // 0x0069fa50

void path_build_full(char *source, char *destination, int16_t location)
{
    destination[0] = 0;
    if (location == 2) {
        strcpy(destination, source);
        return;
    }
    if (location > 0) {
        strcpy(destination, file_root_template);
        strncat(destination, source, 0xfb);
        return;
    }
    if (source[1] != '\\\\' && source[1] != ':') {
        memcpy(destination, ".\\\\", 3); // 0x00671f94
    }
    strcat(destination, source);
}
''')
print('ok')
