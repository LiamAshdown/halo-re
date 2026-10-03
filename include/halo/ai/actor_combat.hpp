#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "projectiles.h"

namespace halo::ai {

/**
 * Behaviour group "combat_ops" of the actor AI: 25 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class combat_ops {
public:
    explicit combat_ops(datum_index value) : datum(value) {}

    uint8_t check_burst_length_exceeded();
    void check_melee_target_reachable(actor_mode_flee_data *record);
    static uint8_t check_vehicle_target_available(datum_index vehicle_object_index, datum_index actor_index, uint8_t flag_pursue);
    uint8_t check_weapon_pickup_reachable(actor_mode_flee_data *record);
    void choose_best_target();
    static void choose_random_point_near(real_point3d *inout_point, float radius);
    void clear_target_state();
    float compute_accuracy_scale();
    static float compute_target_priority_weight(datum_index prop_index, datum_index actor_index);
    uint16_t consider_target_candidate(datum_index candidate_prop_index);
    uint8_t evaluate_custom_charge_trigger();
    static int16_t evaluate_flank_offset(real_vector3d *cover_direction, real_vector3d *out_offset, real_point3d *threat_position, real_point3d *candidate_position);
    void forward_target_object_reference(uint32_t param);
    void get_aim_from_position(uint32_t out_position[3]);
    float get_consideration_wait_threshold(int16_t mode, actor_combat_consideration *consideration);
    static datum_index get_relevant_squad_member_target(uint32_t unused_param, datum_index member_prop_index, char require_is_unit);
    datum_index get_squad_recent_attacker_target(char require_is_unit);
    static void get_target_state_flags(int16_t ax_mode, int16_t cx_mode, uint8_t shared_flag, uint32_t actor_index, int16_t mode_b, char force_c, char force_d, uint8_t *out_a, char *out_in_e, uint8_t *out_f, uint8_t *out_g, uint8_t *out_h, uint8_t *out_i);
    void * get_threat_weapon_definition();
    uint8_t is_burst_pending();
    uint8_t is_target_within_engagement_range();
    static int32_t evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster, real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask, datum_index exclude_object_index, uint8_t flying);
    datum_index get_threat_weapon_object_index();
    uint8_t has_unshielded_threat_weapon();
    static void issue_multi_target_vocalization(int16_t line, datum_index actor_index, int16_t variant, datum_index vehicle_object_index);

    datum_index datum;
};

}
