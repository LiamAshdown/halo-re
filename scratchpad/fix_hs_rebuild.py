import os
os.chdir(r'C:\Users\Liam-\halo-re')
p = 'src/hs/hs_rebuild_source.c'
s = open(p, encoding='utf-8').read()
start = s.index('// file_reference: defined in types/hs.h')
end = s.index('\n}\n', s.index('char hs_rebuild_source(void)')) + 3
new = r'''// FIXED 2026-09-28 (retail-independence loop): rewritten from objdump 0x483e20..0x484087 with the file helpers'
//   register arguments restored (path_append_component ESI/EBX, path_remove_last_component EBX, path_build_full
//   EAX/EDX/CX, path_split_components EBX/ESI/EDI + stack); the earlier version called them without arguments and
//   named the qsort comparator file_reference_compare_by_name, which bound to nothing -- it is
//   file_reference_compare_full_path (0x483d20).

typedef struct rebuild_file_reference {
    uint32_t signature; // 0x000 'filo'
    uint8_t flags;      // 0x004
    uint8_t unknown_005;
    int16_t location;   // 0x006
    char path[0x100];   // 0x008
    void *handle;       // 0x108
} rebuild_file_reference; // size 0x10c == k_hs_file_reference_size

extern uint8_t file_reference_exists(rebuild_file_reference *ref); // 0x555720, blam-cc: EAX
extern void file_enumerate_start(uint32_t flags, rebuild_file_reference *ref); // 0x555b90
extern uint8_t file_enumerate_find_next(rebuild_file_reference *out_entry, uint32_t *out_write_time); // 0x555c10
extern void path_append_component(char *destination, const char *component); // 0x555ec0, blam-cc: ESI destination, EBX component
extern void path_remove_last_component(char *path); // 0x555f80, blam-cc: EBX
extern void path_build_full(char *source, char *destination, int16_t location); // 0x5560d0, blam-cc: EAX, EDX, CX
extern void path_split_components(char **dir_start_out, char *path, char **ext_fallback_out,
    char **name_end_out, char **ext_start_out, uint8_t split_extension); // 0x556000, blam-cc: EBX, ESI, EDI, stack
extern int32_t file_reference_compare_full_path(const void *a, const void *b); // 0x483d20

extern datum_index global_scenario_index; // 0x0069e8d4
extern tag_instance *tag_instances;       // 0x0087bc14

// Looks for the HS source on disk: "data\global_scripts.hsc" and every *.hsc in "data\<scenario tag
// directory>\scripts" (at most 8 entries, sorted). Returns 1 when NOTHING was found -- each existing file clears it.
char hs_rebuild_source(void)
{
    char directory_path[0x100];
    char display_name[0x100];
    char extension[0x100];
    rebuild_file_reference global_scripts;
    rebuild_file_reference scripts_directory;
    rebuild_file_reference entries[8];
    char *dir_start;
    char *ext_fallback;
    char *name_end;
    char *ext_start;
    char nothing_found = 1;
    int16_t count;
    int16_t i;

    sprintf(directory_path, "data\\%s", tag_instances[(int16_t)global_scenario_index].name); // 0x00660fb4
    sprintf(strrchr(directory_path, '\\') + 1, "scripts"); // 0x00660fac

    memset(&global_scripts, 0, sizeof(global_scripts));
    global_scripts.signature = 0x66696c6f;
    global_scripts.location = -1;
    if ((global_scripts.flags & 1) != 0) {
        path_remove_last_component(global_scripts.path);
    }
    path_append_component(global_scripts.path, "data\\global_scripts.hsc"); // 0x00660f94
    global_scripts.flags |= 1;
    if (file_reference_exists(&global_scripts) != 0) {
        file_reference_exists(&global_scripts); // the binary calls it twice, the second result unused
        nothing_found = 0;
    }

    memset(&scripts_directory, 0, sizeof(scripts_directory));
    scripts_directory.signature = 0x66696c6f;
    scripts_directory.location = -1;
    path_append_component(scripts_directory.path, directory_path);
    file_enumerate_start(0, &scripts_directory);
    for (count = 0; count < 8; count++) {
        if (file_enumerate_find_next(&entries[count], 0) == 0) {
            break;
        }
    }
    qsort(entries, count, sizeof(rebuild_file_reference), (int (*)(const void *, const void *))file_reference_compare_full_path);

    for (i = 0; i < count; i++) {
        memset(display_name, 0, sizeof(display_name));
        path_build_full(entries[i].path, display_name, entries[i].location);
        path_split_components(&dir_start, display_name, &ext_fallback, &name_end, &ext_start, (uint8_t)(entries[i].flags & 1));
        extension[0] = 0;
        if (*ext_start != 0) {
            char *end = extension + strlen(extension);

            if (end != extension) { // never: the buffer was just emptied
                *end++ = '.';
                *end = 0;
            }
            strncpy(end, ext_start, 0xff - strlen(extension));
            extension[0xff] = 0;
        }
        if (strcmp(extension, "hsc") == 0) { // 0x00660f90, 4-byte compare including the NUL
            file_reference_exists(&entries[i]);
            nothing_found = 0;
        }
    }
    return nothing_found;
}
'''
s = s[:start] + new + s[end:]
open(p, 'w', encoding='utf-8').write(s)
print('rewrote', p)
