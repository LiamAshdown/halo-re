/**
 * Widget text refresh and status-flag synchronisation helpers.
 */

#include "crt.h"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
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
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/interface/widget_pool.hpp"
#include "halo/interface/wide_text.hpp"

static auto &profile_slot_id = halo::link::ref<int16_t []>(halo::ui::vars().profile_slot_id);
static auto &profile_globals_block = halo::link::ref<saved_player_profile_slot [k_maximum_local_player_profiles]>(halo::ui::vars().profile_globals_block);
static auto &global_text_field_00719278 = halo::link::ref<uint16_t [0x40]>(halo::ui::vars().global_text_field_00719278);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &hud_messaging = halo::link::ref<hud_messaging_globals *>(halo::ui::vars().hud_messaging);

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
        status = profile_globals_block[slot].profile.look_inverted;
    }
    widget->selection_index = (status != 0);
}

/**
 *
 * @address 0x4a4fe0
 */
void UiWidgets::widget_text_ensure_and_refresh(widget_instance *widget)
{
    if (widget->text == nullptr) {
        uint32_t *block = (uint32_t *)halo::memory::heap_reallocate(widget->text, 0x80, widget_memory_pool);

        widget->text = block;
        if (block != nullptr) {
            int32_t i;

            for (i = 0; i < 0x20; i++) {
                block[i] = 0;
            }
        }
    }
    if (widget->text != nullptr) {
        wcsncpy((wchar_t *)(halo::interface::widget_text(widget)), (const wchar_t *)global_text_field_00719278, 0x3f);
        (halo::interface::widget_text(widget))[0x3f] = 0;
    }
}

/**
 *
 * @address 0x4a6770
 */
void UiWidgets::widget_text_from_hud_objective(widget_instance *widget)
{
    HUDMessageTextMessage *entry = hud_messaging->objective_text;
    HUDMessageText *text_tag;
    uint16_t *text;
    int32_t length;
    uint16_t *buffer;

    if (entry == 0) {
        return;
    }
    text_tag = halo::interface::tag_data<HUDMessageText>(*(uint32_t *)&halo::scenario::globals().scenario->hud_messages.tag_id);
    text = (uint16_t *)((uint8_t *)text_tag->text_data.pointer + (uint32_t)entry->start_index_into_text_blob * 2);
    if (text == 0 || *text == 0) {
        return;
    }
    length = wcslen((const wchar_t *)text);
    if (length <= 0) {
        return;
    }
    buffer = halo::interface::widget_pool_resize_text(widget->text, (uint16_t)(length * 2 + 2));
    widget->text = buffer;
    if (buffer == 0) {
        return;
    }
    wcsncpy((wchar_t *)buffer, (const wchar_t *)text, (size_t)length);
    buffer[length] = 0;
}

}
