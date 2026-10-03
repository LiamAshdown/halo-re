#include "halo/projectiles/projectile.hpp"
#include "halo/projectiles/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern game_time_globals *game_time;
extern player_globals *local_player_globals;
extern data_array *player_data;
extern real_vector3d *global_origin3d_pointer;
extern game_main_globals *main_game_globals;
extern void ai_accumulate_repeated_event(datum_index object_index, real_point3d *origin, int32_t kind, int16_t noise, int32_t unused);
extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification);
extern double cos(double x);
extern double sin(double x);
extern double sqrt(double x);
extern float sound_definition_maximum_distance(datum_index sound_definition);
extern datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale);
}

namespace {

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(h)].data)
#define F(p, o) (*(float *)((p) + (o)))
static void projectile_raise_state(uint32_t projectile_index, int16_t state)
{
    uint8_t *o = OBJECT_DATA(projectile_index);

    if (((projectile_object *)o)->projectile.state < state) {
        ((projectile_object *)o)->projectile.state = state;
    }
}
#undef OBJECT_DATA
#undef F

}

namespace halo::projectiles {

namespace {
constexpr int16_t k_guided_zoom_table_index = 19;
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(h)].data)
#define F(p, o) (*(float *)((p) + (o)))
/**
 * Original function projectile_update; the author notes are in
 * docs/original/projectiles/projectile_update.c.txt.
 *
 * @address 0x4bdc00
 */
int ProjectileHandle::update()
{
    uint32_t projectile_index = (uint32_t)handle;

    uint8_t *obj = OBJECT_DATA(projectile_index);                 
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data; 
    projectile_object *self = (projectile_object *)obj;
    Projectile *definition = (Projectile *)tag;
    real_vector3d *velocity = &self->base.velocity;
    real_vector3d *forward = &self->base.forward;
    real_vector3d *up = &self->base.up;
    real remaining = 1.0f;          
    int16_t collisions = 0;         
    uint8_t flyby_played = 0;       
    collision_result hit;           

    memset(&hit, 0, sizeof(hit));

    
    if (!(self->projectile.flags & to_bits(projectile_flag::tracer)) && self->projectile.contrail_attachment_index != -1) {
        int32_t slot = self->projectile.contrail_attachment_index;

        if (self->base.attachment_handles[slot] != k_datum_index_none) {
            halo::effects::contrail_delete(self->base.attachment_handles[slot]);
        }
        self->base.attachment_handles[self->projectile.contrail_attachment_index] = k_datum_index_none;
        self->projectile.contrail_attachment_index = -1;
    }
    self->projectile.arming_timer += self->projectile.arming_timer_rate; 
    self->projectile.deceleration_delay = self->projectile.deceleration_delay_rate + self->projectile.deceleration_delay; 
    {
        int16_t starts = definition->detonation_timer_starts;
        uint8_t condition = (starts == 1 || starts == 2) ? (uint8_t)((self->projectile.flags & to_bits(projectile_flag::at_rest)) != 0) : 1;
        uint32_t flags = self->projectile.flags;

        if ((flags & to_bits(projectile_flag::detonation_timer_started)) || (flags & to_bits(projectile_flag::attached)) || condition) {
            if (!(flags & to_bits(projectile_flag::detonation_timer_started))) {
                self->projectile.flags = flags | to_bits(projectile_flag::detonation_timer_started);
            }
            self->projectile.detonation_timer = self->projectile.detonation_timer_rate + self->projectile.detonation_timer;
            if (!(self->projectile.detonation_timer < 1.0f)) {
                projectile_raise_state(projectile_index, 1);
            }
        }
    }
    projectile_update_function_values(projectile_index);

    for (;;) {
        int16_t state = self->projectile.state;
        real_vector3d vel;          
        real_vector3d step;         
        real_point3d swept;         
        real speed;                 
        real speed_after;           
        real average_speed;         
        real gravity;               
        real vel_k;                 
        real step_k;
        real scale;
        uint8_t collision_attempted = 0; 
        datum_index shooter;        

        if (state != 0 && !(state == 1 && self->projectile.arming_timer_rate != 0.0f && self->projectile.arming_timer < 1.0f)) {
            break;
        }
        if ((self->projectile.flags & to_bits(projectile_flag::attached)) || (self->base.flags & to_bits(projectile_object_flag::at_rest)) ||
            self->base.parent_object != k_datum_index_none) {
            break;
        }
        vel = *velocity;
        speed = (real)sqrt(vel.i * vel.i + vel.j * vel.j + vel.k * vel.k);
        speed_after = speed;
        average_speed = speed;
        step = vel;
        shooter = self->projectile.ignore_object_index;

        
        if (self->projectile.tracked_object_index != k_datum_index_none && definition->guided_angular_velocity > 0.0f) {
            datum_index tracked_index = self->projectile.tracked_object_index;
            object *tracked_object = (object *)OBJECT_DATA(tracked_index);
            real turn = definition->guided_angular_velocity * k_seconds_per_tick;   
            real fade;                                  
            real distance;
            real_point3d target;                        
            real_vector3d to_target;                    
            real_vector3d axis;                         
            int32_t salt = (int32_t)projectile_index >> 16;
            int32_t tick = game_time->game_time;
            real angle_a;
            real angle_b;

            if (((1u << (tracked_object->type & 0x1f)) & 3) && ((unit_data *)((uint8_t *)tracked_object + k_unit_data_offset))->controlling_player != k_datum_index_none) {
                turn *= weapon_get_zoom_fov(k_guided_zoom_table_index, halo::main::globals().game_globals->difficulty);
            }
            {
                real dx = self->base.bounding_center.x - tracked_object->bounding_center.x;
                real dy = self->base.bounding_center.y - tracked_object->bounding_center.y;
                real dz = self->base.bounding_center.z - tracked_object->bounding_center.z;

                distance = (real)sqrt(dx * dx + dy * dy + dz * dz);
            }
            if (!(distance <= 10.0f)) {
                fade = 1.0f;
            } else if (distance <= 2.0f) {
                fade = 0.0f;
            } else {
                fade = (distance - 2.0f) * 0.125f;
                if (fade < 0.0f) {
                    fade = 0.0f;
                } else if (!(fade <= 1.0f)) {
                    fade = 1.0f;
                }
            }
            halo::units::unit_get_secondary_eye_marker_position(tracked_index, &target);
            angle_a = halo::math::periodic_function_evaluate(_periodic_function_wander,
                (double)((real)(int32_t)((salt * 7 + tick) & halo::k_datum_slot_mask) * 0.011111111f)) * 6.2831855f;
            angle_b = 3.1415927f - halo::math::periodic_function_evaluate(_periodic_function_wander,
                (double)((real)(int32_t)((tick + salt * 3) & halo::k_datum_slot_mask) * 0.011111111f)) * 1.5707964f;
            {
                real cos_b = (real)cos(angle_b);
                real wander_x = (real)cos(angle_a) * cos_b;
                real wander_y = (real)sin(angle_a) * cos_b;
                real wander_z = (real)sin(angle_b);

                target.x += wander_x * fade;
                target.y += wander_y * fade;
                target.z += wander_z * fade;
            }
            to_target.i = target.x - self->base.position.x;
            to_target.j = target.y - self->base.position.y;
            to_target.k = target.z - self->base.position.z;
            halo::math::vector3d_cross_product(axis, to_target, *velocity);
            if (to_target.k * velocity->k + to_target.j * velocity->j + to_target.i * velocity->i > 0.0f &&
                halo::math::vector3d_normalize_with_length(axis) > 0.0f) {
                halo::math::vector3d_rotate_about_axis(vel, axis, (real)sin(turn), (real)cos(turn));
            }
        }

        
        vel_k = vel.k;
        if (!(self->projectile.deceleration_delay < 1.0f)) {
            real final_speed = definition->final_velocity;

            if (speed > final_speed && self->projectile.deceleration != 0.0f) {
                real drop = remaining * self->projectile.deceleration;

                speed_after = speed - drop;
                if (!(speed_after > final_speed)) {
                    
                    real f = (speed - final_speed) / drop;   
                    real g = 1.0f - f;                      
                    real ratio;

                    speed_after = final_speed * 0.99f;
                    average_speed = (speed_after + speed) * f * 0.5f + g * final_speed;
                    ratio = speed_after / speed;
                    vel.i *= ratio;
                    vel.j *= ratio;
                    vel_k = ratio * vel.k;
                    step.i = (vel.i + velocity->i) * f * 0.5f + g * vel.i;
                    step.j = (vel.j + velocity->j) * f * 0.5f + g * vel.j;
                    step.k = (vel_k + velocity->k) * f * 0.5f + g * vel_k;
                } else {
                    real ratio;

                    average_speed = speed - drop * 0.5f;
                    ratio = speed_after / speed;
                    vel.i *= ratio;
                    vel.j *= ratio;
                    vel_k = ratio * vel.k;
                    step.i = (vel.i + velocity->i) * 0.5f;
                    step.j = (vel.j + velocity->j) * 0.5f;
                    step.k = (vel_k + velocity->k) * 0.5f;
                }
            } else if (definition->maximum_range == 0.0f && definition->timer[1] == 0.0f && !(definition->minimum_velocity > definition->final_velocity) &&
                       (self->projectile.deceleration != 0.0f || !(self->projectile.distance_travelled < self->projectile.deceleration_end_range))) {
                
                projectile_request_state(projectile_index, 2);
            } else if (speed < final_speed && speed > 0.0f) {
                
                real ratio = final_speed / speed * 0.99f;

                vel.i *= ratio;
                vel.j *= ratio;
                vel_k = ratio * vel.k;
            }
        }

        
        halo::physics::globals().gravity = halo::physics::k_physics_gravity * ((self->base.flags & to_bits(projectile_object_flag::in_water)) ? definition->water_gravity_scale : definition->air_gravity_scale);
        vel.k = vel_k - gravity * remaining;
        step_k = step.k - gravity * remaining * 0.5f;

        
        scale = 1.0f;
        if (definition->maximum_range != 0.0f && !(average_speed * remaining + self->projectile.distance_travelled <= definition->maximum_range)) {
            if (average_speed == 0.0f) {
                scale = 0.0f;
            } else {
                scale = (definition->maximum_range - self->projectile.distance_travelled) / average_speed * remaining;
            }
            projectile_request_state(projectile_index, 1);
        }
        scale *= remaining;
        swept.x = step.i * scale + self->base.position.x;
        swept.y = step.j * scale + self->base.position.y;
        swept.z = scale * step_k + self->base.position.z;

        
        {
            uint8_t hit_something = 0;

            if (collisions == 10) {
                projectile_raise_state(projectile_index, 1);
            } else if (self->projectile.state != 2) {
                collision_attempted = 1;
                hit_something = projectile_collision_test(projectile_index, &swept, &hit);
            }
            if (hit_something) {
                remaining = 1.0f - hit.t;
                vel.k += gravity * remaining;
                if (speed_after != 0.0f) {
                    real s = remaining * self->projectile.deceleration + speed_after;
                    real ratio;

                    if (!(s <= speed)) {
                        s = speed;
                    }
                    ratio = s / speed_after;
                    vel.i *= ratio;
                    vel.j *= ratio;
                    vel.k *= ratio;
                }
                if (hit.plane.normal.k > 0.3f) {
                    self->projectile.flags |= to_bits(projectile_flag::hit_ground);
                }
                self->projectile.ignore_object_index = k_datum_index_none;
                projectile_response(projectile_index, &hit, &swept, &vel);
                collisions++;
                ai_accumulate_repeated_event(projectile_index, &hit.point, 1, definition->impact_noise, 1);
                if (self->projectile.flags & to_bits(projectile_flag::attached)) {
                    goto next_step;
                }
            } else {
                remaining = 0.0f;
                if (!collision_attempted) {
                    break;
                }
            }
        }

        
        {
            real_vector3d moved;    

            moved.i = swept.x - self->base.position.x;
            moved.j = swept.y - self->base.position.y;
            moved.k = swept.z - self->base.position.z;
            self->projectile.distance_travelled = (real)sqrt(moved.k * moved.k + moved.j * moved.j + moved.i * moved.i) + self->projectile.distance_travelled;
            if (!flyby_played && *(datum_index *)&definition->flyby_sound.tag_id != k_datum_index_none &&
                *(datum_index *)local_player_globals->local_players != k_datum_index_none) {
                datum_index local_player = *(datum_index *)local_player_globals->local_players;
                datum_index listener = ((player *)player_data->data)[halo::datum_slot(local_player)].unit;

                if (listener != k_datum_index_none && listener != shooter) {
                    real_point3d *center = &((object *)OBJECT_DATA(listener))->bounding_center;
                    real radius = halo::sound::sound_definition_maximum_distance(*(datum_index *)&definition->flyby_sound.tag_id);
                    real_vector3d to_listener;  
                    real_vector3d projected;    
                    real_vector3d perpendicular; 
                    real along;

                    to_listener.i = center->x - self->base.position.x;
                    to_listener.j = center->y - self->base.position.y;
                    to_listener.k = center->z - self->base.position.z;
                    halo::math::vector3d_project_onto_axis(projected, moved, to_listener, perpendicular);
                    along = projected.k * moved.k + projected.j * moved.j + projected.i * moved.i;
                    if (!(along < 0.0f) && halo::math::vector3d_magnitude_squared(moved) > along &&
                        radius * radius > halo::math::vector3d_magnitude_squared(perpendicular)) {
                        sound_placement placement;  

                        placement.position.x = center->x - perpendicular.i;
                        placement.position.y = center->y - perpendicular.j;
                        placement.position.z = center->z - perpendicular.k;
                        *(real_vector3d *)&placement.forward = moved;
                        halo::math::vector3d_normalize_with_length(*((real_vector3d *)&placement.forward));
                        *(real_vector3d *)&placement.velocity = *global_origin3d_pointer;
                        placement.leaf_index = *(int32_t *)&hit.leaf;
                        *(int32_t *)&placement.cluster_index = *(int32_t *)((uint8_t *)&hit.leaf + 4);
                        halo::sound::sound_start_at_location(*(datum_index *)&definition->flyby_sound.tag_id, &placement, 1.0f);
                        flyby_played = 1;
                    }
                }
            }
        }

        
        if ((definition->projectile_flags & to_bits(projectile_definition_flag::oriented_along_velocity)) &&
            (velocity->i != 0.0f || velocity->j != 0.0f || velocity->k != 0.0f)) {
            real_vector3d direction = *velocity;

            if (halo::math::vector3d_normalize_with_length(direction) > 0.0f) {
                real_vector3d side;

                *forward = direction;
                halo::math::vector3d_cross_product(side, *forward, *up);
                halo::math::vector3d_cross_product(*up, side, *forward);
                if (halo::math::vector3d_normalize_with_length(*up) == 0.0f) {
                    halo::math::vector3d_build_perpendicular(*up, *forward);
                    halo::math::vector3d_normalize_with_length(*up);
                }
            }
            halo::math::vector3d_rotate_about_axis(*up, *forward, self->projectile.rotation_sine, self->projectile.rotation_cosine);
        } else if (self->projectile.flags & to_bits(projectile_flag::rotation_valid)) {
            real_vector3d *axis = &self->projectile.rotation_axis;
            real_vector3d side;

            halo::math::vector3d_rotate_about_axis(*forward, *axis, self->projectile.rotation_sine, self->projectile.rotation_cosine);
            halo::math::vector3d_rotate_about_axis(*up, *axis, self->projectile.rotation_sine, self->projectile.rotation_cosine);
            halo::math::vector3d_normalize_with_length(*forward);
            halo::math::vector3d_cross_product(side, *forward, *up);
            halo::math::vector3d_cross_product(*up, side, *forward);
            halo::math::vector3d_normalize_with_length(*up);
        }

        
        halo::objects::object_unlink_cluster_or_notify_parent(projectile_index);
        self->base.position = swept;
        halo::objects::object_set_cluster_and_parent(projectile_index, &hit.leaf);
        *velocity = vel;
        if (remaining != 0.0f && collisions != 0 && self->projectile.contrail_attachment_index != -1 &&
            self->base.attachment_handles[self->projectile.contrail_attachment_index] != k_datum_index_none) {
            halo::objects::object_recalculate_bounding_radius(projectile_index);
            halo::effects::contrail_advance(self->base.attachment_handles[self->projectile.contrail_attachment_index], 0,
                             (1.0f - remaining) * k_seconds_per_tick);
        }
    next_step:
        if (!(remaining > 0.0f)) {
            break;
        }
    }

    
    switch (self->projectile.state) {
    case _projectile_state_detonating:
        if (self->projectile.arming_timer_rate != 0.0f && self->projectile.arming_timer < 1.0f) {
            return 1;
        }
        if (self->base.network_role == 1) {
            return 1;
        }
        if (self->base.network_role == 0 && self->projectile.thrown_grenade == 1) {
            projectile_send_detonation(projectile_index);
        }
        projectile_detonate(projectile_index, (char)(collisions == 0), remaining);
        
    case _projectile_state_disappearing: {
        int32_t role = ((object *)OBJECT_DATA(projectile_index))->network_role;

        if (role == 0) {
            halo::objects::object_delete_unparented(projectile_index);
            halo::objects::object_delete_recursive(projectile_index, 0);
        } else if (role == 3) {
            halo::objects::object_delete_recursive(projectile_index, 0);
        }
        break;
    }
    default:
        break;
    }
    return 1;
}
#undef OBJECT_DATA
#undef F

}

namespace halo::projectiles {

int projectile_update(uint32_t projectile_index)
{
    return halo::projectiles::ProjectileHandle(projectile_index).update();
}

}
