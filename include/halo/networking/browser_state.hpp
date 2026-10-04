#pragma once

#include <cstdint>
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/core/shared_links.hpp"

inline auto &scroll_arrow_flash = halo::link::ref<int32_t>(halo::networking::vars().scroll_arrow_flash);
inline auto &refresh_in_flight = halo::link::ref<uint8_t>(halo::networking::vars().refresh_in_flight);
inline auto &ticker_message = halo::link::ref<wchar_t [0x100]>(halo::networking::vars().ticker_message);
inline auto &ticker_message_terminator = halo::link::ref<uint16_t>(halo::networking::vars().ticker_message_terminator);
inline auto &motd_download_state = halo::link::ref<int32_t>(halo::networking::vars().motd_download_state);
inline auto &tick_reset_flag = halo::link::ref<int32_t>(halo::networking::vars().tick_reset_flag);
inline auto &next_auto_refresh_ms = halo::link::ref<int32_t>(halo::networking::vars().next_auto_refresh_ms);
inline auto &DAT_00695424 = halo::link::ref<uint8_t [10]>(halo::networking::vars().DAT_00695424);

namespace halo::networking::browser_state {

/**
 * Server-browser scroll arrow highlight counter: set to +0x10 or -0x10 when the list is scrolled down or up and
 * decayed towards zero by 4 on every browser tick; its sign selects which arrow is drawn highlighted.
 *
 * @address 0x00719484
 */
inline int32_t &scroll_arrow_flash = ::scroll_arrow_flash;

/**
 * Non-zero while a server-list refresh is in flight (set when a refresh is requested or auxiliary updates are
 * pending, cleared when the query engine goes idle).
 *
 * @address 0x00719488
 */
inline uint8_t &refresh_in_flight = ::refresh_in_flight;

/**
 * Wide-character message of the join-game ticker: the downloaded message of the day or the stock ticker label.
 *
 * @address 0x00719498
 */
inline wchar_t (&ticker_message)[0x100] = ::ticker_message;

/**
 * Zero terminator word that follows the ticker message copy of 0xff characters.
 *
 * @address 0x00719696
 */
inline uint16_t &ticker_message_terminator = ::ticker_message_terminator;

/**
 * State of the message-of-the-day download: 0 none, 1 downloading, 2 failed or cancelled, 3 finished.
 *
 * @address 0x00719698
 */
inline int32_t &motd_download_state = ::motd_download_state;

/**
 * Cleared on every browser tick while a query engine exists; never read.
 *
 * @address 0x007196a0
 */
inline int32_t &tick_reset_flag = ::tick_reset_flag;

/**
 * Time in milliseconds at which the server list is next refreshed automatically.
 *
 * @address 0x006953fc
 */
inline int32_t &next_auto_refresh_ms = ::next_auto_refresh_ms;

/**
 * Index of the autopatch download slot carrying the message-of-the-day request, or -1.
 *
 * @address 0x00695420
 */
inline int32_t &motd_download_slot = ::motd_download_slot;

/**
 * Query-engine key ids requested from the master server when the list is refreshed.
 *
 * @address 0x00695424
 */
inline auto &master_query_key_ids = halo::link::ref<uint8_t [10]>(halo::networking::vars().DAT_00695424);

}
