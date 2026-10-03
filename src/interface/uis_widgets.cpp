/**
 * Widget text refresh and status-flag synchronisation helpers.
 */

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <wchar.h>
#include "cache.h"

#include "halo/interface/uis_widgets.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"

extern "C" {
extern int16_t profile_slot_id[];
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles];
extern uint16_t global_text_field_00719278[0x40];
extern heap *widget_memory_pool;
extern hud_messaging_globals *hud_messaging;
}

namespace halo::ui {

/**
 * Walks up to the root ancestor widget, and if profile slot 0's id matches the root's controller_index, mirrors
 * that profile's status byte (profile+0x12f) into this widget's selection_index as a 0/1 flag.
 *
 * @address 0x4a7360
 */
void UiWidgets::widget_sync_profile_status_flag(widget_instance *widget)
{
    widget_instance *root;
    int32_t slot;
    uint8_t status;

    root = widget;
    while (root->parent != 0) {
        root = root->parent;
    }

    slot = (profile_slot_id[0] == root->controller_index) ? 0 : -1;
    if (slot == -1) {
        slot = 0;
    }

    status = 0;
    if (slot != -1) {
        status = ((uint8_t *)&profile_globals_block[slot])[0x12f];
    }
    widget->selection_index = (status != 0);
}

/**
 * Original UI routine; see docs/original/interface/ui_widget_text_ensure_and_refresh.c.txt for the recovery
 * notes.
 *
 * @address 0x4a4fe0
 */
void UiWidgets::widget_text_ensure_and_refresh(widget_instance *widget)
{
    if (widget->text == (void *)0) {
        uint32_t *block = (uint32_t *)halo::memory::heap_reallocate(widget->text, 0x80, widget_memory_pool);

        widget->text = block;
        if (block != (uint32_t *)0) {
            int32_t i;

            for (i = 0; i < 0x20; i++) {
                block[i] = 0;
            }
        }
    }
    if (widget->text != (void *)0) {
        wcsncpy((wchar_t *)((uint16_t *)widget->text), (const wchar_t *)global_text_field_00719278, 0x3f);
        ((uint16_t *)widget->text)[0x3f] = 0;
    }
}

/**
 * Original UI routine; see docs/original/interface/ui_widget_text_from_hud_objective.c.txt for the recovery
 * notes.
 *
 * @address 0x4a6770
 */
void UiWidgets::widget_text_from_hud_objective(widget_instance *widget)
{
    uint8_t *entry = *(uint8_t **)&hud_messaging->objective_text;
    uint8_t *text_tag;
    uint16_t *text;
    int32_t length;
    uint16_t *buffer;

    if (entry == 0) {
        return;
    }
    text_tag = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)((uint8_t *)halo::scenario::globals().scenario + 0x5a0) & 0xffff].data;
    text = (uint16_t *)(*(uint8_t **)(text_tag + 0xc) + (uint32_t)*(uint16_t *)(entry + 0x20) * 2);
    if (text == 0 || *text == 0) {
        return;
    }
    length = wcslen((const wchar_t *)text);
    if (length <= 0) {
        return;
    }
    buffer = (uint16_t *)halo::memory::heap_reallocate(widget->text, (uint16_t)(length * 2 + 2), widget_memory_pool);
    widget->text = buffer;
    if (buffer == 0) {
        return;
    }
    wcsncpy((wchar_t *)buffer, (const wchar_t *)text, (size_t)length);
    buffer[length] = 0;
}

}
