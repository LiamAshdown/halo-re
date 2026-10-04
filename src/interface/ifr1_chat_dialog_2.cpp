#include "halo/interface/ifr1_chat_dialog.hpp"
#include "halo/interface/chat_gui.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/interface/constants.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include <stdint.h>
#include <wchar.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/main/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"

static auto &network_message_scratch = halo::link::ref<uint8_t [halo::interface::k_network_message_scratch_size]>(halo::game::vars().network_message_scratch);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &chat_local_prompt_string = halo::link::ref<const uint16_t []>(halo::ui::vars().chat_local_prompt_string);
static auto &chat_dialog_open = halo::link::ref<uint8_t>(halo::ui::vars().chat_dialog_open);
static auto &chat_scope_active = halo::link::ref<int32_t>(halo::ui::vars().chat_scope_active);
static auto &empty_string = halo::link::ref<wchar_t>(halo::game::vars().empty_string);
static auto &chat_gui_active = halo::link::ref<uint8_t>(halo::ui::vars().chat_gui_active);
#include "halo/interface/wide_text.hpp"

static void chat_relay_iterator_begin(data_iterator *iterator)
{
    iterator->data = halo::game::globals().player_data;
    iterator->next_index = 0;
    iterator->index = (datum_index)halo::k_dword_none;
    iterator->signature = (uint32_t)(uintptr_t)halo::game::globals().player_data ^ k_data_iterator_signature;
}

namespace halo::interface {

/**
 * Encodes a chat message and, for every connected machine whose player is on team_index (or every machine if
 * team_index is -1), queues the message length and payload bits into that machine's outgoing bit stream when
 * there is room (or room can be freed).
 *
 * @address 0x4aade0
 */
void ChatDialog::queue_team_message(int32_t team_index)
{
    uint8_t formatted[128];
    void *fields = formatted;
    int32_t header_size = 4;
    uint8_t terminator = 0xff;
    int32_t encoded_bits;
    data_iterator iterator;
    player *entry;

    (void)header_size;
    (void)terminator;

    halo::text::string_format_wide_va(reinterpret_cast<uint16_t *>(formatted), chat_local_prompt_string);

    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::interface::k_network_message_scratch_size, 0, 0xf, 0, &fields, 0, 1, 0);
    if (encoded_bits <= 0) {
        return;
    }

    iterator.data = halo::game::globals().player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    entry = (player *)halo::memory::data_iterator_next(&iterator);

    while (entry != 0) {
        if (team_index == -1 || entry->team_index_desired == team_index) {
            if ((int8_t)entry->machine_index != -1) {
                int32_t machine_index = (int8_t)entry->machine_index;
                int32_t i;
                network_machine *machines = halo::networking::globals().server->machines;
                for (i = 0; i < 0x10; i = i + 1) {
                    if (machines[i].machine_id == machine_index) {
                        network_channel *session = machines[i].channel;
                        if (session != 0) {
                            queue_on_channel(session, encoded_bits);
                        }
                        break;
                    }
                }
            }
            if (team_index != -1) {
                return;
            }
        }
        entry = (player *)halo::memory::data_iterator_next(&iterator);
    }
}

/**
 *
 * @address 0x4aabd0
 */
void ChatDialog::server_relay_incoming_message(void **context, void *machine)
{
    chat_relay_message message;
    void *item;
    uint32_t zero_24;
    uint8_t text[halo::interface::k_long_text_chars];
    int32_t sender;
    int32_t bits;
    data_iterator iterator;
    player *entry;

    if (*(int32_t *)context[0] != 0) {
        halo::networking::message_delta_decode_compound_field_staged(context);
        return;
    }
    message.scope = 0;
    message.sender = 0xff;
    message.text = text;
    if (halo::networking::message_delta_decode_compound_field(context, &message) == 0) {
        return;
    }
    sender = halo::networking::network_object_owner_team_index_desired((object *)machine);
    if (sender == -1) {
        return;
    }
    message.sender = (uint8_t)sender;
    item = &message;
    zero_24 = 0;
    (void)zero_24;
    bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::interface::k_network_message_scratch_size, 0, 0xf, 0, &item, 0, 1, 0);
    if (message.scope == 0) {
        halo::networking::network_session_broadcast_to_flagged(bits, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 1, 3);
    } else if (message.scope == 1) {
        uint8_t *sender_player = (uint8_t *)halo::memory::datum_get((datum_index)message.sender, halo::game::globals().player_data);

        if (sender_player == 0) {
            return;
        }
        chat_relay_iterator_begin(&iterator);
        while ((entry = (player *)halo::memory::data_iterator_next(&iterator)) != 0) {
            if (entry->team == ((struct player *)sender_player)->team && (int8_t)entry->machine_index != -1) {
                halo::networking::network_session_send_to_machine((int8_t)entry->machine_index, halo::networking::globals().server, 1, network_message_scratch,
                                                (uint32_t)bits, 1, 0, 1, 3);
            }
        }
    } else if (message.scope == 2) {
        datum_index vehicle = halo::interface::player_get_vehicle((datum_index)message.sender);

        if (vehicle == (datum_index)halo::k_dword_none) {
            return;
        }
        chat_relay_iterator_begin(&iterator);
        while ((entry = (player *)halo::memory::data_iterator_next(&iterator)) != 0) {
            uint8_t *unit = (uint8_t *)halo::objects::object_try_and_get(entry->unit, 3);

            if (unit != 0 && ((unit_object *)unit)->base.parent_object == vehicle && (int8_t)entry->machine_index != -1) {
                halo::networking::network_session_send_to_machine((int8_t)entry->machine_index, halo::networking::globals().server, 1, network_message_scratch,
                                                (uint32_t)bits, 1, 0, 1, 3);
            }
        }
    }
}

/**
 * Opens the multiplayer chat input dialog for the requested scope: 0 = all, 1 = team (falls back to "all" if
 * teams are disabled), 2 = vehicle (falls back to "team", then "all"), populating its prompt text from the
 * ui\multiplayer_game_text tag when available.
 *
 * @address 0x4aa700
 */
void ChatDialog::open(int32_t chat_scope)
{
    const void *prompt_text;
    if (chat_dialog_open != 0 || halo::main::globals().console_globals.active != 0) {
        return;
    }

    chat_scope_active = -1;

    if (chat_scope < 0 || chat_scope > 2) {
        return;
    }

    auto scope_prompt = [](int32_t string_index) -> const void * {
        datum_index tag_id = halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);
        return (tag_id == k_datum_index_none) ? (const void *)&empty_string
                                           : (const void *)halo::text::text_string_list_get_string(tag_id, string_index);
    };

    if (chat_scope != 0 && !halo::game::game_engine_get_teams_enabled()) {
        chat_scope = 0;
    }

    if (chat_scope == 0) {
        prompt_text = scope_prompt(0xb8);
        chat_scope_active = 0;
    } else if (chat_scope == 2 &&
               halo::interface::player_get_vehicle((datum_index)halo::interface::chat_default_team_channel()) != -1) {
        prompt_text = scope_prompt(0xba);
        chat_scope_active = 2;
    } else {
        chat_scope_active = 1;
        prompt_text = scope_prompt(0xb9);
    }

    chat_gui_active = 1;
    ChatGui::get().open_edit(static_cast<const wchar_t *>(prompt_text));
    chat_dialog_open = 1;
    halo::input::DirectInput::keyboard_set_capture_mode(1);
}

}
