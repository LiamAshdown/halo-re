/**
 * @file src/networking/net2_server_sort.cpp
 * Server browser sort strategies.
 */
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "halo/networking/server_sort.hpp"
#include "halo/networking/api.hpp"


namespace halo::networking {

/**
 * Returns the ping-then-hostname comparator.
 */
server_browser_sort_comparator PingThenHostnameSort::comparator() const
{
    return reinterpret_cast<server_browser_sort_comparator>(&server_list_compare_by_ping_then_hostname);
}

/**
 * Returns the game type comparator.
 */
server_browser_sort_comparator GametypeSort::comparator() const
{
    return &server_list_compare_by_gametype;
}

/**
 * Returns the player count comparator.
 */
server_browser_sort_comparator PlayersSort::comparator() const
{
    return &server_list_compare_by_players;
}

/**
 * Returns the host name comparator.
 */
server_browser_sort_comparator HostnameSort::comparator() const
{
    return &server_list_compare_by_hostname;
}

/**
 * Returns the scroll-clamp entry point, as the original column 1 selection does.
 */
server_browser_sort_comparator ScrollClampSort::comparator() const
{
    return reinterpret_cast<server_browser_sort_comparator>(&server_list_scroll_clamp);
}

namespace {
constexpr ScrollClampSort k_scroll_clamp_sort{};
constexpr GametypeSort k_gametype_sort{};
constexpr PingThenHostnameSort k_ping_sort{};
constexpr PlayersSort k_players_sort{};
constexpr HostnameSort k_hostname_sort{};
}  // namespace

/**
 * Selects the strategy for a sort column: 1 scroll clamp, 2 game type, 3 ping then host name, 4 players, anything
 * else host name.
 */
const ServerSortStrategy &ServerSortRegistry::for_column(int32_t column)
{
    switch (column) {
    case 1:
        return k_scroll_clamp_sort;
    case 2:
        return k_gametype_sort;
    case 3:
        return k_ping_sort;
    case 4:
        return k_players_sort;
    default:
        return k_hostname_sort;
    }
}

}  // namespace halo::networking
