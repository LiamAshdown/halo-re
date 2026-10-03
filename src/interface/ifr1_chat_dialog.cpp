#include "halo/interface/ifr1_chat_dialog.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/main/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/records.hpp"
#include "halo/interface/constants.hpp"
#include "halo/interface/wide_text.hpp"
#include "halo/interface/com_object.hpp"

extern "C" {
extern uint8_t chat_dialog_open;
extern int32_t chat_scope_active;
extern uint8_t controls_input_capture_flags;
extern void **keyboard_device;
extern uint8_t key_frames[0x6d];
extern uint8_t key_release_pending[0x6d];
extern void *chat_gui_root_handle;
extern chat_gui_find_object_fn chat_gui_find_object;
extern void *chat_gui_find_object_arg;
extern chat_gui_set_focus_fn chat_gui_set_focus;
extern chat_gui_set_state_fn chat_gui_set_state;
extern chat_gui_release_fn chat_gui_release;
extern uint8_t chat_gui_active;
extern wchar_t empty_string;
extern int32_t shell_load_localized_string(int32_t id, char *out_buffer);
extern uint8_t chat_hotkey_all;
extern uint8_t chat_hotkey_team;
extern uint8_t chat_hotkey_vehicle;
extern chat_gui_find_child_fn chat_gui_find_child;
extern chat_gui_get_property_string_fn keystone_control_get_attribute;
extern uint8_t network_message_scratch[halo::interface::k_network_message_scratch_size];
}

static const wchar_t *chat_prefix_format(int16_t string_index)
{
    datum_index tag = halo::interface::lookup_tag(halo::groups::unicode_string_list, "ui\\multiplayer_game_text");
    if (tag == (datum_index)-1) {
        return &empty_string;
    }
    return reinterpret_cast<const wchar_t *>(halo::text::text_string_list_get_string(tag, string_index));
}

namespace halo::interface {

/**
 * Closes the multiplayer chat input dialog: clears the chat-open state, resets the DirectInput keyboard device
 * the same way virtual_keyboard_close does, and releases the GUI dialog object.
 *
 * @address 0x4aa900
 */
void ChatDialog::close(void)
{
    void *gui_object;

    if (chat_dialog_open == 0) {
        return;
    }

    controls_input_capture_flags &= 0xfb;
    chat_scope_active = -1;
    chat_dialog_open = 0;

    if (keyboard_device != 0) {
        int32_t minus_one = -1;
        void **vtable = halo::interface::com_vtable(keyboard_device);
        ((directinput_set_property_fn)vtable[0x28 / 4])(keyboard_device, 0x14, 0, &minus_one, 0);
        memset(key_release_pending, 0, sizeof(key_release_pending));
        memset(key_frames, 0, sizeof(key_frames));
    }

    chat_gui_active = 0;
    gui_object = chat_gui_find_object(chat_gui_root_handle, chat_gui_find_object_arg);
    if (gui_object != 0) {
        chat_gui_set_focus(gui_object, 0);
        chat_gui_set_state(gui_object, 0);
        chat_gui_release(gui_object);
    }
}

/**
 * Scans player_data for the first locally-driven player and returns its desired team index, or -1 if none is
 * found.
 *
 * @address 0x4ab1e0
 */
int32_t ChatDialog::default_team_channel(void)
{
    data_iterator iterator;
    player *entry;

    iterator.data = halo::game::globals().player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)-1;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    entry = (player *)halo::memory::data_iterator_next(&iterator);
    while (entry != 0) {
        if (entry->local_player_index != -1) {
            return entry->team_index_desired;
        }
        entry = (player *)halo::memory::data_iterator_next(&iterator);
    }
    return -1;
}

namespace {

class PlayerChatSource final : public ChatLineSource {
public:
    bool accepts(const chat_incoming_record &record) const override;
    void deliver(const chat_incoming_record &record, wchar_t *text) const override;
};

class LocalizedChatSource final : public ChatLineSource {
public:
    bool accepts(const chat_incoming_record &record) const override;
    void deliver(const chat_incoming_record &record, wchar_t *text) const override;
};

class PlainChatSource final : public ChatLineSource {
public:
    bool accepts(const chat_incoming_record &record) const override;
    void deliver(const chat_incoming_record &record, wchar_t *text) const override;
};

bool PlayerChatSource::accepts(const chat_incoming_record &record) const
{
    return record.player_index != 0xff;
}

void PlayerChatSource::deliver(const chat_incoming_record &record, wchar_t *text) const
{
    wchar_t line[halo::interface::k_long_text_chars];
    player *sender = (player *)halo::memory::datum_get((datum_index)record.player_index, halo::game::globals().player_data);

    if (sender == 0) {
        return;
    }
    memset(line, 0, sizeof(line));
    if (record.kind == 0) {
        halo::text::string_format_wide_va(reinterpret_cast<uint16_t *>(line), reinterpret_cast<const uint16_t *>(chat_prefix_format(0xbb)), sender->name);
        wcslen(line);
    } else if (record.kind > 0 && record.kind <= 2) {
        halo::text::string_format_wide_va(reinterpret_cast<uint16_t *>(line), reinterpret_cast<const uint16_t *>(chat_prefix_format(0xbc)), sender->name);
        wcslen(line);
    }
    wcscat(line, text);
    halo::interface::chimera__multiplayer_message(line);
}

bool LocalizedChatSource::accepts(const chat_incoming_record &record) const
{
    return record.kind == 4;
}

void LocalizedChatSource::deliver(const chat_incoming_record &record, wchar_t *) const
{
    wchar_t short_line[0x80];
    char localized[halo::interface::k_hint_text_chars];
    int32_t string_id = (int32_t)_wtol((const wchar_t *)record.text);

    memset(short_line, 0, sizeof(short_line));
    if (halo::shell::shell_load_localized_string(sizeof(localized), halo::shell::globals().module_handle, localized, string_id) == 0) {
        return;
    }
    halo::text::string_format_wide_va_bounded(0x7f, (uint16_t *)short_line, halo::interface::wide(L"%S"), localized);
    short_line[0x7f] = 0;
    halo::interface::chimera__multiplayer_message(short_line);
}

bool PlainChatSource::accepts(const chat_incoming_record &record) const
{
    return record.kind == 3;
}

void PlainChatSource::deliver(const chat_incoming_record &, wchar_t *text) const
{
    halo::interface::chimera__multiplayer_message(text);
}

const PlayerChatSource k_player_source;
const LocalizedChatSource k_localized_source;
const PlainChatSource k_plain_source;
const ChatLineSource *const k_chat_sources[] = { &k_player_source, &k_localized_source, &k_plain_source };

}

const ChatLineSource *ChatLineSource::select(const chat_incoming_record &record)
{
    for (const ChatLineSource *source : k_chat_sources) {
        if (source->accepts(record)) {
            return source;
        }
    }
    return nullptr;
}

/**
 * Decodes one incoming chat network event and appends the resulting line to the chat listbox. The decoded record
 * is routed to the first ChatLineSource that accepts it: a player message, a localized server string or plain text.
 * blam-cc: event -> EAX
 *
 * @address 0x4aaf70
 */
void ChatDialog::dispatch_incoming(void *event)
{
    chat_incoming_record record;
    wchar_t text[halo::interface::k_text_buffer_chars];
    const ChatLineSource *source;

    if (*(int32_t *)*(void **)event != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)event);
        return;
    }

    record.kind = 0;
    record.player_index = 0xff;
    record.text = (uint16_t *)text;
    if (!halo::networking::message_delta_decode_compound_field((void **)event, &record)) {
        return;
    }

    source = ChatLineSource::select(record);
    if (source != nullptr) {
        source->deliver(record, text);
    }
}

/**
 * Opens the chat dialog for whichever scope hotkey is set (all, team, vehicle, checked in that order), then
 * always refreshes the chat message listbox.
 *
 * @address 0x4aaa90
 */
uint8_t ChatDialog::poll_hotkeys(void)
{
    if (halo::main::globals().console_globals.active == 0) {
        if (chat_hotkey_all == 1) {
            halo::interface::chimera__chat_open(0);
            halo::interface::hud_chat_listbox_update();
            return chat_dialog_open;
        }
        if (chat_hotkey_team == 1) {
            halo::interface::chimera__chat_open(1);
            halo::interface::hud_chat_listbox_update();
            return chat_dialog_open;
        }
        if (chat_hotkey_vehicle == 1) {
            halo::interface::chimera__chat_open(2);
        }
    }
    halo::interface::hud_chat_listbox_update();
    return chat_dialog_open;
}

/**
 * Reads the text typed into the open chat editbox and, if the default team channel is valid and the box is
 * non-empty, sends it (truncated to 254 wide characters) via chimera__chat_out before closing the dialog.
 *
 * @address 0x4aa9b0
 */
void ChatDialog::submit_input(void)
{
    if (chat_dialog_open == 0) {
        return;
    }

    {
        int32_t team_index = halo::interface::chat_default_team_channel();
        if (team_index != -1) {
            const wchar_t *text = 0;
            void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_gui_find_object_arg);
            if (gui_object != 0) {
                void *editbox = chat_gui_find_child(gui_object, halo::interface::wide(L"oEditbox"));
                if (editbox != 0) {
                    text = (const wchar_t *)(keystone_control_get_attribute(editbox, halo::interface::wide(L"text")));
                }
                chat_gui_release(gui_object);

                if (text != 0 && *text != 0) {
                    wchar_t buffer[256];
                    uint32_t length = wcslen((const wchar_t *)((const uint16_t *)text));
                    size_t count = (length < 0xff) ? length : 0xfe;
                    wcsncpy(buffer, text, count);
                    buffer[count] = 0;
                    halo::interface::chimera__chat_out((uint8_t)team_index);
                }
            }
        }
        halo::interface::chat_close();
    }
}

/**
 * Appends the chat message staged in network_message_scratch (encoded_bits long) to the channel's outgoing stream when the
 * channel is not a listener and the stream has room for it or can be flushed to make room.
 *
 * @address 0x4aab00
 */
void ChatDialog::queue_on_channel(network_channel *channel, int32_t encoded_bits)
{
    bit_stream *stream = &channel->outgoing.stream;

    if ((channel->flags & k_network_channel_listening) == 0 &&
        (encoded_bits + 1 <= ((int32_t)stream->last_bit + (int32_t)stream->byte_cursor * -8) - (int32_t)stream->bit_cursor + 1 ||
         halo::networking::network_channel_stream_flush(&channel->outgoing, channel, 1) != 0)) {
        uint32_t item_flag = 1;

        channel->send_budget = channel->send_budget + encoded_bits + 1;
        halo::memory::bit_stream_write_bits_chunked(stream, &item_flag, 1);
        channel->outgoing.empty = 0;
        halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)(network_message_scratch), encoded_bits);
        channel->outgoing.empty = 0;
    }
}

/**
 * Encodes a chat text message (type 0xf) for the given channel and, if the session's outgoing buffer has room
 * (or can be flushed to make room), queues its length and payload bits for network transmission.
 *
 * @address 0x4aab00
 */
void ChatDialog::out(uint8_t channel)
{
    int32_t encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::interface::k_network_message_scratch_size, 0, 0xf, 0, (void **)&channel, 0, 1, 0);

    if (encoded_bits > 0) {
        queue_on_channel(halo::networking::globals().client->channel, encoded_bits);
    }
}

}
