#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Network game hosting, autojoin and per-session reset.
 */
class NetworkSetup {
public:
    NetworkSetup() = delete;

    static uint8_t host_session_start();
    static uint8_t autojoin_from_command_line();
    static void clear_player_ready_flags();
    static void game_host_start(char *map_name, char *variant_name, uint8_t disconnect_timeout_flag);
    static void game_setup_teardown();
    static uint32_t server_reset_game_stats();
};

/**
 * Multiplayer settings and server list menu widgets.
 */
class MenuListView {
public:
    explicit constexpr MenuListView(widget_instance *view_widget) : widget(view_widget) {}
    widget_instance *widget;

    void refresh_3wide();
    void update_item(const uint16_t *record);
    uint32_t choice_handler();
    void update();
};

} // namespace halo::interface
