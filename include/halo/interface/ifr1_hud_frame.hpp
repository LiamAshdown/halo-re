#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "items.h"
#include "effects.h"
#include "interface.h"
#include "units.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Behaviour of the original HudFrame functions.
 */
class HudFrame {
public:
    static void draw_damage_indicators(int16_t local_player_index);
    static void draw_grenade_interface(int16_t local_player_index, datum_index unit_index);
    static void draw_weapon_interface(player *p);
    static uint8_t player_weapon_ammo_state(const player *p, weapon_hud_ammo_state *out);
    static void render_unit_interface(player *p);
    static void state_allocate(void);
    static void state_reset(void);
    static void update_dispatch(void);
    static void update_interaction_prompt(datum_index player_index);
    static void update_player(void);
};

}
