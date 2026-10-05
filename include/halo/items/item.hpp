#pragma once

#include "halo/items/types.hpp"

namespace halo::items {

namespace {

/**
 * An equipment object addressed by its object datum index: creation, pickup and network codecs.
 */
class equipment_ref {
public:
    explicit equipment_ref(datum_index value) : datum(value) {}

    void apply_network_update(uint32_t *update_record);
    int32_t build_creation_message(uint32_t unused_arg2, uint32_t unused_arg3, uint32_t object_flags);
    int32_t build_network_update(uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type);
    static void create_from_creation_message(void *incoming_record);
    static void definition_play_pickup_sound(uint32_t equipment_tag_id);
    uint8_t is_old_enough();
    void network_baseline_take();
    uint8_t create();
    void new_from_placement(ScenarioEquipment *placement);
    void pickup_play_sound();
    int32_t send_creation(uint32_t arg2, uint32_t arg3);

    datum_index datum;
};

/**
 * A garbage object addressed by its object datum index.
 */
class garbage_ref {
public:
    explicit garbage_ref(datum_index value) : datum(value) {}

    uint8_t create();
    int32_t update();

    datum_index datum;
};

/**
 * An item object (the shared base of weapons, equipment and garbage) addressed by its object datum index.
 * Covers item physics, rotation, holder tracking and detonation timers.
 */
class item_ref {
public:
    explicit item_ref(datum_index value) : datum(value) {}

    void accelerate(real_vector3d *delta, uint8_t apply_detonation_timer);
    void align_to_normal_and_point(real_point3d *out_position, real_vector3d *normal, real_point3d *point);
    static uint32_t any_detonating();
    void compute_rotation();
    void detonation_timer_start();
    uint8_t get_effective_position(real_point3d *out_position);
    uint8_t create();
    void set_holder(datum_index holder_index);
    void stamp_age_timestamp();
    uint8_t update();

    datum_index datum;
};

}
}
