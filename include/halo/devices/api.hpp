/**
 * @file include/halo/devices/api.hpp
 * Functions of the devices module that other modules and the data tables call (namespace halo::devices). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>

struct data_array;

struct TagID;
struct device_placement_data;
struct real_orientation;
struct real_vector3d;
typedef uint32_t datum_index;

namespace halo::devices {

/**
 * The engine globals the devices module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    data_array *&device_groups;
};

Globals &globals();

void control_place(datum_index object_index, uint8_t *placement);
void device_blend_animations(datum_index object_index, real_orientation *orientations);
int device_can_change_position(uint32_t object_index);
void device_change_power_state(float fallback_value, uint32_t object_id);
void device_compute_function_values(uint32_t object_index);
void device_control_activate(uint32_t object_id);
void device_control_touched(uint32_t object_index);
uint8_t device_create(datum_index object_index);
void device_delete(datum_index object_index);
uint8_t device_frontfacing(uint32_t device_index, real_vector3d *forward);
uint8_t device_group_set_value(uint16_t group_index, float value);
void device_group_set_value_immediate(uint16_t group_index, float value);
void device_groups_allocate(void);
void device_groups_clear_disposing_flag(void);
void device_groups_dispose(void);
void device_groups_initialize(void);
void device_machine_melee_attacked(uint32_t object_index);
uint32_t device_machine_update(uint32_t object_index);
void device_new(uint32_t object_index, device_placement_data *placement);
void device_play_state_change_effect(uint32_t object_index, TagID tag_id);
uint8_t device_update_change_values(uint32_t object_index);
void light_fixture_place(datum_index object_index, uint8_t *placement);
uint8_t machine_create(datum_index object_index);
void machine_place(datum_index object_index, uint8_t *placement);

}
