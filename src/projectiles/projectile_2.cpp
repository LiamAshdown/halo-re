#include "halo/projectiles/projectile.hpp"

extern "C" {
extern data_array *object_data;
extern tag_instance *tag_instances;
extern game_time_globals *game_time;
extern player_globals *local_player_globals;
extern data_array *player_data;
extern real_vector3d *global_origin3d_pointer;
extern game_main_globals *main_game_globals;
extern float k_physics_gravity;
extern void contrail_delete(datum_index attachment_handle);
extern void projectile_update_function_values(datum_index projectile_index);
extern void projectile_request_state(datum_index projectile_index, int16_t requested_state);
extern uint8_t projectile_collision_test(uint32_t object_index, real_point3d *target, void *out_record);
extern void projectile_response(datum_index projectile_index, collision_result *hit, real_point3d *out_position, real_vector3d *velocity);
extern void ai_accumulate_repeated_event(datum_index object_index, real_point3d *origin, int32_t kind, int16_t noise, int32_t unused);
extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification);
extern void unit_get_secondary_eye_marker_position(uint32_t object_index, real_point3d *out);
extern real periodic_function_evaluate(periodic_function_t type, double time);
extern double cos(double x);
extern double sin(double x);
extern double sqrt(double x);
extern real vector3d_magnitude_squared(real_vector3d *v);
extern float sound_definition_maximum_distance(datum_index sound_definition);
extern void vector3d_project_onto_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out);
extern datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale);
extern real vector3d_normalize_with_length(real_vector3d *v);
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle);
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir);
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index);
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location);
extern void object_recalculate_bounding_radius(uint32_t object_index);
extern void contrail_advance(datum_index contrail_handle, uint8_t detach, real delta_time);
extern void projectile_send_detonation(datum_index projectile_index);
extern void projectile_detonate(uint32_t object_index, char first_collision, real remaining_tick_fraction);
extern void object_delete_unparented(uint32_t object_index);
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings);
int projectile_update(uint32_t projectile_index);
}

namespace {

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
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

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
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
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data; 
    real_vector3d *velocity = (real_vector3d *)(obj + 0x68);
    real_vector3d *forward = (real_vector3d *)(obj + 0x74);
    real_vector3d *up = (real_vector3d *)(obj + 0x80);
    real remaining = 1.0f;          
    int16_t collisions = 0;         
    uint8_t flyby_played = 0;       
    collision_result hit;           

    memset(&hit, 0, sizeof(hit));

    
    if (!(((projectile_object *)obj)->projectile.flags & 2) && ((projectile_object *)obj)->projectile.contrail_attachment_index != -1) {
        int32_t slot = ((projectile_object *)obj)->projectile.contrail_attachment_index;

        if (((projectile_object *)obj)->base.attachment_handles[slot] != k_datum_index_none) {
            contrail_delete(((projectile_object *)obj)->base.attachment_handles[slot]);
        }
        *(datum_index *)(obj + 0x14c + ((projectile_object *)obj)->projectile.contrail_attachment_index * 4) = k_datum_index_none;
        ((projectile_object *)obj)->projectile.contrail_attachment_index = -1;
    }
    F(obj, 0x248) += F(obj, 0x24c); 
    F(obj, 0x254) = F(obj, 0x258) + F(obj, 0x254); 
    {
        int16_t starts = ((Projectile *)tag)->detonation_timer_starts;
        uint8_t condition = (starts == 1 || starts == 2) ? (uint8_t)((((projectile_object *)obj)->projectile.flags >> 4) & 1) : 1;
        uint32_t flags = ((projectile_object *)obj)->projectile.flags;

        if ((flags & 0x20) || (flags & 8) || condition) {
            if (!(flags & 0x20)) {
                ((projectile_object *)obj)->projectile.flags = flags | 0x20;
            }
            F(obj, 0x240) = F(obj, 0x244) + F(obj, 0x240);
            if (!(F(obj, 0x240) < 1.0f)) {
                projectile_raise_state(projectile_index, 1);
            }
        }
    }
    projectile_update_function_values(projectile_index);

    for (;;) {
        int16_t state = ((projectile_object *)obj)->projectile.state;
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

        if (state != 0 && !(state == 1 && F(obj, 0x24c) != 0.0f && F(obj, 0x248) < 1.0f)) {
            break;
        }
        if ((((projectile_object *)obj)->projectile.flags & 8) || (((projectile_object *)obj)->base.flags & 0x20) ||
            ((projectile_object *)obj)->base.parent_object != k_datum_index_none) {
            break;
        }
        vel = *velocity;
        speed = (real)sqrt(vel.i * vel.i + vel.j * vel.j + vel.k * vel.k);
        speed_after = speed;
        average_speed = speed;
        step = vel;
        shooter = ((projectile_object *)obj)->projectile.ignore_object_index;

        
        if (((projectile_object *)obj)->projectile.tracked_object_index != k_datum_index_none && ((Projectile *)tag)->guided_angular_velocity > 0.0f) {
            datum_index tracked_index = ((projectile_object *)obj)->projectile.tracked_object_index;
            uint8_t *tracked = OBJECT_DATA(tracked_index);
            real turn = ((Projectile *)tag)->guided_angular_velocity * 0.033333335f;   
            real fade;                                  
            real distance;
            real_point3d target;                        
            real_vector3d to_target;                    
            real_vector3d axis;                         
            int32_t salt = (int32_t)projectile_index >> 16;
            int32_t tick = game_time->game_time;
            real angle_a;
            real angle_b;

            if (((1u << (tracked[0xb4] & 0x1f)) & 3) && *(datum_index *)(tracked + 0x218) != k_datum_index_none) {
                turn *= weapon_get_zoom_fov(0x13, main_game_globals->difficulty);
            }
            {
                real dx = F(obj, 0xa0) - F(tracked, 0xa0);
                real dy = F(obj, 0xa4) - F(tracked, 0xa4);
                real dz = F(obj, 0xa8) - F(tracked, 0xa8);

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
            unit_get_secondary_eye_marker_position(tracked_index, &target);
            angle_a = periodic_function_evaluate(_periodic_function_wander,
                (double)((real)(int32_t)((salt * 7 + tick) & 0xffff) * 0.011111111f)) * 6.2831855f;
            angle_b = 3.1415927f - periodic_function_evaluate(_periodic_function_wander,
                (double)((real)(int32_t)((tick + salt * 3) & 0xffff) * 0.011111111f)) * 1.5707964f;
            {
                real cos_b = (real)cos(angle_b);
                real wander_x = (real)cos(angle_a) * cos_b;
                real wander_y = (real)sin(angle_a) * cos_b;
                real wander_z = (real)sin(angle_b);

                target.x += wander_x * fade;
                target.y += wander_y * fade;
                target.z += wander_z * fade;
            }
            to_target.i = target.x - F(obj, 0x5c);
            to_target.j = target.y - F(obj, 0x60);
            to_target.k = target.z - F(obj, 0x64);
            vector3d_cross_product(&axis, &to_target, velocity);
            if (to_target.k * velocity->k + to_target.j * velocity->j + to_target.i * velocity->i > 0.0f &&
                vector3d_normalize_with_length(&axis) > 0.0f) {
                vector3d_rotate_about_axis(&vel, &axis, (real)sin(turn), (real)cos(turn));
            }
        }

        
        vel_k = vel.k;
        if (!(F(obj, 0x254) < 1.0f)) {
            real final_speed = F(tag, 0x1e8);

            if (speed > final_speed && F(obj, 0x25c) != 0.0f) {
                real drop = remaining * F(obj, 0x25c);

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
            } else if (F(tag, 0x1c8) == 0.0f && F(tag, 0x1c0) == 0.0f && !(F(tag, 0x1c4) > F(tag, 0x1e8)) &&
                       (F(obj, 0x25c) != 0.0f || !(F(obj, 0x250) < F(obj, 0x260)))) {
                
                projectile_request_state(projectile_index, 2);
            } else if (speed < final_speed && speed > 0.0f) {
                
                real ratio = final_speed / speed * 0.99f;

                vel.i *= ratio;
                vel.j *= ratio;
                vel_k = ratio * vel.k;
            }
        }

        
        gravity = k_physics_gravity * ((((projectile_object *)obj)->base.flags & 0x10) ? ((Projectile *)tag)->water_gravity_scale : ((Projectile *)tag)->air_gravity_scale);
        vel.k = vel_k - gravity * remaining;
        step_k = step.k - gravity * remaining * 0.5f;

        
        scale = 1.0f;
        if (F(tag, 0x1c8) != 0.0f && !(average_speed * remaining + F(obj, 0x250) <= F(tag, 0x1c8))) {
            if (average_speed == 0.0f) {
                scale = 0.0f;
            } else {
                scale = (F(tag, 0x1c8) - F(obj, 0x250)) / average_speed * remaining;
            }
            projectile_request_state(projectile_index, 1);
        }
        scale *= remaining;
        swept.x = step.i * scale + F(obj, 0x5c);
        swept.y = step.j * scale + F(obj, 0x60);
        swept.z = scale * step_k + F(obj, 0x64);

        
        {
            uint8_t hit_something = 0;

            if (collisions == 10) {
                projectile_raise_state(projectile_index, 1);
            } else if (((projectile_object *)obj)->projectile.state != 2) {
                collision_attempted = 1;
                hit_something = projectile_collision_test(projectile_index, &swept, &hit);
            }
            if (hit_something) {
                remaining = 1.0f - hit.t;
                vel.k += gravity * remaining;
                if (speed_after != 0.0f) {
                    real s = remaining * F(obj, 0x25c) + speed_after;
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
                    ((projectile_object *)obj)->projectile.flags |= 4;
                }
                ((projectile_object *)obj)->projectile.ignore_object_index = k_datum_index_none;
                projectile_response(projectile_index, &hit, &swept, &vel);
                collisions++;
                ai_accumulate_repeated_event(projectile_index, &hit.point, 1, ((Projectile *)tag)->impact_noise, 1);
                if (((projectile_object *)obj)->projectile.flags & 8) {
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

            moved.i = swept.x - F(obj, 0x5c);
            moved.j = swept.y - F(obj, 0x60);
            moved.k = swept.z - F(obj, 0x64);
            F(obj, 0x250) = (real)sqrt(moved.k * moved.k + moved.j * moved.j + moved.i * moved.i) + F(obj, 0x250);
            if (!flyby_played && *(datum_index *)&((Projectile *)tag)->flyby_sound.tag_id != k_datum_index_none &&
                *(datum_index *)local_player_globals->local_players != k_datum_index_none) {
                datum_index player = *(datum_index *)local_player_globals->local_players;
                datum_index listener = *(datum_index *)((uint8_t *)player_data->data + (player & 0xffff) * 0x200 + 0x34);

                if (listener != k_datum_index_none && listener != shooter) {
                    real_point3d *center = (real_point3d *)(OBJECT_DATA(listener) + 0xa0);
                    real radius = sound_definition_maximum_distance(*(datum_index *)&((Projectile *)tag)->flyby_sound.tag_id);
                    real_vector3d to_listener;  
                    real_vector3d projected;    
                    real_vector3d perpendicular; 
                    real along;

                    to_listener.i = center->x - F(obj, 0x5c);
                    to_listener.j = center->y - F(obj, 0x60);
                    to_listener.k = center->z - F(obj, 0x64);
                    vector3d_project_onto_axis(&projected, &moved, &to_listener, &perpendicular);
                    along = projected.k * moved.k + projected.j * moved.j + projected.i * moved.i;
                    if (!(along < 0.0f) && vector3d_magnitude_squared(&moved) > along &&
                        radius * radius > vector3d_magnitude_squared(&perpendicular)) {
                        sound_placement placement;  

                        placement.position.x = center->x - perpendicular.i;
                        placement.position.y = center->y - perpendicular.j;
                        placement.position.z = center->z - perpendicular.k;
                        *(real_vector3d *)&placement.forward = moved;
                        vector3d_normalize_with_length((real_vector3d *)&placement.forward);
                        *(real_vector3d *)&placement.velocity = *global_origin3d_pointer;
                        placement.leaf_index = *(int32_t *)&hit.leaf;
                        *(int32_t *)&placement.cluster_index = *(int32_t *)((uint8_t *)&hit.leaf + 4);
                        sound_start_at_location(*(datum_index *)&((Projectile *)tag)->flyby_sound.tag_id, &placement, 1.0f);
                        flyby_played = 1;
                    }
                }
            }
        }

        
        if ((((Projectile *)tag)->projectile_flags & 1) &&
            (velocity->i != 0.0f || velocity->j != 0.0f || velocity->k != 0.0f)) {
            real_vector3d direction = *velocity;

            if (vector3d_normalize_with_length(&direction) > 0.0f) {
                real_vector3d side;

                *forward = direction;
                vector3d_cross_product(&side, forward, up);
                vector3d_cross_product(up, &side, forward);
                if (vector3d_normalize_with_length(up) == 0.0f) {
                    vector3d_build_perpendicular(up, forward);
                    vector3d_normalize_with_length(up);
                }
            }
            vector3d_rotate_about_axis(up, forward, F(obj, 0x270), F(obj, 0x274));
        } else if (((projectile_object *)obj)->projectile.flags & 1) {
            real_vector3d *axis = (real_vector3d *)(obj + 0x264);
            real_vector3d side;

            vector3d_rotate_about_axis(forward, axis, F(obj, 0x270), F(obj, 0x274));
            vector3d_rotate_about_axis(up, axis, F(obj, 0x270), F(obj, 0x274));
            vector3d_normalize_with_length(forward);
            vector3d_cross_product(&side, forward, up);
            vector3d_cross_product(up, &side, forward);
            vector3d_normalize_with_length(up);
        }

        
        object_unlink_cluster_or_notify_parent(projectile_index);
        ((projectile_object *)obj)->base.position = swept;
        object_set_cluster_and_parent(projectile_index, &hit.leaf);
        *velocity = vel;
        if (remaining != 0.0f && collisions != 0 && ((projectile_object *)obj)->projectile.contrail_attachment_index != -1 &&
            *(datum_index *)(obj + 0x14c + ((projectile_object *)obj)->projectile.contrail_attachment_index * 4) != k_datum_index_none) {
            object_recalculate_bounding_radius(projectile_index);
            contrail_advance(*(datum_index *)(obj + 0x14c + ((projectile_object *)obj)->projectile.contrail_attachment_index * 4), 0,
                             (1.0f - remaining) * 0.033333335f);
        }
    next_step:
        if (!(remaining > 0.0f)) {
            break;
        }
    }

    
    switch (((projectile_object *)obj)->projectile.state) {
    case 1:
        if (F(obj, 0x24c) != 0.0f && F(obj, 0x248) < 1.0f) {
            return 1;
        }
        if (((projectile_object *)obj)->base.network_role == 1) {
            return 1;
        }
        if (((projectile_object *)obj)->base.network_role == 0 && obj[0x278] == 1) {
            projectile_send_detonation(projectile_index);
        }
        projectile_detonate(projectile_index, (char)(collisions == 0), remaining);
        
    case 2: {
        int32_t role = *(int32_t *)(OBJECT_DATA(projectile_index) + 0x4);

        if (role == 0) {
            object_delete_unparented(projectile_index);
            object_delete_recursive(projectile_index, 0);
        } else if (role == 3) {
            object_delete_recursive(projectile_index, 0);
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
}

extern "C" {

int projectile_update(uint32_t projectile_index)
{
    return halo::projectiles::ProjectileHandle(projectile_index).update();
}

}
