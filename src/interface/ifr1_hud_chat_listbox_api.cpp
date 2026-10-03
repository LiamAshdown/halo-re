#include "halo/interface/ifr1_hud_chat_listbox.hpp"
#include "halo/interface/api.hpp"

namespace halo::interface {

/**
 * C ABI entry point; forwards to halo::interface::HudChatListbox::clear.
 *
 * @address 0x4ab400
 */
void hud_chat_listbox_clear(void)
{
    halo::interface::HudChatListbox::clear();
}

/**
 * C ABI entry point; forwards to halo::interface::HudChatListbox::remove_oldest.
 *
 * @address 0x4ab240
 */
uint32_t hud_chat_listbox_remove_oldest(void)
{
    return halo::interface::HudChatListbox::remove_oldest();
}

/**
 * C ABI entry point; forwards to halo::interface::HudChatListbox::update.
 *
 * @address 0x4ab300
 */
void hud_chat_listbox_update(void)
{
    halo::interface::HudChatListbox::update();
}

}
