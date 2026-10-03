#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "camera.h"

namespace halo::game {

/**
 * Developer cheat commands (invincibility, spawning, teleporting).
 */
class Cheats {
public:
    Cheats() = delete;

    static void all_weapons();
    static uint32_t get_target_object_index();
    static void make_player_invincible(int16_t local_player_slot);
    static void make_selected_object_invincible();
    static void spawn_objects_near_camera(TagDependency *tag_array, int16_t count);
    static void spawn_warthog();
    static void teleport_to_camera();
};

}  // namespace halo::game
