#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "game.h"

namespace halo::game {

/**
 * Scripting-function handlers that act on players and vehicles.
 */
class HsPlayerFunctions {
public:
    HsPlayerFunctions() = delete;

    static void vehicle_gunner_evaluate(int16_t function_index, uint32_t thread_index, char first);
    static void vehicle_test_seat_evaluate(int16_t function_index, uint32_t thread_index, char first);
    static void camo_screen_effect(int16_t index, uint32_t thread_index, hs_function_definition *definition, char first);
    static void examine_nearby_vehicle(int16_t index, uint32_t thread_index, hs_function_definition *definition, char first);
    static void set_action_result(int16_t function_index, uint32_t thread_index, char first);
};

}  // namespace halo::game
