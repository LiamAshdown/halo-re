#include "halo/interface/ifr1_checkpoint_list_rows.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include <string.h>
#include <wchar.h>
#include "halo/cache/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/constants.hpp"

extern "C" {
extern uint16_t missing_string_text[];
}

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

namespace halo::interface {

/**
 * Original engine function checkpoint_list_add_row; the author notes are in
 * docs/original/interface/checkpoint_list_add_row.txt.
 *
 * @address 0x4a4280
 */
uint8_t CheckpointListRows::add_row(int32_t index, const char *name, int32_t level_index, int32_t difficulty, int32_t game_time, const void *time, void *user_data)
{
    datum_index strings = halo::interface::lookup_tag(halo::groups::unicode_string_list, "ui\\shell\\main_menu\\map_list_short");
    uint8_t record[0x68];
    char text[0x10];
    uint16_t wide[halo::interface::k_text_buffer_chars];
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
    wcscat((wchar_t *)(record + 0x08), L"|n");
    GetDateFormatA(0x400, 1, (const SYSTEMTIME *)time, 0, text, 0x10);
    widen(wide, text);
    wide[12] = 0;
    wcscat((wchar_t *)(record + 0x08), (const wchar_t *)wide);

    hours = game_time / 108000;
    game_time -= hours * 108000;
    minutes = game_time / 1800;
    game_time -= minutes * 1800;
    seconds = game_time / 30;

    if (strings != halo::k_dword_none) {
        uint8_t *list = halo::interface::tag_data<uint8_t>(strings);
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
    halo::text::string_format_wide_va(wide, (const uint16_t *)L"%s - %02d:%02d:%02d", level_name, hours, minutes, seconds);
    halo::interface::ui_list_add_entry(0, wide, index, record, 0x68, (uint8_t)(index == 0));
    return 1;
}

}
