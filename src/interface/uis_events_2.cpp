/**
 * Widget event handlers reachable only through ui_event_function_table. Each handler takes the widget instance,
 * the event record and an out-flag and returns whether the event was consumed.
 */

#include "win32.h"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

#include "halo/interface/uis_event_handlers.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern uint8_t *network_client;
extern int32_t selected_saved_item;
extern uint8_t saved_item_working_copy[0x1ffc];
extern widget_history_node *ui_widget_history[3];
extern heap *widget_memory_pool;
extern growable_array ui_lists[3];
extern int32_t ui_list_current;
extern uint8_t ui_list_has_default;
extern uint16_t missing_string_text[];
extern int32_t profile_slot_lookup_cache_00692ac8;
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
}

namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

/** Local helper shared by the handlers of this file. */
static void widget_history_pop(int16_t controller)
{
    widget_history_node *node;

    if (controller == -1) {
        controller = 0;
    }
    node = ui_widget_history[controller];
    if (node != 0) {
        heap_block *block = (heap_block *)((uint8_t *)node - 0x10);
        uint32_t size = block->size & 0x7fffffff;

        ui_widget_history[controller] = node->next;
        halo::memory::heap_unlink_block(block, widget_memory_pool);
        widget_memory_pool->bytes_allocated -= (int32_t)size;
        widget_memory_pool->allocation_count -= 1;
    }
}

/** Local helper shared by the handlers of this file. */
static int32_t list_item_id(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].id;
    }
    return -1;
}

}

/**
 * Slot 32 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49dbc0
 */
uint8_t UiEventHandlers::event_49dbc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8
                  : network_client != 0 ? network_client + 0xb14 : 0;
    int16_t key;
    int32_t i;

    if (game == 0 || game[0x138] != 1 || network_client == 0) {
        return 1;
    }
    key = *(int16_t *)network_client;
    if (key == -1) {
        return 1;
    }
    for (i = 0; i < 0x10; i++) {
        uint8_t *entry = game + 0x1a2 + i * 0x20;

        if (halo::networking::network_player_entry_validate((network_player_entry *)entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key &&
            (int16_t)(int8_t)entry[0x1d] == event[1]) {
            uint32_t copy[8];

            memcpy(copy, entry, sizeof(copy));
            ((uint8_t *)copy)[0x1e] = (uint8_t)(((uint8_t *)copy)[0x1e] == 0);
            halo::networking::network_game_record_message_send((network_client_globals *)network_client, copy);
            return 1;
        }
    }
    return 1;
}

/**
 * Slot 33 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49dca0
 */
uint8_t UiEventHandlers::event_49dca0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *client = network_client;
    int16_t *state;
    uint8_t *game;

    if (client == 0) {
        return 1;
    }
    state = (int16_t *)(client + 0xeda);
    if (*state == 1) {
        halo::cseries::time_query_performance_counter_ms();
    }
    if (*state != 2) {
        return 1;
    }
    game = halo::networking::globals().server != 0 ? (uint8_t *)halo::networking::globals().server + 8 : network_client != 0 ? network_client + 0xb14 : 0;
    if (network_client != 0 && *(int16_t *)network_client != -1) {
        int16_t key = *(int16_t *)network_client;
        int16_t i;

        for (i = 0; i < 0x10; i++) {
            uint8_t *entry = game + i * 0x20 + 0x1a2;

            if (halo::networking::network_player_entry_validate((network_player_entry *)entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key &&
                (int16_t)(int8_t)entry[0x1d] == event[1]) {
                return 1;
            }
        }
    }
    halo::networking::network_game_settings_ack_send(client, (int16_t)(uint16_t)event[1]);
    return 1;
}

/**
 * Slot 44 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49e7e0
 */
uint8_t UiEventHandlers::event_49e7e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    static const int32_t lives[] = {5, 10, 15, 25, 50};
    static const int32_t times[] = {0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68};
    widget_instance *parent = widget->parent->parent;
    widget_instance *group;
    int16_t selection;

    if (variant == 0) {
        return 0;
    }
    group = parent->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->engine.slayer.death_bonus = (uint8_t)selection;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->engine.slayer.kill_in_order = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->engine.slayer.kill_penalty = (uint8_t)selection;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 4) {
        ((struct game_variant *)variant)->score_limit = lives[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->teams = (uint8_t)(selection == 0);
    }
    selection = first_list_child(group->next_sibling)->selection_index;
    if (selection >= 0 && selection <= 6) {
        ((struct game_variant *)variant)->time_limit = times[selection];
    }
    widget_history_pop(parent->controller_index);
    return 1;
}

/**
 * Slot 45 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49ea50
 */
uint8_t UiEventHandlers::event_49ea50(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    static const int32_t lives[] = {1, 2, 5, 10, 15};
    static const int32_t times[] = {0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68};
    widget_instance *parent = widget->parent->parent;
    widget_instance *group;
    int16_t selection;

    if (variant == 0) {
        return 0;
    }
    group = parent->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 3) {
        ((struct game_variant *)variant)->engine.oddball.trait_with_ball = selection;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 3) {
        ((struct game_variant *)variant)->engine.oddball.trait_without_ball = selection;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 2) {
        ((struct game_variant *)variant)->engine.oddball.speed_with_ball = selection == 0 ? 1 : selection == 1 ? 0 : 2;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 2) {
        ((struct game_variant *)variant)->engine.oddball.ball_type = selection;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->engine.oddball.random_start = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 0xf) {
        ((struct game_variant *)variant)->engine.oddball.ball_count = selection + 1;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 4) {
        ((struct game_variant *)variant)->score_limit = lives[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->teams = (uint8_t)(selection == 0);
    }
    selection = first_list_child(group->next_sibling)->selection_index;
    if (selection >= 0 && selection <= 6) {
        ((struct game_variant *)variant)->time_limit = times[selection];
    }
    widget_history_pop(parent->controller_index);
    return 1;
}

/**
 * Slot 46 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49edc0
 */
uint8_t UiEventHandlers::event_49edc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    static const int32_t lives[] = {1, 3, 5, 10, 15, 25};
    static const int32_t times[] = {0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68};
    widget_instance *parent = widget->parent->parent;
    widget_instance *group;
    int16_t selection;

    if (variant == 0) {
        return 0;
    }
    group = parent->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 2) {
        ((struct game_variant *)variant)->engine.race.team_scoring = selection;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 2) {
        ((struct game_variant *)variant)->engine.race.race_type = selection;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 5) {
        ((struct game_variant *)variant)->score_limit = lives[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->teams = (uint8_t)(selection == 0);
    }
    selection = first_list_child(group->next_sibling)->selection_index;
    if (selection >= 0 && selection <= 6) {
        ((struct game_variant *)variant)->time_limit = times[selection];
    }
    widget_history_pop(parent->controller_index);
    return 1;
}

/**
 * Slot 47 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49f030
 */
uint8_t UiEventHandlers::event_49f030(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    static const int32_t kills[] = {0, 1, 3, 5};
    static const float scales[] = {0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f};
    static const int32_t times[] = {0, 0x96, 0x12c, 0x1c2};
    widget_instance *group;
    uint32_t *flags;
    int16_t selection;

    if (variant == 0) {
        return 0;
    }
    flags = (uint32_t *)(variant + 0x38);
    group = widget->parent->parent->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 3) {
        ((struct game_variant *)variant)->lives_per_round = kills[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 5) {
        ((struct game_variant *)variant)->health = scales[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0) {
        *flags &= ~8u;
    } else if (selection == 1) {
        *flags |= 8;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 3) {
        ((struct game_variant *)variant)->respawn_time = times[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 3) {
        ((struct game_variant *)variant)->respawn_time_growth = times[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        ((struct game_variant *)variant)->odd_man_out = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0) {
        *flags |= 0x10;
    } else if (selection == 1) {
        *flags &= ~0x10u;
    }
    group = group->next_sibling;
    if (group != 0) {
        selection = first_list_child(group)->selection_index;
        if (selection >= 0 && selection <= 3) {
            ((struct game_variant *)variant)->suicide_penalty = times[selection];
        }
    }
    return 1;
}

/**
 * Slot 48 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49f300
 */
uint8_t UiEventHandlers::event_49f300(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    uint32_t *flags;
    int16_t selection;

    if (variant == 0) {
        return 0;
    }
    flags = (uint32_t *)(variant + 0x38);
    group = widget->parent->parent->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection == 0) {
        *flags |= 4;
    } else if (selection == 1) {
        *flags &= ~4u;
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 0xd) {
        ((struct game_variant *)variant)->weapon_set = selection;
    }
    selection = first_list_child(group->next_sibling)->selection_index;
    if (selection == 0) {
        *flags &= ~0x20u;
    } else if (selection == 1) {
        *flags |= 0x20;
    }
    return 1;
}

/**
 * Slot 49 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49f470
 */
uint8_t UiEventHandlers::event_49f470(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    uint32_t *flags;

    if (variant == 0) {
        return 0;
    }
    flags = (uint32_t *)(variant + 0x38);
    group = widget->parent->parent->first_child;
    switch (first_list_child(group)->selection_index) {
    case 0:
        ((struct game_variant *)variant)->objective_indicator = 0;
        break;
    case 1:
        ((struct game_variant *)variant)->objective_indicator = 1;
        break;
    case 2:
        ((struct game_variant *)variant)->objective_indicator = 2;
        break;
    }
    group = group->next_sibling;
    switch (first_list_child(group)->selection_index) {
    case 0:
        *flags = (*flags & ~0x40u) | 1;
        break;
    case 1:
        *flags |= 0x41;
        break;
    case 2:
        *flags &= ~0x41u;
        break;
    }
    switch (first_list_child(group->next_sibling)->selection_index) {
    case 0:
        *flags |= 2;
        break;
    case 1:
        *flags &= ~2u;
        break;
    }
    return 1;
}

/**
 * Slot 50 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49f560
 */
uint8_t UiEventHandlers::event_49f560(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *child;
    int32_t i;

    if (variant == 0) {
        return 0;
    }
    switch (((struct game_variant *)variant)->game_engine_index) {
    case 2:
        widget->selection_index = 2;
        break;
    case 3:
        widget->selection_index = 3;
        break;
    case 4:
        widget->selection_index = 1;
        break;
    case 5:
        widget->selection_index = 4;
        break;
    default:
        widget->selection_index = 0;
        break;
    }
    child = widget->first_child;
    for (i = 0; i < widget->selection_index && child != 0; i++) {
        child = child->next_sibling;
    }
    widget->focused_child = child;
    return 1;
}

/**
 * Slot 51 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49f610
 */
uint8_t UiEventHandlers::event_49f610(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    uint16_t *text;

    if (variant == 0) {
        return 0;
    }
    text = (uint16_t *)halo::memory::heap_reallocate(widget->text, 0x100, widget_memory_pool);
    widget->text = text;
    if (text != 0) {
        wcsncpy((wchar_t *)text, (const wchar_t *)variant, 0x7f);
        text[0x7f] = 0;
    }
    return 1;
}

/**
 * Slot 52 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49f680
 */
uint8_t UiEventHandlers::event_49f680(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    int32_t value;

    if (variant == 0) {
        return 0;
    }
    group = widget->first_child;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->engine.ctf.assault == 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->engine.ctf.single_flag_time;
    first_list_child(group)->selection_index = (int16_t)(value == 0x708 ? 1 : value == 0xe10 ? 2 : value == 0x1518 ? 3 :
        value == 0x2328 ? 4 : value == 0x4650 ? 5 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->engine.ctf.flag_must_reset == 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->engine.ctf.flag_at_home_to_score == 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->score_limit;
    first_list_child(group)->selection_index = (int16_t)(value == 3 ? 1 : value == 5 ? 2 : value == 10 ? 3 : value == 15 ? 4 : 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->time_limit;
    first_list_child(group)->selection_index = (int16_t)(value == 0x4650 ? 1 : value == 0x6978 ? 2 : value == 0x8ca0 ? 3 :
        value == 0xafc8 ? 4 : value == 0xd2f0 ? 5 : value == 0x13c68 ? 6 : 0);
    return 1;
}

/**
 * Slot 53 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49f8f0
 */
uint8_t UiEventHandlers::event_49f8f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    int32_t value;

    if (variant == 0) {
        return 0;
    }
    group = widget->first_child;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->engine.king.moving_hill == 0 ? 1 : 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->score_limit;
    first_list_child(group)->selection_index = (int16_t)(value == 2 ? 1 : value == 5 ? 2 : value == 10 ? 3 : value == 15 ? 4 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->teams == 0 ? 1 : 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->time_limit;
    first_list_child(group)->selection_index = (int16_t)(value == 0x4650 ? 1 : value == 0x6978 ? 2 : value == 0x8ca0 ? 3 :
        value == 0xafc8 ? 4 : value == 0xd2f0 ? 5 : value == 0x13c68 ? 6 : 0);
    return 1;
}

/**
 * Slot 54 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49fad0
 */
uint8_t UiEventHandlers::event_49fad0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    int32_t value;

    if (variant == 0) {
        return 0;
    }
    group = widget->first_child;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->engine.slayer.death_bonus == 1);
    group = group->next_sibling;
    if (((struct game_variant *)variant)->engine.slayer.kill_in_order == 0) {
        first_list_child(group)->selection_index = 1;
    } else if (((struct game_variant *)variant)->engine.slayer.kill_in_order == 1) {
        first_list_child(group)->selection_index = 0;
    }
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->engine.slayer.kill_penalty == 1);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->score_limit;
    first_list_child(group)->selection_index = (int16_t)(value == 10 ? 1 : value == 15 ? 2 : value == 25 ? 3 : value == 50 ? 4 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->teams == 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->time_limit;
    first_list_child(group)->selection_index = (int16_t)(value == 0x4650 ? 1 : value == 0x6978 ? 2 : value == 0x8ca0 ? 3 :
        value == 0xafc8 ? 4 : value == 0xd2f0 ? 5 : value == 0x13c68 ? 6 : 0);
    return 1;
}

/**
 * Slot 55 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x49fd30
 */
uint8_t UiEventHandlers::event_49fd30(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    int32_t value;

    if (variant == 0) {
        return 0;
    }
    group = widget->first_child;
    value = ((struct game_variant *)variant)->engine.oddball.trait_with_ball;
    first_list_child(group)->selection_index = (int16_t)((uint32_t)value <= 3 ? value : 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->engine.oddball.trait_without_ball;
    first_list_child(group)->selection_index = (int16_t)((uint32_t)value <= 3 ? value : 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->engine.oddball.speed_with_ball;
    first_list_child(group)->selection_index = (int16_t)(value == 0 ? 1 : value == 2 ? 2 : 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->engine.oddball.ball_type;
    first_list_child(group)->selection_index = (int16_t)(value == 1 ? 1 : value == 2 ? 2 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->engine.oddball.random_start == 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->engine.oddball.ball_count;
    first_list_child(group)->selection_index = (int16_t)(value > 0 && value <= 0x10 ? value - 1 : 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->score_limit;
    first_list_child(group)->selection_index = (int16_t)(value == 2 ? 1 : value == 5 ? 2 : value == 10 ? 3 : value == 15 ? 4 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->teams == 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->time_limit;
    first_list_child(group)->selection_index = (int16_t)(value == 0x4650 ? 1 : value == 0x6978 ? 2 : value == 0x8ca0 ? 3 :
        value == 0xafc8 ? 4 : value == 0xd2f0 ? 5 : value == 0x13c68 ? 6 : 0);
    return 1;
}

/**
 * Slot 57 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a02a0
 */
uint8_t UiEventHandlers::event_4a02a0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    int32_t value;
    uint32_t flags;

    if (variant == 0) {
        return 0;
    }
    flags = ((struct game_variant *)variant)->flags;
    group = widget->first_child;
    value = ((struct game_variant *)variant)->lives_per_round;
    first_list_child(group)->selection_index = (int16_t)(value == 1 ? 1 : value == 3 ? 2 : value == 5 ? 3 : 0);
    group = group->next_sibling;
    value = (int32_t)((double)((struct game_variant *)variant)->health * -10.0);
    first_list_child(group)->selection_index = (int16_t)(value == -10 ? 1 : value == -15 ? 2 : value == -20 ? 3 :
        value == -30 ? 4 : value == -40 ? 5 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)((flags >> 3) & 1);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->respawn_time;
    first_list_child(group)->selection_index = (int16_t)(value == 0x96 ? 1 : value == 0x12c ? 2 : value == 0x1c2 ? 3 : 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->respawn_time_growth;
    first_list_child(group)->selection_index = (int16_t)(value == 0x96 ? 1 : value == 0x12c ? 2 : value == 0x1c2 ? 3 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((struct game_variant *)variant)->odd_man_out == 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(((flags >> 4) & 1) == 0);
    group = group->next_sibling;
    value = ((struct game_variant *)variant)->suicide_penalty;
    first_list_child(group)->selection_index = (int16_t)(value == 0x96 ? 1 : value == 0x12c ? 2 : value == 0x1c2 ? 3 : 0);
    return 1;
}

/**
 * Slot 58 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0590
 */
uint8_t UiEventHandlers::event_4a0590(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    uint32_t flags;
    uint32_t value;

    if (variant == 0) {
        return 0;
    }
    flags = ((struct game_variant *)variant)->flags;
    group = widget->first_child;
    first_list_child(group)->selection_index = (int16_t)(((flags >> 2) & 1) != 0 ? 0 : 1);
    group = group->next_sibling;
    value = *(uint32_t *)&((struct game_variant *)variant)->weapon_set;
    first_list_child(group)->selection_index = (int16_t)(value <= 0xd ? value : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)((((struct game_variant *)variant)->flags >> 5) & 1);
    return 1;
}

/**
 * Slot 59 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0700
 */
uint8_t UiEventHandlers::event_4a0700(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    widget_instance *group;
    int32_t value;
    uint32_t flags;

    if (variant == 0) {
        return 0;
    }
    group = widget->first_child;
    value = ((struct game_variant *)variant)->objective_indicator;
    first_list_child(group)->selection_index = (int16_t)(value == 1 || value == 2 ? value : 0);
    group = group->next_sibling;
    flags = ((struct game_variant *)variant)->flags;
    first_list_child(group)->selection_index = (int16_t)((flags & 1) == 0 ? 2 : (flags >> 6) & 1);
    first_list_child(group->next_sibling)->selection_index = (int16_t)(((((struct game_variant *)variant)->flags >> 1) & 1) == 0 ? 1 : 0);
    return 1;
}

/**
 * Slot 60 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a07e0
 */
uint8_t UiEventHandlers::event_4a07e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *root;

    if (halo::interface::saved_item_has_unsaved_changes() != 0) {
        int32_t item = selected_saved_item;

        if (item != -1 && (item & 0xf) <= 1 && ((item >> 30) & 1) != 0 && (uint8_t)halo::interface::saved_item_name_changed() == 0) {
            halo::interface::saved_item_name_edit_begin();
            return 0;
        }
        return halo::interface::player_profile_save();
    }
    selected_saved_item = -1;
    root = widget;
    while (root->parent != 0) {
        root = root->parent;
    }
    halo::interface::widget_close(root);
    *out_handled = 1;
    return 0;
}

/**
 * Slot 61 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0860
 */
uint8_t UiEventHandlers::event_4a0860(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    uint8_t *indices;
    datum_index strings;
    int32_t i;

    for (i = 0; i < 3; i++) {
        ui_lists[i].element_size = 0x10;
        ui_lists[i].count = 0;
        ui_lists[i].data = 0;
    }
    ui_list_current = -1;
    ui_list_has_default = 0;
    if (profile != 0) {
        int16_t colour = *(int16_t *)(profile + 0x11a);

        colour = (int16_t)(colour < 0 ? 0 : colour > 0x11 ? 0x11 : colour);
        *(int16_t *)(profile + 0x11a) = colour;
        widget->selection_index = colour;
        *(int16_t *)&((struct widget_instance *)widget)->text = *(int16_t *)(profile + 0x11a);
        *(int16_t *)((uint8_t *)widget + 0x3e) = -1;
    }
    indices = (uint8_t *)halo::memory::heap_reallocate(widget->list_items, 0x12, widget_memory_pool);
    widget->list_items = indices;
    if (indices == 0) {
        return 1;
    }
    strings = halo::cache::tag_lookup(halo::groups::unicode_string_list, (char *)"ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile_edit\\color_edit\\colors_list");
    for (i = 0; i < 0x12; i++) {
        uint16_t *text = missing_string_text;
        uint8_t is_default;
        uint32_t index;

        ((uint8_t *)widget->list_items)[i] = (uint8_t)i;
        if (strings != halo::k_dword_none) {
            uint8_t *list = halo::interface::tag_data<uint8_t>(strings);

            if (i < *(int32_t *)list) {
                uint8_t *element = *(uint8_t **)(list + 4) + i * 0x14;
                uint32_t size = *(uint32_t *)element;

                if ((int32_t)size > 0) {
                    text = *(uint16_t **)(element + 0xc);
                    text[(size >> 1) - 1] = 0;
                }
            }
        }
        is_default = (uint8_t)(i == widget->selection_index);
        index = halo::memory::growable_array_add_element(&ui_lists[0]);
        if (index != halo::k_dword_none) {
            ui_list_item *item = (ui_list_item *)ui_lists[0].data + index;
            uint16_t *copy;

            item->data = 0;
            copy = (uint16_t *)GlobalAlloc(0, (uint32_t)wcslen((const wchar_t *)text) * 2 + 2);
            item->name = copy;
            item->id = i;
            item->is_default = is_default;
            if (is_default) {
                ui_list_has_default = 1;
            }
            wcscpy((wchar_t *)copy, (const wchar_t *)text);
        }
    }
    widget->item_count = 0x12;
    return 1;
}

/**
 * Slot 63 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0a80
 */
uint8_t UiEventHandlers::event_4a0a80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    int32_t id = list_item_id(*(int16_t *)&((struct widget_instance *)widget)->text);

    if (profile == 0) {
        return 0;
    }
    *(int16_t *)(profile + 0x11a) = (int16_t)id;
    return 1;
}

/**
 * Slot 64 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0ae0
 */
uint8_t UiEventHandlers::event_4a0ae0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *list = widget->first_child;
    int32_t item = ((int32_t *)list->list_items)[list->selection_index];

    profile_slot_lookup_cache_00692ac8 = -1;
    if (item != -1) {
        if (item < 0) {
            halo::interface::saved_item_select(item);
            return 1;
        }
        if (quit_confirm_error_string_index == -1) {
            quit_confirm_error_string_index = 0x1f;
            quit_confirm_error_unknown_ae = -1;
            quit_confirm_error_modal = 1;
            quit_confirm_error_is_error = 0;
        }
    }
    halo::interface::widget_play_sound_effect(4);
    return 0;
}

/**
 * Slot 66 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0bc0
 */
uint8_t UiEventHandlers::event_4a0bc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    const uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;

    if (profile == 0 || halo::interface::virtual_keyboard_open((uint16_t *)(profile + 2), 0x18, 8) == 0) {
        return 0;
    }
    return 1;
}

/**
 * Slot 67 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0c00
 */
uint8_t UiEventHandlers::event_4a0c00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *root;

    halo::interface::widget_play_sound_effect(2);
    if (halo::interface::saved_item_has_unsaved_changes() != 0) {
        uint8_t saved = halo::interface::player_profile_save();

        if (saved != 0) {
            return saved;
        }
    }
    selected_saved_item = -1;
    root = widget;
    while (root->parent != 0) {
        root = root->parent;
    }
    halo::interface::widget_close(root);
    *out_handled = 1;
    return 0;
}

/**
 * Slot 68 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0c60
 */
uint8_t UiEventHandlers::event_4a0c60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *list;

    if (profile == 0) {
        return 0;
    }
    list = first_list_child(widget->first_child);
    list->selection_index = (int16_t)(profile[0x12d] <= 3 ? profile[0x12d] : 0);
    list = first_list_child(widget->first_child->next_sibling);
    list->selection_index = (int16_t)(profile[0x12c] <= 4 ? profile[0x12c] : 0);
    return 1;
}

/**
 * Slot 69 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0d60
 */
uint8_t UiEventHandlers::event_4a0d60(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *group;
    uint8_t value;

    if (profile == 0) {
        return 0;
    }
    group = widget->first_child;
    first_list_child(group)->selection_index = (int16_t)(profile[0x12f] == 0 ? 1 : 0);
    group = group->next_sibling;
    value = profile[0x12e];
    first_list_child(group)->selection_index = (int16_t)(value > 0 && value <= 10 ? value - 1 : 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(profile[0x130] == 1);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(profile[0x131] == 0);
    group = group->next_sibling;
    first_list_child(group)->selection_index = (int16_t)(profile[0x132] == 0);
    return 1;
}

/**
 * Slot 70 of ui_event_function_table (widget event handler); only reachable through that table.
 *
 * @address 0x4a0e90
 */
uint8_t UiEventHandlers::event_4a0e90(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    int16_t selection;

    if (profile == 0) {
        return 0;
    }
    selection = first_list_child(widget->first_child)->selection_index;
    if (selection >= 0 && selection <= 3) {
        profile[0x12d] = (uint8_t)selection;
    }
    selection = first_list_child(widget->first_child->next_sibling)->selection_index;
    if (selection >= 0 && selection <= 4) {
        profile[0x12c] = (uint8_t)selection;
    }
    return 1;
}

}
