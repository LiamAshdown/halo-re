#pragma once

#include "halo/items/types.hpp"

namespace halo::items {

namespace {

/**
 * Operations on the trigger state machine of one weapon object, addressed by the weapon object index.
 * Wraps the original weapon_trigger_* and trigger_* functions; the trigger index stays an explicit argument.
 */
class weapon_trigger_ref {
public:
    explicit weapon_trigger_ref(datum_index value) : datum(value) {}

    void create_projectiles(int16_t trigger_index, uint32_t role);
    static void barrel_spread_offset(real_vector3d *v, real_vector3d *axis, uint16_t barrel_index, int16_t distribution_function, real distribution_angle, uint32_t flags);
    void become_charged(int16_t trigger_index);
    void begin_reload(int16_t magazine_index, int8_t is_client_predicted);
    void continue_burst(int16_t trigger_index);
    void effect_clear(int16_t trigger_index);
    void effect_set_out_of_ammo(int16_t trigger_index);
    void effect_set_state(int16_t trigger_index, int8_t state, int16_t counter);
    void enter_recovery(int16_t trigger_index);
    void finish_shot(int16_t trigger_index);
    void fire_or_reload(int16_t trigger_index, int8_t force);
    uint8_t get_aiming_vector(int16_t trigger_index, real_point3d *origin, real_point3d *target, uint8_t use_high_arc, real_vector3d *out_direction, real *out_time, real *out_range, uint8_t *out_used_straight_line);
    static real get_average_damage(datum_index weapon_tag_id, float *out_max_rate_of_fire);
    real get_charge_fraction(int16_t trigger_index);
    void handle_empty(int16_t trigger_index);
    real projectile_time_fraction(int16_t trigger_index, real elapsed);
    int32_t ready_to_fire(int16_t trigger_index);
    void reset_tracking(int16_t trigger_index);

    datum_index datum;
};

/**
 * A weapon object addressed by its object datum index: state, ammo, zoom, reload, firing and network codecs.
 * Every member is the body of the matching original weapon_* function; members without a receiver are static.
 */
class weapon_ref {
public:
    explicit weapon_ref(datum_index value) : datum(value) {}

    static int32_t add_ammunition(void **message_record);
    static void apply_ammo_correction(void **message_record);
    static void apply_ammo_correction_and_resync(void **message_record);
    void apply_network_update(uint32_t *update_record);
    int32_t build_creation_message(uint32_t unused_param_2, uint32_t unused_param_3, uint32_t object_flags);
    void build_hud_ammo_state(weapon_hud_ammo_state *out);
    int32_t build_network_update(uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type);
    real clamp_zoom_fov(int16_t zoom_level, real base_fov);
    static void create_from_creation_message(void *incoming_record);
    uint32_t fire_trigger(int16_t trigger_index);
    void force_settled_state();
    int16_t get_first_person_animation_time(int16_t animation_index, int16_t category, int16_t mode);
    char * get_label();
    int32_t get_next_zoom_level(int32_t current_level);
    real get_zoom_magnification(int16_t zoom_level);
    int32_t has_active_state();
    uint8_t is_old_enough();
    uint8_t is_out_of_ammo();
    int32_t is_reloading();
    void magazine_begin_chamber(int16_t magazine_index);
    void magazine_reload_tick(int16_t magazine_index);
    void magazine_reload_tick_predicted(int16_t magazine_index);
    uint32_t must_be_readied();
    void network_baseline_take();
    uint8_t create();
    datum_index new_from_placement(ScenarioWeapon *placement);
    void notify_ammo_pickup(int16_t magazine_index, int16_t rounds);
    void notify_reload_begin(int16_t magazine_index);
    void notify_reload_cancel(int16_t magazine_index);
    void notify_reload_step(int16_t magazine_index);
    uint32_t play_trigger_tag_effect(datum_index tag_id, real scale_a, real scale_b);
    static void predict_ammo(void **message_record);
    uint32_t prevents_grenade_throwing();
    uint32_t prevents_melee_attack();
    int32_t put_away(int8_t force);
    void ready();
    void reload_recovery_finish();
    void reset_triggers();
    int32_t send_creation(uint32_t arg2, uint32_t arg3);
    void set_ammo_counts(int16_t *reserve_counts);
    void set_control_flags(uint16_t control_flags, real primary_trigger);
    void set_loaded_ammo_fraction(real fraction);
    void set_ready_timer(real value);
    int32_t set_state(int16_t new_state, int8_t force);
    void set_state_indicator_flags();
    uint32_t stop_object_effect(datum_index tag_id);
    uint32_t transfer_ammunition(datum_index source_item_index, int16_t requesting_player_index, int16_t *out_transferred);
    int32_t triggers_idle();
    int32_t update();
    void update_function_values();

    datum_index datum;
};

}
}
