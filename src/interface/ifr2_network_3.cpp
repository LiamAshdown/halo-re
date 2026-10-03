#include "win32.h"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include "halo/interface/ifr2_network.hpp"
#include "crt.h"
#include <wchar.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

#ifdef interface
#undef interface
#endif

static auto &server_list_entries_006b380c = halo::link::ref<void *[9]>(halo::ui::vars().server_list_entries_006b380c);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &missing_string_text = halo::link::ref<uint16_t []>(halo::ui::vars().missing_string_text);
static auto &chat_local_prompt_string = halo::link::ref<uint16_t []>(halo::ui::vars().chat_local_prompt_string);

namespace halo::interface {

/**
 * @address 0x4a5040
 */
void MenuListView::update()
{
    uint8_t *client = (uint8_t *)halo::networking::globals().client;
    int32_t count = 0;
    int32_t i;
    widget_instance *row;

    server_list_entries_006b380c[0] = nullptr;
    server_list_entries_006b380c[1] = nullptr;
    server_list_entries_006b380c[2] = nullptr;
    server_list_entries_006b380c[3] = nullptr;
    server_list_entries_006b380c[4] = nullptr;
    server_list_entries_006b380c[5] = nullptr;
    server_list_entries_006b380c[6] = nullptr;
    server_list_entries_006b380c[7] = nullptr;
    server_list_entries_006b380c[8] = nullptr;

    if (client == nullptr) {
        return;
    }

    {
        uint8_t *entry = client + 4;

        for (i = 0; i < 9; i++) {
            if (halo::networking::network_game_search_entry_is_fresh((network_game_search_entry *)entry) != 0 && *(int16_t *)(entry + 0x12a) == 1 && entry[300] != 0) {
                server_list_entries_006b380c[count] = entry;
                count++;
            }
            entry += 0x130;
        }
    }

    {
        int32_t *entry = (int32_t *)(client + 0x1c);

        for (i = 0; i < 9; i++) {
            if (*((int8_t *)entry + 0x115) != 0) {
                large_integer counter;
                int32_t now_ms;

                QueryPerformanceCounter((LARGE_INTEGER *)&counter);
                now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
                if (now_ms - *entry <= 0x1770 && *(int16_t *)((uint8_t *)entry + 0x112) == 1 &&
                    *((int8_t *)entry + 0x45 * 4) == 0) {
                    server_list_entries_006b380c[count] = (uint8_t *)entry - 0x18;
                    count++;
                }
            }
            entry += 0x4c;
        }
    }

    widget->list_items = server_list_entries_006b380c;
    widget->item_count = (uint16_t)count;
    {
        int16_t bound = (count - 1 < widget->selection_index) ? (int16_t)(count - 1) : widget->selection_index;

        widget->selection_index = bound;
    }

    row = widget->first_child;
    for (i = 0; row != (widget_instance *)0 && i < count; i++) {
        uint16_t *buf = (uint16_t *)halo::memory::heap_reallocate(row->text, 0x20, widget_memory_pool);
        uint8_t *entry = (uint8_t *)server_list_entries_006b380c[i];

        row->text = buf;
        if (buf != nullptr) {
            if (entry[300] == 1) {
                wcsncpy((wchar_t *)buf, (const wchar_t *)((const uint16_t *)(entry + 0x1c)), 0xf);
                ((uint16_t *)row->text)[0xf] = 0;
            } else {
                datum_index tag = halo::interface::lookup_tag(halo::groups::unicode_string_list, "ui\\multiplayer_game_text");
                uint16_t *source = missing_string_text;

                if (tag != (datum_index)-1) {
                    UnicodeStringList *list = halo::interface::tag_data<UnicodeStringList>(tag);

                    if (list->strings.count > 0x13) {
                        UnicodeStringListString *strings = (UnicodeStringListString *)list->strings.pointer;
                        uint32_t size = strings[0x13].string.size;

                        if ((int32_t)size > 0) {
                            source = (uint16_t *)strings[0x13].string.pointer;
                            *(uint16_t *)((uint8_t *)source + ((size & 0xfffffffe) - 2)) = 0;
                        }
                    }
                }
                halo::text::string_format_wide_va_bounded(0xf, reinterpret_cast<uint16_t *>((wchar_t *)buf), reinterpret_cast<const uint16_t *>(L"%s %s"), source, entry + 0x1c);
                ((uint16_t *)row->text)[0xf] = 0;
            }
        }
        row = row->next_sibling;
    }

    if (count > 0 && widget->selection_index < 0) {
        widget->selection_index = 0;
    }

    {
        large_integer counter;
        int32_t now_ms;
        widget_instance *r1 = widget->extended_description->first_child;
        widget_instance *r2 = r1->next_sibling;
        widget_instance *r3 = r2->next_sibling->next_sibling;
        widget_instance *r4 = r2->next_sibling->first_child;
        widget_instance *r5 = r4->next_sibling;
        widget_instance *r6 = r5->next_sibling;
        widget_instance *r7 = r6->next_sibling;
        widget_instance *r8 = r7->next_sibling;
        widget_instance *r9 = r8->next_sibling;
        widget_instance *r10 = r9->next_sibling;

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

        if (widget->selection_index < 0) {
            r1->background_bitmap_frame = 5;
            r2->background_bitmap_frame = 0x13;
            r4->selection_index = 1;
            r5->selection_index = 0x14;
            r6->selection_index = 1;
            r7->selection_index = 1;
            {
                uint16_t *b = (uint16_t *)halo::memory::heap_reallocate(r8->text, 8, widget_memory_pool);

                r8->text = b;
                if (b != nullptr) b[0] = 0;
            }
            {
                uint16_t *b = (uint16_t *)halo::memory::heap_reallocate(r9->text, 8, widget_memory_pool);

                r9->text = b;
                if (b != nullptr) b[0] = 0;
            }
            r10->selection_index = 1;
            r3->selection_index = (uint32_t)(now_ms - widget->creation_time) > 999;
            r3->hidden = 1;
        } else {
            uint8_t *sel = (uint8_t *)server_list_entries_006b380c[widget->selection_index];
            int16_t kind = *(int16_t *)(sel + 0x120);
            const char *map_name = (const char *)(sel + 0xa0);
            int16_t map_index;

            switch (kind) {
            case 1: r1->background_bitmap_frame = 0; break;
            case 2: r1->background_bitmap_frame = 2; break;
            case 3: r1->background_bitmap_frame = 3; break;
            case 4: r1->background_bitmap_frame = 1; break;
            case 5: r1->background_bitmap_frame = 4; break;
            default: r1->background_bitmap_frame = 5; break;
            }

            if (strstr(map_name, "beavercreek") != 0) map_index = 0;
            else if (strstr(map_name, "sidewinder") != 0) map_index = 1;
            else if (strstr(map_name, "damnation") != 0) map_index = 2;
            else if (strstr(map_name, "ratrace") != 0) map_index = 3;
            else if (strstr(map_name, "prisoner") != 0) map_index = 4;
            else if (strstr(map_name, "hangemhigh") != 0) map_index = 5;
            else if (strstr(map_name, "chillout") != 0) map_index = 6;
            else if (strstr(map_name, "carousel") != 0) map_index = 7;
            else if (strstr(map_name, "boardingaction") != 0) map_index = 8;
            else if (strstr(map_name, "bloodgulch") != 0) map_index = 9;
            else if (strstr(map_name, "wizard") != 0) map_index = 10;
            else if (strstr(map_name, "putput") != 0) map_index = 11;
            else if (strstr(map_name, "longest") != 0) map_index = 0xc;
            else map_index = 0x13;
            r2->background_bitmap_frame = map_index;

            r4->selection_index = (sel[300] != 1) + 0x14;
            r5->selection_index = r2->background_bitmap_frame;

            switch (kind) {
            case 1: r6->selection_index = 3; break;
            case 2: r6->selection_index = 4; break;
            case 3: r6->selection_index = 5; break;
            case 4: r6->selection_index = 6; break;
            case 5: r6->selection_index = 7; break;
            default: r6->selection_index = 8; break;
            }
            r7->selection_index = (sel[0x12e] != 1) + 0xc;

            {
                uint16_t *b = (uint16_t *)halo::memory::heap_reallocate(r8->text, 8, widget_memory_pool);

                r8->text = b;
                if (b != nullptr) {
                    halo::text::string_format_wide_va_bounded(3, reinterpret_cast<uint16_t *>((wchar_t *)b), reinterpret_cast<const uint16_t *>((const wchar_t *)chat_local_prompt_string),
                                                   (int32_t)*(uint16_t *)(sel + 0x124));
                    ((uint16_t *)r8->text)[3] = 0;
                }
            }
            {
                uint16_t *b = (uint16_t *)halo::memory::heap_reallocate(r9->text, 8, widget_memory_pool);

                r9->text = b;
                if (b != nullptr) {
                    halo::text::string_format_wide_va_bounded(3, reinterpret_cast<uint16_t *>((wchar_t *)b), reinterpret_cast<const uint16_t *>((const wchar_t *)chat_local_prompt_string),
                                                   (int32_t)*(int16_t *)(sel + 0x128));
                    ((uint16_t *)r9->text)[3] = 0;
                }
            }

            switch (kind) {
            case 1: r10->selection_index = 0x16; break;
            case 2: r10->selection_index = 0x18; break;
            case 3: r10->selection_index = 0x18 - (sel[0x12f] != 1); break;
            case 4: r10->selection_index = 0x17; break;
            case 5: r10->selection_index = 0x19; break;
            default: r10->selection_index = 1; break;
            }
            r3->selection_index = 2;
            r3->hidden = 0;

            if (widget->focused_child == (widget_instance *)0) {
                widget->selection_index = 0;
                widget->focused_child = widget->first_child;
                return;
            }
        }
    }
}

} // namespace halo::interface

namespace halo::interface {

void server_list_menu_update(widget_instance *widget)
{
    halo::interface::MenuListView(widget).update();
}

}
