#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Removal of stray and dropped objects and per-unit game engine flags. Stateless service class: every function
 * is a static member and the state it acts on lives in the engine globals.
 */
class ObjectCleanup {
public:
    static void cleanup_dropped_objects(void);
    static void cleanup_stray_items(void);
    static void cleanup_stray_projectiles(void);
    static void clear_unit_shields_when_disabled(datum_index player_handle);
    static void flag_local_player_units(void);
    static uint8_t object_flag_bit3_clear(int32_t handle);
};

}
