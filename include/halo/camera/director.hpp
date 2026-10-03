#pragma once

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "camera.h"
#include <wchar.h>

namespace halo::camera {
namespace {

/**
 * View of the director for one local player: chooses the gameplay camera mode, builds the
 * camera input and switches between flying and seat cameras.
 */
class DirectorHandle {
public:
    explicit DirectorHandle(int16_t value) : player_handle(value) {}
    uint8_t build_camera_input(camera_input *input);
    void choose_gameplay_camera(uint8_t reset);
    void set_flying_camera(uint8_t force);
    void update_seat_camera(uint8_t force);
    int16_t get_type_for_player();
    void input_axes_update(uint32_t key_bits, float zoom);

    int16_t player_handle;
};

/**
 * Director notifications that are not tied to one player.
 */
class DirectorEvents {
public:
    static void game_state_loaded();
};

}
}
