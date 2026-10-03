#include "halo/interface/ifr1_hud_messaging.hpp"

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::hud_message.
 * blam-cc: local_player_index -> AX
 *
 * @address 0x4ae180
 */
extern "C" void chimera__hud_message(int16_t local_player_index, const wchar_t *text)
{
    halo::interface::HudMessaging::hud_message(local_player_index, text);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::multiplayer_message.
 *
 * @address 0x4ab4b0
 */
extern "C" void chimera__multiplayer_message(const wchar_t *text)
{
    halo::interface::HudMessaging::multiplayer_message(text);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::add_item_message.
 * blam-cc: EAX -> local_player_index, ECX -> source, BL -> source_kind
 *
 * @address 0x4ae400
 */
extern "C" void hud_add_item_message(int16_t local_player_index, int32_t source, uint8_t source_kind, int16_t count)
{
    halo::interface::HudMessaging::add_item_message(local_player_index, source, source_kind, count);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::display_checkpoint_message.
 * blam-cc: DL -> is_begin
 *
 * @address 0x4aa310
 */
extern "C" void hud_display_checkpoint_message(uint8_t is_begin)
{
    halo::interface::HudMessaging::display_checkpoint_message(is_begin);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::display_loading_message.
 * blam-cc: AL -> is_begin
 *
 * @address 0x4aa2a0
 */
extern "C" void hud_display_loading_message(uint8_t is_begin)
{
    halo::interface::HudMessaging::display_loading_message(is_begin);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::get_message_string.
 * blam-cc: message_index -> EDX
 *
 * @address 0x4aa3f0
 */
extern "C" uint16_t * hud_get_message_string(int32_t message_index)
{
    return halo::interface::HudMessaging::get_message_string(message_index);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::message_broadcast_to_local_players.
 *
 * @address 0x495f50
 */
extern "C" void hud_message_broadcast_to_local_players(const uint16_t *text)
{
    halo::interface::HudMessaging::message_broadcast_to_local_players(text);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::message_compare.
 *
 * @address 0x4ae500
 */
extern "C" int32_t hud_message_compare(const void *a, const void *b)
{
    return halo::interface::HudMessaging::message_compare(a, b);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::message_find_slot.
 * blam-cc: source -> ESI
 *
 * @address 0x4ae480
 */
extern "C" hud_message_slot * hud_message_find_slot(int32_t source, hud_player_messaging_state *record, uint8_t source_kind)
{
    return halo::interface::HudMessaging::message_find_slot(source, record, source_kind);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::messaging_clear_after_load.
 *
 * @address 0x4ae530
 */
extern "C" void hud_messaging_clear_after_load(void)
{
    halo::interface::HudMessaging::messaging_clear_after_load();
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::messaging_update.
 * blam-cc: local_player_index -> AX
 *
 * @address 0x4ae550
 */
extern "C" void hud_messaging_update(int16_t local_player_index)
{
    halo::interface::HudMessaging::messaging_update(local_player_index);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::play_pickup_notification.
 * blam-cc: ESI -> object_index, ECX -> position, EAX -> forward, stack -> sound, marker, gain, flag
 *
 * @address 0x492990
 */
extern "C" void hud_play_pickup_notification(uint32_t object_or_slot_index, int16_t item_type_code)
{
    halo::interface::HudMessaging::play_pickup_notification(object_or_slot_index, item_type_code);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::post_item_message.
 * blam-cc: count -> EAX, source -> ECX, kind -> DL
 *
 * @address 0x4ae350
 */
extern "C" void hud_post_item_message(int16_t count, int32_t source, uint8_t kind, int16_t local_player_index, int8_t machine_id)
{
    halo::interface::HudMessaging::post_item_message(count, source, kind, local_player_index, machine_id);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::receive_item_message.
 * blam-cc: message -> EAX
 *
 * @address 0x4ae200
 */
extern "C" void hud_receive_item_message(void **message)
{
    halo::interface::HudMessaging::receive_item_message(message);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::set_action_text_shown.
 * blam-cc: local_player_index -> EAX, shown -> BL
 *
 * @address 0x4ae110
 */
extern "C" void hud_set_action_text_shown(int16_t local_player_index, uint8_t shown)
{
    halo::interface::HudMessaging::set_action_text_shown(local_player_index, shown);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::set_help_text.
 *
 * @address 0x4adb30
 */
extern "C" void hud_set_help_text(int16_t message_index)
{
    halo::interface::HudMessaging::set_help_text(message_index);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::set_message_icon_argument.
 * blam-cc: local_player_index -> EAX, slot -> ESI
 *
 * @address 0x4ae050
 */
extern "C" void hud_set_message_icon_argument(int16_t local_player_index, int16_t slot, const hud_messaging_information *information)
{
    halo::interface::HudMessaging::set_message_icon_argument(local_player_index, slot, information);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::set_message_string_argument.
 * blam-cc: local_player_index -> EAX, slot -> ESI
 *
 * @address 0x4ae0b0
 */
extern "C" void hud_set_message_string_argument(int16_t local_player_index, int16_t slot, int16_t string_index, uint8_t from_scenario_names)
{
    halo::interface::HudMessaging::set_message_string_argument(local_player_index, slot, string_index, from_scenario_names);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::set_objective_text.
 *
 * @address 0x4adb80
 */
extern "C" void hud_set_objective_text(int16_t message_index)
{
    halo::interface::HudMessaging::set_objective_text(message_index);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMessaging::set_player_message.
 * blam-cc: message_index -> EAX
 *
 * @address 0x4adfc0
 */
extern "C" void hud_set_player_message(int16_t message_index, int16_t local_player_index)
{
    halo::interface::HudMessaging::set_player_message(message_index, local_player_index);
}
