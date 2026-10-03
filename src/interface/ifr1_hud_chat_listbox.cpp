#include "halo/interface/ifr1_hud_chat_listbox.hpp"
#include <wchar.h>
#include <string.h>
#include "halo/cseries/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/game/api.hpp"

static auto &hud_chat_message_count = halo::link::ref<int32_t>(halo::ui::vars().hud_chat_message_count);
static auto &hud_chat_message_expiry = halo::link::ref<int32_t [8]>(halo::ui::vars().hud_chat_message_expiry);
static auto &chat_gui_root_handle = halo::link::ref<void *>(halo::ui::vars().chat_gui_root_handle);
static auto &chat_gui_find_object = halo::link::ref<chat_gui_find_object_fn>(halo::ui::vars().chat_gui_find_object);
static auto &chat_listbox_gui_find_object_arg = halo::link::ref<void *>(halo::ui::vars().chat_listbox_gui_find_object_arg);
static auto &chat_gui_find_child = halo::link::ref<chat_gui_find_child_fn>(halo::ui::vars().chat_gui_find_child);
static auto &chat_gui_set_property_int = halo::link::ref<chat_gui_set_property_int_fn>(halo::ui::vars().chat_gui_set_property_int);
static auto &chat_gui_finalize = halo::link::ref<chat_gui_finalize_fn>(halo::ui::vars().chat_gui_finalize);
static auto &chat_gui_release = halo::link::ref<chat_gui_release_fn>(halo::ui::vars().chat_gui_release);
static auto &hud_chat_listbox_visible = halo::link::ref<uint8_t>(halo::ui::vars().hud_chat_listbox_visible);
static auto &game_engine_state_value = halo::link::ref<int32_t>(halo::game::vars().game_engine_state_value);
static auto &game_engine_nameplate_fade_opacity_array = halo::link::ref<float>(halo::game::vars().game_engine_nameplate_fade_opacity_array);
static auto &chat_gui_set_state = halo::link::ref<chat_gui_set_state_fn>(halo::ui::vars().chat_gui_set_state);

namespace halo::interface {

/**
 * Removes every row from the chat listbox GUI control (if one exists) and resets the module's own message-
 * count and expiry-timestamp bookkeeping.
 *
 * @address 0x4ab400
 */
void HudChatListbox::clear(void)
{
    if (chat_gui_find_object != 0) {
        void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
        if (gui_object != 0) {
            void *listbox = chat_gui_find_child(gui_object, (const uint16_t *)L"oListbox");
            if (listbox != 0) {
                while (hud_chat_message_count != 0 &&
                       (int32_t)chat_gui_set_property_int(listbox, 0x182, 0, 0) > 0) {
                    hud_chat_message_count = hud_chat_message_count - 1;
                }
                chat_gui_finalize(gui_object);
            }
            chat_gui_release(gui_object);
        }
    }

    {
        int32_t i;
        for (i = 0; i < 8; i = i + 1) {
            hud_chat_message_expiry[i] = 0;
        }
    }
    hud_chat_message_count = 0;
}

/**
 * Removes the row 0 entry from the chat listbox GUI control (if one exists) and shifts the
 * hud_chat_message_expiry timestamps down by one, returning whatever the GUI's own row-removal property call
 * returned (0 if the control could not be reached at all).
 *
 * @address 0x4ab240
 */
uint32_t HudChatListbox::remove_oldest(void)
{
    uint32_t result = 0;

    if (hud_chat_message_count > 0 && chat_gui_find_object != 0) {
        void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
        if (gui_object != 0) {
            void *listbox = chat_gui_find_child(gui_object, (const uint16_t *)L"oListbox");
            if (listbox != 0) {
                result = chat_gui_set_property_int(listbox, 0x182, 0, 0);
                chat_gui_set_property_int(listbox, 0x115, 2, 0);
                chat_gui_finalize(gui_object);
            }
            chat_gui_release(gui_object);
        }
    }

    hud_chat_message_count = hud_chat_message_count - 1;
    memmove(hud_chat_message_expiry, hud_chat_message_expiry + 1, hud_chat_message_count * 4);
    hud_chat_message_expiry[hud_chat_message_count] = 0;
    return result;
}

/**
 * Expires timed-out chat messages (whose stored expiry has passed the current time) from the front of the
 * listbox, then shows or hides the GUI listbox control depending on
 * game_engine_state_value/game_engine_nameplate_fade_opacity_array.
 *
 * @address 0x4ab300
 */
void HudChatListbox::update(void)
{
    if (hud_chat_message_count <= 0 || chat_gui_find_object == 0) {
        return;
    }

    {
        uint32_t now = (uint32_t)halo::cseries::time_query_performance_counter_ms();
        while (hud_chat_message_count > 0 && hud_chat_message_expiry[0] != 0 &&
               (uint32_t)hud_chat_message_expiry[0] <= now) {
            halo::interface::hud_chat_listbox_remove_oldest();
        }
    }

    if (hud_chat_listbox_visible == 0) {
        if (game_engine_state_value != 0 || game_engine_nameplate_fade_opacity_array == 0.0f) {
            void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
            if (gui_object != 0) {
                chat_gui_set_state(gui_object, 5);
                chat_gui_release(gui_object);
            }
            hud_chat_listbox_visible = 1;
        }
    } else if (game_engine_state_value == 0 && game_engine_nameplate_fade_opacity_array != 0.0f) {
        void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
        if (gui_object != 0) {
            chat_gui_set_state(gui_object, 0);
            chat_gui_release(gui_object);
        }
        hud_chat_listbox_visible = 0;
    }
}

}
