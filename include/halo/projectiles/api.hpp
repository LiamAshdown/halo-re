/**
 * @file include/halo/projectiles/api.hpp
 * Functions of the projectiles module that other modules and the data tables call (namespace halo::projectiles). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdarg.h>
#include <stdint.h>

typedef float real;
struct Projectile;
struct collision_result;
struct real_point3d;
struct real_vector3d;
struct update_record;
typedef uint32_t datum_index;

namespace halo::projectiles {

void projectile_apply_network_update(datum_index projectile_index, uint32_t *update_record);
int32_t projectile_build_network_update(uint32_t projectile_index, uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type);
void projectile_network_baseline_take(uint32_t object_index);
void projectile_request_state(datum_index projectile_index, int16_t requested_state);
int32_t projectile_send_creation(uint32_t projectile_index);
void projectile_attach_apply(void *incoming_record);
void projectile_create_from_network(void *incoming_record);
void projectile_detonation_message_apply(void *incoming_record);
void projectile_send_attach(datum_index projectile_index, datum_index parent_object_index, int16_t marker_index);
void projectile_send_detonation(datum_index projectile_index);
uint8_t projectile_new(uint32_t object_index);
void projectile_update_function_values(uint32_t object_index);
void projectile_compute_deceleration(uint32_t object_index);
void projectile_compute_rotation(uint32_t object_index);
uint8_t projectile_collision_test(uint32_t object_index, real_point3d *target, void *out_record);
void projectile_detonate(uint32_t object_index, char first_collision, real remaining_tick_fraction);
uint8_t projectile_force_detonate(uint32_t object_index);
uint8_t projectile_is_old_enough(uint32_t object_index);
void projectile_notify_object_deleted(uint32_t object_index, datum_index dying_object_index);
real projectile_deceleration_from_range(Projectile *tag, real r0, real r1);
uint8_t object_type_definition_return_false(void);
uint8_t object_type_definition_return_true(void);
int projectile_update(uint32_t projectile_index);
void projectile_response(datum_index projectile_index, collision_result *hit, real_point3d *out_position, real_vector3d *velocity);

}
