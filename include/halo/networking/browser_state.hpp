#pragma once

#include <cstdint>
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/core/shared_links.hpp"

inline auto &DAT_00719484 = halo::link::ref<int32_t>(halo::networking::vars().DAT_00719484);
inline auto &DAT_00719488 = halo::link::ref<uint8_t>(halo::networking::vars().DAT_00719488);
inline auto &DAT_00719498 = halo::link::ref<wchar_t [0x100]>(halo::networking::vars().DAT_00719498);
inline auto &DAT_00719696 = halo::link::ref<uint16_t>(halo::networking::vars().DAT_00719696);
inline auto &DAT_00719698 = halo::link::ref<int32_t>(halo::networking::vars().DAT_00719698);
inline auto &DAT_007196a0 = halo::link::ref<int32_t>(halo::networking::vars().DAT_007196a0);
inline auto &DAT_006953fc = halo::link::ref<int32_t>(halo::networking::vars().DAT_006953fc);
inline auto &DAT_00695424 = halo::link::ref<uint8_t [10]>(halo::networking::vars().DAT_00695424);

namespace halo::networking::browser_state {

/**
 * Server-browser scroll arrow highlight counter: set to +0x10 or -0x10 when the list is scrolled down or up and
 * decayed towards zero by 4 on every browser tick; its sign selects which arrow is drawn highlighted.
 *
 * @address 0x00719484
 */
inline int32_t &scroll_arrow_flash = ::DAT_00719484;

/**
 * Non-zero while a server-list refresh is in flight (set when a refresh is requested or auxiliary updates are
 * pending, cleared when the query engine goes idle).
 *
 * @address 0x00719488
 */
inline uint8_t &refresh_in_flight = ::DAT_00719488;

/**
 * Wide-character message of the join-game ticker: the downloaded message of the day or the stock ticker label.
 *
 * @address 0x00719498
 */
inline wchar_t (&ticker_message)[0x100] = ::DAT_00719498;

/**
 * Zero terminator word that follows the ticker message copy of 0xff characters.
 *
 * @address 0x00719696
 */
inline uint16_t &ticker_message_terminator = ::DAT_00719696;

/**
 * State of the message-of-the-day download: 0 none, 1 downloading, 2 failed or cancelled, 3 finished.
 *
 * @address 0x00719698
 */
inline int32_t &motd_download_state = ::DAT_00719698;

/**
 * Cleared on every browser tick while a query engine exists; never read.
 *
 * @address 0x007196a0
 */
inline int32_t &tick_reset_flag = ::DAT_007196a0;

/**
 * Time in milliseconds at which the server list is next refreshed automatically.
 *
 * @address 0x006953fc
 */
inline int32_t &next_auto_refresh_ms = ::DAT_006953fc;

/**
 * Index of the autopatch download slot carrying the message-of-the-day request, or -1.
 *
 * @address 0x00695420
 */
inline int32_t &motd_download_slot = ::DAT_00695420;

/**
 * Query-engine key ids requested from the master server when the list is refreshed.
 *
 * @address 0x00695424
 */
inline auto &master_query_key_ids = halo::link::ref<uint8_t [10]>(halo::networking::vars().DAT_00695424);

}
