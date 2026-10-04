#include "halo/interface/ifr1_hud_chat_listbox.hpp"
#include "halo/interface/chat_gui.hpp"
#include <wchar.h>
#include <string.h>
#include "halo/cseries/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/interface/wide_text.hpp"

static auto &hud_chat_message_count = halo::link::ref<int32_t>(halo::ui::vars().hud_chat_message_count);
static auto &hud_chat_message_expiry = halo::link::ref<int32_t [8]>(halo::ui::vars().hud_chat_message_expiry);
static auto &hud_chat_listbox_visible = halo::link::ref<uint8_t>(halo::ui::vars().hud_chat_listbox_visible);
static auto &game_engine_state_value = halo::link::ref<int32_t>(halo::game::vars().game_engine_state_value);
static auto &game_engine_nameplate_fade_opacity_array = halo::link::ref<float>(halo::game::vars().game_engine_nameplate_fade_opacity_array);

namespace halo::interface {

/**
 * Removes every row from the chat listbox GUI control (if one exists) and resets the module's own message-
 * count and expiry-timestamp bookkeeping.
 *
 * @address 0x4ab400
 */
void HudChatListbox::clear(void)
{
    ChatGui::get().clear_lines();

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

    if (hud_chat_message_count > 0) {
        ChatGui::get().remove_oldest_line();
        result = 1;
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
    if (hud_chat_message_count <= 0) {
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
            ChatGui::get().set_log_visible(true);
            hud_chat_listbox_visible = 1;
        }
    } else if (game_engine_state_value == 0 && game_engine_nameplate_fade_opacity_array != 0.0f) {
        ChatGui::get().set_log_visible(false);
        hud_chat_listbox_visible = 0;
    }
}

}
