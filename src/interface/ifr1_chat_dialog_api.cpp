#include "halo/interface/ifr1_chat_dialog.hpp"
#include "halo/interface/api.hpp"

namespace halo::interface {

/**
 * C ABI entry point; forwards to halo::interface::ChatDialog::close.
 *
 * @address 0x4aa900
 */
void chat_close(void)
{
    halo::interface::ChatDialog::close();
}

/**
 * C ABI entry point; forwards to halo::interface::ChatDialog::default_team_channel.
 *
 * @address 0x4ab1e0
 */
int32_t chat_default_team_channel(void)
{
    return halo::interface::ChatDialog::default_team_channel();
}

/**
 * C ABI entry point; forwards to halo::interface::ChatDialog::dispatch_incoming.
 * blam-cc: event -> EAX
 *
 * @address 0x4aaf70
 */
void chat_dispatch_incoming(void *event)
{
    halo::interface::ChatDialog::dispatch_incoming(event);
}

/**
 * C ABI entry point; forwards to halo::interface::ChatDialog::poll_hotkeys.
 *
 * @address 0x4aaa90
 */
uint8_t chat_poll_hotkeys(void)
{
    return halo::interface::ChatDialog::poll_hotkeys();
}

/**
 * C ABI entry point; forwards to halo::interface::ChatDialog::queue_team_message.
 *
 * @address 0x4aade0
 */
void chat_queue_team_message(int32_t team_index)
{
    halo::interface::ChatDialog::queue_team_message(team_index);
}

/**
 * C ABI entry point; forwards to halo::interface::ChatDialog::server_relay_incoming_message.
 *
 * @address 0x4aabd0
 */
void chat_server_relay_incoming_message(void **context, void *machine)
{
    halo::interface::ChatDialog::server_relay_incoming_message(context, machine);
}

/**
 * C ABI entry point; forwards to halo::interface::ChatDialog::submit_input.
 *
 * @address 0x4aa9b0
 */
void chat_submit_input(void)
{
    halo::interface::ChatDialog::submit_input();
}

/**
 * C ABI entry point; forwards to halo::interface::ChatDialog::open.
 *
 * @address 0x4aa700
 */
void chimera__chat_open(int32_t chat_scope)
{
    halo::interface::ChatDialog::open(chat_scope);
}

/**
 * C ABI entry point; forwards to halo::interface::ChatDialog::out.
 *
 * @address 0x4aab00
 */
void chimera__chat_out(int32_t scope, const wchar_t *text, uint8_t channel)
{
    halo::interface::ChatDialog::out(scope, text, channel);
}

}
