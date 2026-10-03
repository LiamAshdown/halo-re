#include "halo/interface/ifr1_chat_dialog.hpp"
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

static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &chat_local_prompt_string = halo::link::ref<const uint16_t []>(halo::ui::vars().chat_local_prompt_string);
extern "C" {
extern datum_index player_get_vehicle(datum_index player_index);
}
static auto &chat_dialog_open = halo::link::ref<uint8_t>(halo::ui::vars().chat_dialog_open);
static auto &chat_scope_active = halo::link::ref<int32_t>(halo::ui::vars().chat_scope_active);
static auto &empty_string = halo::link::ref<wchar_t>(halo::game::vars().empty_string);
static auto &chat_gui_root_handle = halo::link::ref<void *>(halo::ui::vars().chat_gui_root_handle);
static auto &chat_gui_find_object = halo::link::ref<chat_gui_find_object_fn>(halo::ui::vars().chat_gui_find_object);
static auto &chat_gui_find_object_arg = halo::link::ref<void *>(halo::ui::vars().chat_gui_find_object_arg);
static auto &chat_gui_find_child = halo::link::ref<chat_gui_find_child_fn>(halo::ui::vars().chat_gui_find_child);
static auto &chat_gui_set_focus = halo::link::ref<chat_gui_set_focus_fn>(halo::ui::vars().chat_gui_set_focus);
static auto &keystone_control_set_attribute = halo::link::ref<chat_gui_set_property_string_fn>(halo::ui::vars().keystone_control_set_attribute);
static auto &chat_gui_set_property_int = halo::link::ref<chat_gui_set_property_int_fn>(halo::ui::vars().chat_gui_set_property_int);
static auto &chat_gui_set_state = halo::link::ref<chat_gui_set_state_fn>(halo::ui::vars().chat_gui_set_state);
static auto &chat_gui_release = halo::link::ref<chat_gui_release_fn>(halo::ui::vars().chat_gui_release);
static auto &chat_gui_active = halo::link::ref<uint8_t>(halo::ui::vars().chat_gui_active);

typedef struct chat_relay_message {
    int32_t scope;
    uint8_t sender;
    uint8_t pad_05[3];
    void *text;
} chat_relay_message;

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

    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xf, 0, &fields, 0, 1, 0);
    if (encoded_bits <= 0) {
        return;
    }

    iterator.data = halo::game::globals().player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)-1;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    entry = (player *)halo::memory::data_iterator_next(&iterator);

    while (entry != 0) {
        if (team_index == -1 || entry->team_index_desired == team_index) {
            if (*((int8_t *)entry + 0x64) != -1) {
                int32_t machine_index = *((int8_t *)entry + 0x64);
                int32_t i;
                int16_t *slot_table = (int16_t *)((uint8_t *)halo::networking::globals().server + 0x3c4);
                for (i = 0; i < 0x10; i = i + 1) {
                    if (slot_table[i * 0x30] == machine_index) {
                        uint8_t **session_ptr = (uint8_t **)((uint8_t *)halo::networking::globals().server + 0x3b8 + i * 0x60);
                        uint8_t *session = *session_ptr;
                        if (session != 0 && (session[0xa8c] & 1) == 0 &&
                            (encoded_bits + 1 <= (*(int32_t *)(session + 0x24) +
                                                   *(int32_t *)(session + 0x1c) * -8) -
                                                      *(int32_t *)(session + 0x20) + 1 ||
                             halo::networking::network_channel_stream_flush((network_channel_stream *)((uint8_t *)session + 0x10), (network_channel *)session, 1) != 0)) {
                            *(int32_t *)(session + 0xa80) = *(int32_t *)(session + 0xa80) + encoded_bits + 1;
                            { uint32_t item_flag = 1; halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)session + 0x10), &item_flag, 1); }
                            session[0x2c] = 0;
                            halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)session + 0x10), (const uint32_t *)(network_message_scratch), encoded_bits);
                            session[0x2c] = 0;
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
 * Original engine function chat_server_relay_incoming_message; the author notes are in
 * docs/original/interface/chat_server_relay_incoming_message.txt.
 *
 * @address 0x4aabd0
 */
void ChatDialog::server_relay_incoming_message(void **context, void *machine)
{
    chat_relay_message message;
    void *item;
    uint32_t zero_24;
    uint8_t text[0x200];
    int32_t sender;
    int32_t bits;
    data_iterator iterator;
    uint8_t *entry;

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
    bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xf, 0, &item, 0, 1, 0);
    if (message.scope == 0) {
        halo::networking::network_session_broadcast_to_flagged(bits, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 1, 3);
    } else if (message.scope == 1) {
        uint8_t *sender_player = (uint8_t *)halo::memory::datum_get((datum_index)message.sender, halo::game::globals().player_data);

        if (sender_player == 0) {
            return;
        }
        chat_relay_iterator_begin(&iterator);
        while ((entry = (uint8_t *)halo::memory::data_iterator_next(&iterator)) != 0) {
            if (*(int32_t *)(entry + 0x20) == ((struct player *)sender_player)->team && *(int8_t *)(entry + 0x64) != -1) {
                halo::networking::network_session_send_to_machine(*(int8_t *)(entry + 0x64), halo::networking::globals().server, 1, network_message_scratch,
                                                (uint32_t)bits, 1, 0, 1, 3);
            }
        }
    } else if (message.scope == 2) {
        datum_index vehicle = halo::interface::player_get_vehicle((datum_index)message.sender);

        if (vehicle == (datum_index)halo::k_dword_none) {
            return;
        }
        chat_relay_iterator_begin(&iterator);
        while ((entry = (uint8_t *)halo::memory::data_iterator_next(&iterator)) != 0) {
            uint8_t *unit = (uint8_t *)halo::objects::object_try_and_get(*(datum_index *)(entry + 0x34), 3);

            if (unit != 0 && ((unit_object *)unit)->base.parent_object == vehicle && *(int8_t *)(entry + 0x64) != -1) {
                halo::networking::network_session_send_to_machine(*(int8_t *)(entry + 0x64), halo::networking::globals().server, 1, network_message_scratch,
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
    void *gui_object;
    void *child;

    if (chat_dialog_open != 0 || halo::main::globals().console_globals.active != 0 || chat_gui_find_object == 0) {
        return;
    }

    chat_scope_active = -1;

    if (chat_scope == 0) {
all_scope:
        {
            datum_index tag_id = halo::interface::lookup_tag(halo::groups::unicode_string_list, "ui\\multiplayer_game_text");
            prompt_text = (tag_id == (datum_index)-1) ? (const void *)&empty_string
                                                       : (const void *)halo::text::text_string_list_get_string(tag_id, 0xb8);
            chat_scope_active = 0;
        }
    } else if (chat_scope == 1) {
        if (!halo::game::game_engine_get_teams_enabled()) {
            goto all_scope;
        }
        goto team_scope;
    } else if (chat_scope == 2) {
        if (!halo::game::game_engine_get_teams_enabled()) {
            goto all_scope;
        }
        {
            int32_t unit_index = halo::interface::chat_default_team_channel();
            int32_t player_index = halo::interface::player_get_vehicle((datum_index)unit_index);
            if (player_index != -1) {
                datum_index tag_id = halo::interface::lookup_tag(halo::groups::unicode_string_list, "ui\\multiplayer_game_text");
                prompt_text = (tag_id == (datum_index)-1) ? (const void *)&empty_string
                                                           : (const void *)halo::text::text_string_list_get_string(tag_id, 0xba);
                chat_scope_active = 2;
                goto gui_setup;
            }
        }
team_scope:
        {
            datum_index tag_id;
            chat_scope_active = 1;
            tag_id = halo::interface::lookup_tag(halo::groups::unicode_string_list, "ui\\multiplayer_game_text");
            prompt_text = (tag_id == (datum_index)-1) ? (const void *)&empty_string
                                                       : (const void *)halo::text::text_string_list_get_string(tag_id, 0xb9);
            if (chat_scope_active == -1) {
                return;
            }
        }
    } else {
        chat_scope_active = -1;
        return;
    }

gui_setup:
    chat_gui_active = 1;
    gui_object = chat_gui_find_object(chat_gui_root_handle, chat_gui_find_object_arg);
    if (gui_object != 0) {
        child = chat_gui_find_child(gui_object, (const uint16_t *)L"oPrompt");
        if (child != 0) {
            keystone_control_set_attribute(child, (const uint16_t *)L"text", prompt_text);
        }
        child = chat_gui_find_child(gui_object, (const uint16_t *)L"oEditbox");
        if (child != 0) {
            int32_t zero[2] = {0, 0};
            chat_gui_set_focus(gui_object, child);
            keystone_control_set_attribute(child, (const uint16_t *)L"text", &empty_string);
            chat_gui_set_property_int(child, 0x201, 0, zero);
        }
        chat_gui_set_state(gui_object, 5);
        chat_gui_release(gui_object);
    }
    chat_dialog_open = 1;
    halo::input::DirectInput::keyboard_set_capture_mode(1);
}

}
