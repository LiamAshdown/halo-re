#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Behaviour of the original FirstPersonWeaponController functions.
 */
class FirstPersonWeaponController {
public:
    static void center_flashlight(datum_index unit_index, real_point3d *out_origin, real_vector3d *out_extents, real_vector3d *out_direction);
    static uint32_t get_marker_data(datum_index weapon_index, const char *marker_name, object_marker *out, uint32_t maximum);
    static void interface_initialize(int16_t local_player_index);
    static void interface_tick(void);
    static void interface_tick_reset(int16_t local_player_index);
    static void process_action(int16_t local_player_index, int16_t action_code);
    static void set_attached(int16_t local_player_index, uint8_t attached);
    static void set_state(int16_t local_player_index, uint8_t force_pose_snapshot, int16_t new_state);
    static void snapshot_pose(int16_t local_player_index, int16_t blend_gap);
    static void update(int16_t local_player_index);
    static void update_active_state(void);
    static void update_animation_controls(int16_t local_player_index);
    static void update_lighting(void);
    static void update_screen_effects(void);
    static void update_state(int16_t local_player_index);
    static void update_zoom_static_tint(uint8_t enabled);
};

}
