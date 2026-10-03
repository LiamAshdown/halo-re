#include "halo/interface/ifr1_chat_dialog.hpp"
#include <string.h>
#include <stdint.h>
#include <wchar.h>

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
extern data_array *player_data;
extern void *data_iterator_next(data_iterator *iterator);
extern void *shell_module_handle;
extern wchar_t empty_string;
extern uint8_t message_delta_decode_compound_field(void *event, chat_incoming_record *out_record);
extern void message_delta_decode_compound_field_staged(void *event);
extern void *datum_get(datum_index handle, data_array *array);
extern datum_index tag_lookup(tag_group group, char *path);
extern wchar_t *text_string_list_get_string(datum_index tag, int16_t index);
extern wchar_t *string_format_wide_va(wchar_t *dest, const wchar_t *format, ...);
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...);
extern int32_t shell_load_localized_string(int32_t id, char *out_buffer);
extern void chimera__multiplayer_message(const wchar_t *text);
extern console_globals console_globals_data;
extern uint8_t chat_hotkey_all;
extern uint8_t chat_hotkey_team;
extern uint8_t chat_hotkey_vehicle;
extern void chimera__chat_open(int32_t chat_scope);
extern void hud_chat_listbox_update(void);
extern chat_gui_find_child_fn chat_gui_find_child;
extern chat_gui_get_property_string_fn keystone_control_get_attribute;
extern int32_t chat_default_team_channel(void);
extern void chimera__chat_out(uint8_t team_index);
extern void chat_close(void);
extern uint8_t network_message_scratch[0x7ff8];
extern network_client_globals *network_client;
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode);
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count);
}

static const wchar_t *chat_prefix_format(int16_t string_index)
{
    datum_index tag = tag_lookup(0x75737472 , (char *)"ui\\multiplayer_game_text");
    if (tag == (datum_index)-1) {
        return &empty_string;
    }
    return text_string_list_get_string(tag, string_index);
}

namespace halo::interface {

/**
 * 0x00721eec Closes the multiplayer chat input dialog: clears the chat-open state, resets the DirectInput
 * keyboard device the same way virtual_keyboard_close does, and releases the GUI dialog object.
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
        void **vtable = *(void ***)keyboard_device;
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
 * 0x4d05d0 Scans player_data for the first locally-driven player and returns its desired team index, or -1 if
 * none is found.
 *
 * @address 0x4ab1e0
 */
int32_t ChatDialog::default_team_channel(void)
{
    data_iterator iterator;
    player *entry;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)-1;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    entry = (player *)data_iterator_next(&iterator);
    while (entry != 0) {
        if (entry->local_player_index != -1) {
            return entry->team_index_desired;
        }
        entry = (player *)data_iterator_next(&iterator);
    }
    return -1;
}

/**
 * Decodes one incoming chat network event and appends the resulting line to the chat listbox.
 * blam-cc: event -> EAX
 *
 * @address 0x4aaf70
 */
void ChatDialog::dispatch_incoming(void *event)
{
    chat_incoming_record record;
    wchar_t short_line[0x80];
    wchar_t line[0x200];
    wchar_t text[0x100];

    if (*(int32_t *)*(void **)event != 0) {
        message_delta_decode_compound_field_staged(event);
        return;
    }

    record.kind = 0;
    record.player_index = 0xff;
    record.text = (uint16_t *)text;
    if (!message_delta_decode_compound_field(event, &record)) {
        return;
    }

    if (record.player_index != 0xff) {
        player *sender = (player *)datum_get((datum_index)record.player_index, player_data);
        if (sender == 0) {
            return;
        }
        memset(line, 0, sizeof(line));
        if (record.kind == 0) {
            string_format_wide_va(line, chat_prefix_format(0xbb), sender->name);
            wcslen(line);
        } else if (record.kind > 0 && record.kind <= 2) {
            string_format_wide_va(line, chat_prefix_format(0xbc), sender->name);
            wcslen(line);
        }
        wcscat(line, text);
        chimera__multiplayer_message(line);
        return;
    }

    if (record.kind == 4) {
        char localized[0x400];
        int32_t string_id = (int32_t)_wtol((const wchar_t *)record.text);
        memset(short_line, 0, sizeof(short_line));
        if (shell_load_localized_string(string_id, localized) == 0) {
            return;
        }
        string_format_wide_va_bounded(0x7f, (uint16_t *)short_line, (const uint16_t *)L"%S", localized);
        short_line[0x7f] = 0;
        chimera__multiplayer_message(short_line);
        return;
    }

    if (record.kind == 3) {
        chimera__multiplayer_message(text);
    }
}

/**
 * 0x4ab300, the caller reloads AL from 0x006b3858 afterwards Opens the chat dialog for whichever scope hotkey
 * is set (all, team, vehicle, checked in that order), then always refreshes the chat message listbox.
 *
 * @address 0x4aaa90
 */
uint8_t ChatDialog::poll_hotkeys(void)
{
    if (console_globals_data.active == 0) {
        if (chat_hotkey_all == 1) {
            chimera__chat_open(0);
            hud_chat_listbox_update();
            return chat_dialog_open;
        }
        if (chat_hotkey_team == 1) {
            chimera__chat_open(1);
            hud_chat_listbox_update();
            return chat_dialog_open;
        }
        if (chat_hotkey_vehicle == 1) {
            chimera__chat_open(2);
        }
    }
    hud_chat_listbox_update();
    return chat_dialog_open;
}

/**
 * 0x4aa900 Reads the text typed into the open chat editbox and, if the default team channel is valid and the
 * box is non-empty, sends it (truncated to 254 wide characters) via chimera__chat_out before closing the
 * dialog.
 *
 * @address 0x4aa9b0
 */
void ChatDialog::submit_input(void)
{
    if (chat_dialog_open == 0) {
        return;
    }

    {
        int32_t team_index = chat_default_team_channel();
        if (team_index != -1) {
            const wchar_t *text = 0;
            void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_gui_find_object_arg);
            if (gui_object != 0) {
                void *editbox = chat_gui_find_child(gui_object, (const uint16_t *)L"oEditbox");
                if (editbox != 0) {
                    text = (const wchar_t *)(keystone_control_get_attribute(editbox, (const uint16_t *)L"text"));
                }
                chat_gui_release(gui_object);

                if (text != 0 && *text != 0) {
                    wchar_t buffer[256];
                    uint32_t length = wcslen((const wchar_t *)((const uint16_t *)text));
                    size_t count = (length < 0xff) ? length : 0xfe;
                    wcsncpy(buffer, text, count);
                    buffer[count] = 0;
                    chimera__chat_out((uint8_t)team_index);
                }
            }
        }
        chat_close();
    }
}

/**
 * 0x4cf8f0, EAX stream, ECX values, stack bits Encodes a chat text message (type 0xf) for the given channel
 * and, if the session's outgoing buffer has room (or can be flushed to make room), queues its length and
 * payload bits for network transmission.
 *
 * @address 0x4aab00
 */
void ChatDialog::out(uint8_t channel)
{
    int32_t encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xf, 0, (void **)&channel, 0, 1, 0);

    if (encoded_bits > 0) {
        uint8_t *session = *(uint8_t **)((uint8_t *)network_client + 0xadc);

        if ((session[0xa8c] & 1) == 0 &&
            (encoded_bits + 1 <= (*(int32_t *)(session + 0x24) + *(int32_t *)(session + 0x1c) * -8) -
                                      *(int32_t *)(session + 0x20) + 1 ||
             network_channel_stream_flush((network_channel_stream *)((uint8_t *)session + 0x10), (network_channel *)session, 1) != 0)) {
            *(int32_t *)(session + 0xa80) = *(int32_t *)(session + 0xa80) + encoded_bits + 1;
            { uint32_t item_flag = 1; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)session + 0x10), &item_flag, 1); }
            session[0x2c] = 0;
            bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)session + 0x10), (const uint32_t *)(network_message_scratch), encoded_bits);
            session[0x2c] = 0;
        }
    }
}

}
