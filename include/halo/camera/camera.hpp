#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "crt.h"
#include <stdint.h>
#include "halo/camera/layout.hpp"

namespace halo::camera {

/**
 * Top-level camera subsystem entry points: initialisation, the per-frame update, the debug
 * camera controls, dead-player teammate spectating and the script hooks.
 */
class CameraSystem {
public:
    static void initialize();
    static void update(float dt);
    static void control(uint8_t enable);
    static uint8_t is_local_player_default_first_person();
    static int16_t get_seat_camera_state(datum_index unit, int16_t *out_state);
    static void script_set_animation(datum_index animation_tag, char *name);
    static datum_index dead_find_next_teammate(datum_index reference_player, datum_index current_target, uint8_t require_same_team);
    static uint8_t dead_player_has_teammate(datum_index reference_player);
    static void debug_start(int16_t camera_point_index, int16_t ticks, datum_index relative_object);
    static void debug_load_from_file();
    static void debug_save_to_file();
};

/**
 * Constructor-style initialiser of the dead-player (spectator) camera data.
 */
class DeadCamera {
public:
    static dead_camera_data * construct(dead_camera_data *self, int16_t local_player_index, datum_index unit);
};

}
