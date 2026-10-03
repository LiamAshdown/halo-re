#pragma once

#include "halo/game/game2_types.hpp"

namespace halo::game {

/**
 * Multiplayer spawn placement, netgame equipment/flags, teleporters and round object resets.
 */
class EnginePlacement {
public:
    static float rate_location_ally_bonus(uint32_t self_index, real_point3d *point);
    static float rate_location_crowding(uint32_t self_index, real_point3d *point);
    static real rate_player_starting_location(ScenarioPlayerStartingLocation *location, datum_index player_handle);
    static uint32_t remap_placement_by_type(uint32_t handle);
    static uint32_t resolve_multiplayer_placement(uint32_t handle);
    static int32_t resolve_netgame_flag_role(uint32_t handle);
    static void scan_netgame_flags_noop(int16_t needle);
    static void spawn_or_replay_netgame_equipment(int32_t *message);
    static void update_netgame_equipment(char force_respawn);
    static void update_teleporter(uint32_t player_index);
    static void validate_scenario_placements_noop(void);
    static void touch_multiplayer_predicted_resources(void);
    static void update_item_scale_and_pickup(void);
    static void reset_round_objects(void);
    static void reset_vehicles_or_race_cleanup(void);
    static uint32_t pack_object_flags_or_passthrough(uint32_t input);

private:
    static void touch_tag_if_valid(int32_t tag_id);
};

}
