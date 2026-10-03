#include "halo/interface/ifr1_hud_messaging.hpp"
#include "halo/text/api.hpp"
#include <wchar.h>
#include <stdint.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/scenario/api.hpp"

extern "C" {
extern hud_messaging_globals *hud_messaging;
extern game_time_globals *game_time;
extern hud_message_slot *hud_message_find_slot(int32_t source, hud_player_messaging_state *record,
                                               uint8_t source_kind);
extern int32_t hud_chat_message_count;
extern int32_t hud_chat_message_expiry[8];
extern void *chat_gui_root_handle;
extern chat_gui_find_object_fn chat_gui_find_object;
extern void *chat_listbox_gui_find_object_arg;
extern chat_gui_find_child_fn chat_gui_find_child;
extern chat_gui_set_property_int_fn chat_gui_set_property_int;
extern chat_gui_finalize_fn chat_gui_finalize;
extern chat_gui_release_fn chat_gui_release;
extern uint32_t hud_chat_listbox_remove_oldest(void);
extern HUDGlobals *hud_globals_tag_data;
extern player_globals *local_player_globals;
extern uint16_t *empty_wide_string_pointer;
extern void chimera__hud_message(int16_t local_player_index, const uint16_t *text);
extern uint16_t *hud_get_message_string(int32_t message_index);
extern data_array *player_data;
extern data_array *object_data;
extern void *global_zero_vector3d_pointer;
extern int16_t item_type_to_message_stage(int16_t item_type_code);
extern int16_t item_type_to_animation_stage(int16_t message_stage);
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern int16_t network_game_mode;
extern network_server_globals *network_server;
extern uint8_t network_message_scratch[0x7ff8];
extern void hud_add_item_message(int16_t local_player_index, int32_t source, uint8_t source_kind,
                                 int16_t count);
extern int32_t message_delta_encode_message(void *buffer, int32_t buffer_size, int32_t flag, int32_t message_type,
                                            int32_t changed_offset, void **items, int32_t type_offset,
                                            int32_t count, char force_changed);
extern network_machine *network_machine_find_by_id(network_server_globals *server, int16_t machine_id);
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, int32_t unknown_0,
                                               void *data, int32_t bits, int32_t reliable, int32_t unknown_a,
                                               int32_t unknown_b, int32_t priority);
extern hud_globals_flags *hud_flags;
}

namespace halo::interface {

/**
 * Adds a new timestamped text message into local player `local_player_index`'s HUD message slot array (reusing
 * a free slot or the oldest one through hud_message_find_slot) and clears the reserved action line flag.
 * blam-cc: local_player_index -> AX
 *
 * @address 0x4ae180
 */
void HudMessaging::hud_message(int16_t local_player_index, const wchar_t *text)
{
    if (local_player_index != -1) {
        hud_player_messaging_state *player_record =
            (hud_player_messaging_state *)((uint8_t *)hud_messaging + local_player_index * 0x460);
        hud_message_slot *slot = hud_message_find_slot(-1, player_record, 0);

        wcsncpy((wchar_t *)slot->text, text, 0x3f);
        slot->source = -1;
        slot->timestamp = game_time->game_time;
        slot->active = 1;
        slot->sequence = hud_messaging->next_sequence;
        hud_messaging->next_sequence++;
        player_record->prompt_changed = 0;
    }
}

/**
 * Appends a new line of text to the on-screen chat/message listbox GUI control, evicting the oldest entry
 * first if 8 or more are already shown, and stamps its expiry 8000 ms past the current performance-counter
 * time.
 *
 * @address 0x4ab4b0
 */
void HudMessaging::multiplayer_message(const wchar_t *text)
{
    int64_t counter;
    int64_t now_ms;

    if (hud_chat_message_count > 7) {
        hud_chat_listbox_remove_oldest();
    }

    if (chat_gui_find_object != 0) {
        void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
        if (gui_object != 0) {
            void *listbox = chat_gui_find_child(gui_object, (const uint16_t *)L"oListbox");
            if (listbox != 0) {
                chat_gui_set_property_int(listbox, 0x180, 0, text);
                chat_gui_set_property_int(listbox, 0x115, 2, 0);
                chat_gui_finalize(gui_object);
            }
            chat_gui_release(gui_object);
        }
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (counter * 1000) / halo::cseries::globals().performance_frequency;
    hud_chat_message_expiry[hud_chat_message_count] = (int32_t)now_ms + 8000;
    hud_chat_message_count = hud_chat_message_count + 1;
}

/**
 * Original engine function hud_add_item_message; the author notes are in
 * docs/original/interface/hud_add_item_message.txt.
 * blam-cc: EAX -> local_player_index, ECX -> source, BL -> source_kind
 *
 * @address 0x4ae400
 */
void HudMessaging::add_item_message(int16_t local_player_index, int32_t source, uint8_t source_kind, int16_t count)
{
    hud_player_messaging_state *record;
    hud_message_slot *slot;

    if (local_player_index == -1) {
        return;
    }
    record = &hud_messaging->players[local_player_index];
    slot = hud_message_find_slot(source, record, source_kind);
    if (slot->active == 0) {
        slot->count = 0;
    }
    slot->count = (int16_t)(slot->count + count);
    slot->source = source;
    slot->source_kind = source_kind;
    slot->timestamp = game_time->game_time;
    slot->active = 1;
    slot->sequence = hud_messaging->next_sequence;
    hud_messaging->next_sequence++;
    record->prompt_changed = 0;
}

/**
 * Clears the local player's 4 message-slot active flags, plays the HUDGlobals checkpoint sound when is_begin
 * is set and one is configured, then displays the checkpoint begin/end text (only when is_begin is set and
 * this build's one local player slot is in use) if a string is configured for it.
 * blam-cc: DL -> is_begin
 *
 * @address 0x4aa310
 */
void HudMessaging::display_checkpoint_message(uint8_t is_begin)
{
    HUDGlobals *hud_globals = (HUDGlobals *)hud_globals_tag_data;
    int16_t message_index = is_begin ? hud_globals->checkpoint_begin_text : hud_globals->checkpoint_end_text;
    int32_t i;

    for (i = 0; i < 4; i = i + 1) {
        hud_messaging->players[0].messages[i].active = 0;
    }

    if (is_begin) {
        int32_t sound_tag_id = *(int32_t *)&hud_globals->checkpoint_sound.tag_id;
        if (sound_tag_id != -1) {
            hud_sound_start_parameters parameters;

            parameters.unknown_00 = 0;
            parameters.scale = 1.0f;
            parameters.gain = 1.0f;
            halo::sound::sound_play_new((datum_index)sound_tag_id, (sound_location *)&parameters, -1, 0, 0, 0, 0);
        }
    }

    if (message_index != -1 && local_player_globals->local_players[0] != (datum_index)-1) {
        const uint16_t *text = empty_wide_string_pointer;
        int32_t string_list_tag_id = *(int32_t *)&hud_globals->item_message_text.tag_id;
        if (string_list_tag_id != -1) {
            int32_t *string_list_tag_data = (int32_t *)halo::cache::globals().tag_instances[string_list_tag_id & 0xffff].data;
            if (string_list_tag_data != 0 && message_index > -1 && message_index < *string_list_tag_data) {
                text = halo::text::text_string_list_get_string((datum_index)string_list_tag_id, message_index);
            }
        }
        chimera__hud_message(0, text);
    }
}

/**
 * Clears the local player's 4 message-slot active flags, then displays the HUDGlobals loading begin/end text
 * (picked by is_begin, see header note) if a string is configured for it.
 * blam-cc: AL -> is_begin
 *
 * @address 0x4aa2a0
 */
void HudMessaging::display_loading_message(uint8_t is_begin)
{
    HUDGlobals *hud_globals = (HUDGlobals *)hud_globals_tag_data;
    int16_t message_index = is_begin ? hud_globals->loading_begin_text : hud_globals->loading_end_text;
    int32_t i;

    for (i = 0; i < 4; i = i + 1) {
        hud_messaging->players[0].messages[i].active = 0;
    }

    if (message_index != -1) {
        chimera__hud_message(local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1,
                             hud_get_message_string(message_index));
    }
}

/**
 * Resolves a weapon-HUD message index to its localized string out of the current hud_globals tag's
 * item_message_text string list, or the shared empty string if the index is out of range.
 * blam-cc: message_index -> EDX
 *
 * @address 0x4aa3f0
 */
uint16_t * HudMessaging::get_message_string(int32_t message_index)
{
    HUDGlobals *hud_globals = (HUDGlobals *)hud_globals_tag_data;
    int32_t string_list_tag_id = *(int32_t *)&hud_globals->item_message_text.tag_id;

    if (string_list_tag_id != -1) {
        int32_t *string_list_tag_data = (int32_t *)halo::cache::globals().tag_instances[string_list_tag_id & 0xffff].data;
        if (string_list_tag_data != 0 && message_index > -1 && message_index < *string_list_tag_data) {
            return halo::text::text_string_list_get_string((datum_index)string_list_tag_id, (int16_t)message_index);
        }
    }
    return empty_wide_string_pointer;
}

/**
 * Posts a HUD message (via chimera__hud_message) once for every local player in player_data.
 *
 * @address 0x495f50
 */
void HudMessaging::message_broadcast_to_local_players(const uint16_t *text)
{
    data_iterator iterator;
    player *record;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)0xffffffff;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    record = (player *)halo::memory::data_iterator_next(&iterator);
    while (record != (player *)0) {
        if (record->local_player_index != -1) {
            chimera__hud_message(record->local_player_index, text);
        }
        record = (player *)halo::memory::data_iterator_next(&iterator);
    }
}

/**
 * Original engine function hud_message_compare; the author notes are in
 * docs/original/interface/hud_message_compare.txt.
 *
 * @address 0x4ae500
 */
int32_t HudMessaging::message_compare(const void *a, const void *b)
{
    const hud_message_slot *left = (const hud_message_slot *)a;
    const hud_message_slot *right = (const hud_message_slot *)b;
    int32_t difference;

    difference = right->timestamp - left->timestamp;
    if (difference == 0) {
        difference = right->source - left->source;
        if (difference == 0) {
            difference = (int32_t)right->sequence - (int32_t)left->sequence;
        }
    }
    return difference;
}

/**
 * Original engine function hud_message_find_slot; the author notes are in
 * docs/original/interface/hud_message_find_slot.txt.
 * blam-cc: source -> ESI
 *
 * @address 0x4ae480
 */
hud_message_slot * HudMessaging::message_find_slot(int32_t source, hud_player_messaging_state *record, uint8_t source_kind)
{
    hud_message_slot *candidate = 0;
    int32_t oldest_time = 0x7fffffff;
    int16_t oldest = 0;
    uint16_t i;

    for (i = 0; i < 4; i++) {
        hud_message_slot *slot = &record->messages[(int16_t)i];

        if ((source != -1 && source == slot->source && source_kind == slot->source_kind) || slot->active == 0) {
            candidate = slot;
            if (source == -1 || source == slot->source) {
                break;
            }
        } else if (slot->timestamp < oldest_time) {
            oldest_time = slot->timestamp;
            oldest = (int16_t)i;
        }
    }
    if (candidate != 0) {
        return candidate;
    }
    return &record->messages[oldest];
}

/**
 * (same prototype as first_person_weapon_update.c; pushed as sound, -1, 1.0f, flag) Plays a HUD pickup
 * notification (sound/animation) for an equipment/weapon item, gated on: the object actually existing
 * (object_try_and_get, type mask 4), its tag's "pickup notification" dependency (+0x478) being set, both stage
 * remaps succeeding, and the referenced weapon_hud_interface tag's message table containing a valid entry for
 * the remapped index.
 * blam-cc: ESI -> object_index, ECX -> position, EAX -> forward, stack -> sound, marker, gain, flag
 *
 * @address 0x492990
 */
void HudMessaging::play_pickup_notification(uint32_t object_or_slot_index, int16_t item_type_code)
{
    object_header *header;
    uint8_t *object_base;
    uint32_t tag_index;
    uint8_t *item_tag_data;
    int32_t hud_tag_ref;
    uint32_t hud_tag_index;
    uint8_t *hud_tag_data;
    uint8_t *block_a_base;
    int32_t block_a_count;
    int16_t message_stage;
    int16_t animation_stage;
    int16_t table_entry;
    uint8_t *block_c_base;
    int16_t sub_entry;
    uint8_t *block_d_base;
    int32_t message_index;
    datum_index carried_object;
    uint8_t has_carried_object;
    void *carried_record;

    if (object_or_slot_index == 0xffffffff || item_type_code == -1) {
        return;
    }
    if (object_try_and_get((datum_index)object_or_slot_index, 4) == (void *)0) {
        return;
    }

    header = &((object_header *)object_data->data)[object_or_slot_index & 0xffff];
    object_base = (uint8_t *)header->data;

    tag_index = *(uint32_t *)object_base & 0xffff;
    item_tag_data = *(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances + tag_index * 0x20 + 0x14);

    hud_tag_ref = *(int32_t *)(item_tag_data + 0x478);
    if (hud_tag_ref == -1) {
        return;
    }

    message_stage = item_type_to_message_stage(item_type_code);
    if (message_stage == -1) {
        return;
    }
    animation_stage = item_type_to_animation_stage(message_stage);
    if (animation_stage == -1) {
        return;
    }

    hud_tag_index = (uint32_t)hud_tag_ref & 0xffff;
    hud_tag_data = *(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances + hud_tag_index * 0x20 + 0x14);

    block_a_count = *(int32_t *)(hud_tag_data + 0x48);
    block_a_base = (block_a_count != 0) ? *(uint8_t **)(hud_tag_data + 0x4c) : (uint8_t *)0;

    if (animation_stage < 0) {
        return;
    }
    if (animation_stage >= *(int32_t *)(block_a_base + 0x10)) {
        return;
    }
    table_entry = *(int16_t *)(*(uint8_t **)(block_a_base + 0x14) + animation_stage * 2);
    if (table_entry == -1) {
        return;
    }

    block_c_base = *(uint8_t **)(hud_tag_data + 0x78);
    sub_entry = *(int16_t *)(block_c_base + (int32_t)table_entry * 0xb4 + 0x3c);
    if (sub_entry == -1) {
        return;
    }

    block_d_base = *(uint8_t **)(hud_tag_data + 0x58);
    message_index = *(int32_t *)(block_d_base + (int32_t)sub_entry * 0x14 + 0xc);
    if (message_index == -1) {
        return;
    }

    carried_object = *(datum_index *)(object_base + 0xc0);
    has_carried_object = 0;
    if (carried_object != (datum_index)0xffffffff) {
        carried_record = halo::memory::datum_get(carried_object, player_data);
        if (carried_record != 0 && *(int16_t *)((uint8_t *)carried_record + 2) != -1) {
            has_carried_object = 1;
        }
    }

    halo::sound::sound_start_at_object_marker((datum_index)object_or_slot_index, (Point3D *)global_zero_vector3d_pointer, (Vector3D *)halo::math::globals().global_forward3d_pointer,
                 (datum_index)message_index, -1, 1.0f, has_carried_object);
}

/**
 * Posts an item pickup message for a local player, over the network when the game is networked.
 * blam-cc: count -> EAX, source -> ECX, kind -> DL
 *
 * @address 0x4ae350
 */
void HudMessaging::post_item_message(int16_t count, int32_t source, uint8_t kind, int16_t local_player_index, int8_t machine_id)
{
    hud_item_message payload;
    void *items[2];
    int32_t bits;
    network_machine *machine;

    if (network_game_mode == 0) {
        hud_add_item_message(local_player_index, source, kind, count);
        return;
    }
    payload.item_definition = source;
    payload.count = count;
    items[0] = &payload;
    payload.kind = kind;
    items[1] = 0;
    bits = message_delta_encode_message(network_message_scratch, 0x7ff8, 0, 6, 0, items, 0, 1, 0);
    if (bits <= 0) {
        return;
    }
    machine = network_machine_find_by_id(network_server, (int16_t)machine_id);
    if (machine != 0 && machine->machine_id != -1) {
        network_session_send_to_machine(machine->machine_id, network_server, 1, network_message_scratch, bits, 1, 0,
                                        1, 3);
    }
}

/**
 * Original engine function hud_set_action_text_shown; the author notes are in
 * docs/original/interface/hud_set_action_text_shown.txt.
 * blam-cc: local_player_index -> EAX, shown -> BL
 *
 * @address 0x4ae110
 */
void HudMessaging::set_action_text_shown(int16_t local_player_index, uint8_t shown)
{
    static const uint16_t empty_text[1] = {0};
    hud_player_messaging_state *record = &hud_messaging->players[local_player_index];

    record->prompt_changed |= (uint8_t)(record->message_shown != shown);
    record->message_shown = shown;
    record->message = 0;
    if (shown != 0) {
        record->message = 0;
        wcsncpy((wchar_t *)record->action_text, (const wchar_t *)empty_text, 0xff);
    }
    record->message_shown_copy = shown;
}

/**
 * Points the HUD help text at message message_index of the scenario hud_messages tag; ignored unless
 * show_hud_help_text is on and the scenario has a hud_messages tag.
 *
 * @address 0x4adb30
 */
void HudMessaging::set_help_text(int16_t message_index)
{
    datum_index tag_id;

    if (hud_flags->help_text_shown == 0) {
        return;
    }
    tag_id = *(datum_index *)&halo::scenario::globals().scenario->hud_messages.tag_id;
    if (tag_id == (datum_index)-1) {
        return;
    }
    hud_messaging->help_text =
        (HUDMessageTextMessage *)((HUDMessageText *)halo::cache::globals().tag_instances[tag_id & 0xffff].data)->messages.pointer +
        message_index;
}

/**
 * Original engine function hud_set_message_icon_argument; the author notes are in
 * docs/original/interface/hud_set_message_icon_argument.txt.
 * blam-cc: local_player_index -> EAX, slot -> ESI
 *
 * @address 0x4ae050
 */
void HudMessaging::set_message_icon_argument(int16_t local_player_index, int16_t slot, const hud_messaging_information *information)
{
    hud_player_messaging_state *record = &hud_messaging->players[local_player_index];

    if (record->message_shown == 0 || hud_flags->help_text_shown != 0 || record->message == 0) {
        return;
    }
    record->arguments[slot] = (int32_t)information;
    record->argument_is_string &= (uint8_t)~(uint8_t)(1 << slot);
}

/**
 * Original engine function hud_set_message_string_argument; the author notes are in
 * docs/original/interface/hud_set_message_string_argument.txt.
 * blam-cc: local_player_index -> EAX, slot -> ESI
 *
 * @address 0x4ae0b0
 */
void HudMessaging::set_message_string_argument(int16_t local_player_index, int16_t slot, int16_t string_index, uint8_t from_scenario_names)
{
    hud_player_messaging_state *record = &hud_messaging->players[local_player_index];
    uint8_t *argument;

    if (record->message_shown == 0 || hud_flags->help_text_shown != 0 || record->message == 0) {
        return;
    }
    argument = (uint8_t *)&record->arguments[slot];
    *(int16_t *)argument = string_index;
    argument[2] = from_scenario_names;
    record->argument_is_string |= (uint8_t)(1 << slot);
}

/**
 * Shows message message_index of the scenario hud_messages tag as the objective text for the HUD globals
 * objective up time plus fade time.
 *
 * @address 0x4adb80
 */
void HudMessaging::set_objective_text(int16_t message_index)
{
    HUDMessageText *messages_tag;
    HUDMessageTextMessage *message;
    datum_index tag_id;

    tag_id = *(datum_index *)&halo::scenario::globals().scenario->hud_messages.tag_id;
    if (tag_id == (datum_index)-1) {
        return;
    }
    messages_tag = (HUDMessageText *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
    message = (HUDMessageTextMessage *)messages_tag->messages.pointer + message_index;
    if (message->panel_count != 1) {
        return;
    }
    if (((HUDMessageTextElement *)messages_tag->message_elements.pointer)[message->start_index_of_message_block].type != 0) {
        return;
    }
    hud_messaging->objective_text = message;
    hud_messaging->objective_text_ticks =
        (int16_t)(hud_globals_tag_data->objective_fade_ticks + hud_globals_tag_data->objective_uptime_ticks);
}

/**
 * Shows HUDGlobals hud_messages message message_index as the action message line of a local player, clearing
 * its substitution argument kinds.
 * blam-cc: message_index -> EAX
 *
 * @address 0x4adfc0
 */
void HudMessaging::set_player_message(int16_t message_index, int16_t local_player_index)
{
    hud_player_messaging_state *record;
    datum_index tag_id;

    if (hud_flags->help_text_shown != 0) {
        return;
    }
    tag_id = *(datum_index *)&hud_globals_tag_data->hud_messages.tag_id;
    if (tag_id == (datum_index)-1) {
        return;
    }
    record = &hud_messaging->players[local_player_index];
    if (message_index != -1) {
        HUDMessageText *messages = (HUDMessageText *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
        if ((int32_t)message_index < (int32_t)messages->messages.count) {
            record->message = (HUDMessageTextMessage *)messages->messages.pointer + message_index;
            record->argument_is_string = 0;
            record->message_shown = message_index != -1;
            return;
        }
        message_index = -1;
    }
    record->message_shown = message_index != -1;
}

}
