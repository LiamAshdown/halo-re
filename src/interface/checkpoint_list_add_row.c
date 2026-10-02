// checkpoint_list_add_row  (not a Ghidra function; the game_checkpoint_enumerate_files callback that the checkpoint
//   list screen (ui_event_4a44f0) passes as an immediate at 0x4a44f5; no C existed, and it was not even listed as
//   missing because its only reference sat in 0x4a44f0, which had no C either)
// address 0x4a4280, size 615 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4a4280..0x4a44e6: builds a 0x68 byte row record -- +0x00 level index word,
//   +0x04 difficulty, +0x08 the save time as wide text (GetTimeFormatA flags 0xc, then L"|n", then GetDateFormatA
//   flags 1, each at most 16 characters converted byte-wise and cut at 12), +0x48 the checkpoint name (strcpy) -- and
//   adds it to ui list 0 named L"<level> - hh:mm:ss" (format at 0x0066a4ec) where the level name is entry
//   level_index of the ui\shell\main_menu\map_list_short unicode string list (L"<missing string>" when absent) and
//   the time comes from game_time ticks at 30 per second. The first row (index 0) is the default. Returns 1.
//   Record bytes +0x02..+0x03 are stack garbage in the binary; zeroed here.
// blam-cc: stack -> index, name, level_index, difficulty, game_time, time, user_data (cdecl); returns AL

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern tag_instance *tag_instances; // 0x0087bc14
extern uint16_t missing_string_text[]; // 0x00671fac, L"<missing string>"
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest
extern void ui_list_add_entry(int32_t group_index, const uint16_t *name, int32_t id, const void *data_blob, uint32_t data_size, uint8_t is_default); // 0x4a7ba0, blam-cc: EAX group_index, CL is_default

// The inlined ascii to wide widening: at most 0xff characters, terminated, then cut at 12 by the caller.
static void widen(uint16_t *out, const char *in)
{
    int32_t length = (int32_t)strlen(in);

    if (length * 2 + 2 > 0x200) {
        length = 0xff;
    }
    out[length] = 0;
    while (--length >= 0) {
        out[length] = (uint8_t)in[length];
    }
}

uint8_t checkpoint_list_add_row(int32_t index, const char *name, int32_t level_index, int32_t difficulty, int32_t game_time,
    const void *time, void *user_data)
{
    datum_index strings = tag_lookup(0x75737472, (char *)"ui\\shell\\main_menu\\map_list_short"); // 'ustr', 0x0066a51c
    uint8_t record[0x68];
    char text[0x10];
    uint16_t wide[0x100];
    const uint16_t *level_name = missing_string_text;
    int32_t hours;
    int32_t minutes;
    int32_t seconds;

    memset(record, 0, 4);
    *(int16_t *)(record + 0x00) = (int16_t)level_index;
    *(int32_t *)(record + 0x04) = difficulty;
    strcpy((char *)(record + 0x48), name);

    GetTimeFormatA(0x400, 0xc, (const SYSTEMTIME *)time, 0, text, 0x10);
    widen(wide, text);
    wide[12] = 0;
    wcscpy((wchar_t *)(record + 0x08), (const wchar_t *)wide);
    wcscat((wchar_t *)(record + 0x08), L"|n"); // 0x0066a514
    GetDateFormatA(0x400, 1, (const SYSTEMTIME *)time, 0, text, 0x10);
    widen(wide, text);
    wide[12] = 0;
    wcscat((wchar_t *)(record + 0x08), (const wchar_t *)wide);

    hours = game_time / 108000;
    game_time -= hours * 108000;
    minutes = game_time / 1800;
    game_time -= minutes * 1800;
    seconds = game_time / 30;

    if (strings != 0xffffffff) {
        uint8_t *list = (uint8_t *)tag_instances[strings & 0xffff].data;
        int16_t level = (int16_t)level_index;

        if (level >= 0 && level < *(int32_t *)list) {
            uint8_t *element = *(uint8_t **)(list + 4) + level * 0x14;
            uint32_t size = *(uint32_t *)element;

            if ((int32_t)size > 0) {
                uint16_t *text_data = *(uint16_t **)(element + 0xc);

                text_data[(size >> 1) - 1] = 0;
                level_name = text_data;
            }
        }
    }
    string_format_wide_va(wide, (const uint16_t *)L"%s - %02d:%02d:%02d", level_name, hours, minutes, seconds); // 0x0066a4ec
    ui_list_add_entry(0, wide, index, record, 0x68, (uint8_t)(index == 0));
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
