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
 * First-person weapon presentation for one local player: weapon state machine, pose snapshots, animation controls and
 * the screen effects that follow from them. An instance is bound to a local player index; the static members work on
 * state shared by all local players.
 */
class FirstPersonWeaponController {
public:
    explicit FirstPersonWeaponController(int16_t local_player_index) : local_player_index(local_player_index) {}

    static void center_flashlight(datum_index unit_index, real_point3d *out_origin, real_vector3d *out_extents, real_vector3d *out_direction);
    static uint32_t get_marker_data(datum_index weapon_index, const char *marker_name, object_marker *out, uint32_t maximum);
    void interface_initialize();
    static void interface_tick(void);
    void interface_tick_reset();
    void process_action(int16_t action_code);
    void set_attached(uint8_t attached);
    void set_state(uint8_t force_pose_snapshot, int16_t new_state);
    void snapshot_pose(int16_t blend_gap);
    void update();
    static void update_active_state(void);
    void update_animation_controls();
    static void update_lighting(void);
    static void update_screen_effects(void);
    void update_state();
    static void update_zoom_static_tint(uint8_t enabled);

private:
    int16_t local_player_index;
};

}
