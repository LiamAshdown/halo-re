#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/projectiles/projectile.hpp"
#include "halo/projectiles/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern real_vector3d *global_down3d_pointer;
extern real_point3d *global_origin3d_pointer;
extern char *projectile_effect_coordinate_system_names[5];
extern ProjectileMaterialResponse projectile_default_material_response;
extern int16_t network_game_mode;
extern datum_index effect_new_on_object_with_node_table(datum_index creator_object_index, datum_index definition_index, datum_index object_index, uint16_t node_index, uint16_t ctx_08, uint32_t ctx_0c, uint32_t ctx_10, uint32_t ctx_14, real a_scale, real b_scale, const void *color, const void *tint_source);
extern void effect_new_with_color(uint32_t effect, uint32_t target_or_index, void *velocity, int32_t kind, char **labels, void *position_block, void *direction_block, real fade_in, real fade_out, int32_t color, int32_t tint_source, int32_t force_create);
extern void breakable_surface_apply_damage(damage_data *request, uint32_t packed_leaf_and_flags, int32_t surface_index);
}

namespace halo::projectiles {

/**
 * The whole ProjectileResponse state machine: applies object damage on a direct hit, looks up
 * (or falls back to the default) ProjectileMaterialResponse row for the surface/object
 * material, rerolls it against the row's "potential" override thresholds, updates the
 * projectile's velocity and out_position for whichever response (disappear / detonate /
 * reflect / overpenetrate / attach) was chosen, spawns the row's default and potential-
 * material effects, and -- for an attach onto another object -- runs the same super-combining
 * sibling sweep and network broadcast.
 *
 * Register convention in the original: object index on the stack (param_1), collision_result*
 * on the stack (param_2), the in/out predicted-position real_point3d* on the stack (param_3),
 * the in/out velocity real_vector3d* in EAX (in_EAX).
 *
 * @address 0x4bf390
 */
void ProjectileHandle::response(collision_result *hit, real_point3d *out_position, real_vector3d *velocity)
{
    datum_index projectile_index = (datum_index)handle;

    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(projectile_index)].data;
    Projectile *tag = (Projectile *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    projectile_data *pd = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);

    int16_t new_material_index = hit->material_type; 
        
    real speed_fraction = 1.0f; 
        
        
    real effect_scale = 1.0f;   
        
        
    real fade_out = 0.0f;       
    real_vector3d unit_velocity = *velocity;
    ProjectileMaterialResponse *response;
    ProjectileResponse response_type;
    uint32_t response_effect_tag; 
        
        
        
    real angle_score, alignment_score;
    real impact_speed; 

    
    
    impact_speed = halo::math::vector3d_normalize_with_length(unit_velocity);
    if (0.0f == impact_speed) {
        unit_velocity = *halo::math::globals().global_up3d_pointer;
    }
    if (tag->final_velocity == tag->initial_velocity) {
        speed_fraction = 1.0f;
    } else {
        speed_fraction = (impact_speed - tag->final_velocity) / (tag->initial_velocity - tag->final_velocity);
        if (speed_fraction < 0.0f) {
            speed_fraction = 0.0f;
        } else if (1.0f < speed_fraction) {
            speed_fraction = 1.0f;
        }
    }

    if (hit->type == _collision_result_type_object && *(int32_t *)&tag->impact_damage.tag_id != -1) {
        damage_data dd;
        uint8_t *zero = (uint8_t *)&dd;
        int32_t i;
        for (i = 0; i < (int32_t)sizeof(dd); i++) {
            zero[i] = 0;
        }
        dd.flags |= 0x08; 
        
        
        dd.location_cluster_index = -1;
        dd.material_type = -1;
        dd.responsible_player = obj->owner_linkage;
        dd.responsible_object = (datum_index)obj->creator_object;
        dd.team_index = (int16_t)obj->owner_team;
        dd.epicentre = hit->point;
        dd.origin = hit->point;
        dd.direction = *velocity;
        dd.random_blend = speed_fraction;
        dd.multiplier = 1.0f;
        dd.damage_effect_tag = *(datum_index *)&tag->impact_damage.tag_id;

        halo::math::vector3d_normalize_with_length(dd.direction);
        halo::objects::object_apply_damage(&dd, hit->object_index, hit->node_index, hit->region_index, hit->collision_material_index, (uint32_t)&hit->plane.normal);

        
        
        
        if (dd.material_type != -1) {
            new_material_index = dd.material_type;
        }
        
        fade_out = *(real *)&dd.remaining_vitality;
    }

    pd->material_response_index = new_material_index;
    if (new_material_index < 0 || tag->projectile_material_response.count <= (uint32_t)new_material_index) {
        response = &projectile_default_material_response;
    } else {
        response = (ProjectileMaterialResponse *)tag->projectile_material_response.pointer + new_material_index;
    }

    
    
    
    {
        uint32_t seed_step = advance_random_seed(halo::math::globals().random_seed_global);
        real angular_noise = response->angular_noise;
        halo::math::globals().random_seed_global = advance_random_seed(seed_step);
        
        
        
        
        alignment_score = ((((real)(seed_step >> k_random_high_shift) * halo::k_unit_word_scale) * response->velocity_noise +
            -response->velocity_noise) - hit->plane.normal.k * velocity->k) - hit->plane.normal.j * velocity->j -
            hit->plane.normal.i * velocity->i;
        
        angle_score = ((angular_noise - -angular_noise) * ((real)((halo::math::globals().random_seed_global >> k_random_high_shift) & halo::k_slot_mask) * halo::k_unit_word_scale) +
            -angular_noise) + (halo::math::vector3d_angle_between_4cd4f0(*(real_vector3d *)&hit->plane.normal, *velocity) - 1.5707964f);
        
    }

    if (response->potential_response == 0 ||
        
        
        (response->potential_between[1] != 0.0f &&
         (angle_score < response->potential_between[0] ||
          angle_score > response->potential_between[1])) ||
        (response->potential_and[1] != 0.0f &&
         (alignment_score < response->potential_and[0] ||
          alignment_score > response->potential_and[1])) ||
        ((response->potential_flags & 1) != 0 && 
         (hit->type != _collision_result_type_object || halo::objects::object_try_and_get(hit->object_index, _object_mask_unit) == 0)) ||
        ((real)((halo::math::globals().random_seed_global = advance_random_seed(halo::math::globals().random_seed_global), halo::math::globals().random_seed_global) >> k_random_high_shift) *
             halo::k_unit_word_scale < response->potential_skip_fraction)) {
        response_type = (ProjectileResponse)response->default_response;
        response_effect_tag = *(uint32_t *)&response->default_effect.tag_id;
    } else {
        response_type = (ProjectileResponse)response->potential_response;
        response_effect_tag = *(uint32_t *)&response->potential_effect.tag_id;
    }

    if (hit->type == _collision_result_type_structure && (hit->surface_flags & 0x08) != 0) {
        
        
        
        damage_data breakable_surface_damage;
        ProjectileMaterialResponse *surface_response;
        uint8_t *zero = (uint8_t *)&breakable_surface_damage;
        int32_t i;
        for (i = 0; i < (int32_t)sizeof(breakable_surface_damage); i++) {
            zero[i] = 0;
        }
        breakable_surface_damage.damage_effect_tag = *(datum_index *)&tag->impact_damage.tag_id;
        breakable_surface_damage.flags |= 0x08;
        breakable_surface_damage.responsible_player = (datum_index)k_datum_index_none;
        breakable_surface_damage.responsible_object = (datum_index)k_datum_index_none;
        breakable_surface_damage.team_index = -1;
        breakable_surface_damage.location_cluster_index = -1;
        breakable_surface_damage.material_type = -1;
        breakable_surface_damage.epicentre = hit->point;
        breakable_surface_damage.origin = hit->point;
        breakable_surface_damage.direction = *velocity;
        breakable_surface_damage.random_blend = 1.0f;
        breakable_surface_damage.multiplier = 1.0f;
        halo::math::vector3d_normalize_with_length(breakable_surface_damage.direction);

        
        
        breakable_surface_damage.material_type = hit->material_type;
        if (hit->material_type < 0 ||
            tag->projectile_material_response.count <= (uint32_t)hit->material_type) {
            surface_response = &projectile_default_material_response;
        } else {
            surface_response = (ProjectileMaterialResponse *)tag->projectile_material_response.pointer +
                               hit->material_type;
        }
        breakable_surface_damage.material_response = (uint32_t)surface_response;

        
        breakable_surface_damage.location_leaf_index = *(int32_t *)&hit->leaf;
        *(uint32_t *)&breakable_surface_damage.location_cluster_index =
            *(uint32_t *)((uint8_t *)&hit->leaf + 4);

        halo::physics::breakable_surface_apply_damage(&breakable_surface_damage,
            (*(uint32_t *)&hit->leaf & 0xffff0000u) | (uint32_t)hit->breakable_surface_index,
            hit->surface_index);
    }

    *out_position = hit->point;

    if (response_type == projectileresponse_overpenetrate) {
        if (hit->type == _collision_result_type_water_surface) {
            obj->flags ^= _object_in_water_bit;
            projectile_compute_deceleration(projectile_index);
            out_position->x -= hit->plane.normal.i * 0.001f;
            out_position->y -= hit->plane.normal.j * 0.001f;
            out_position->z -= hit->plane.normal.k * 0.001f;
        } else if (hit->type != _collision_result_type_object) {
            if (tag->timer[1] == 0.0f) {
                response_type = projectileresponse_detonate;
            } else {
                
                pd->flags |= _projectile_hit_ground_bit | _projectile_at_rest_bit;
                response_type = projectileresponse_attach;
            }
            goto fall_back_to_up_vector;
        } else {
            real remaining = 1.0f - response->initial_friction;
            velocity->i *= remaining;
            velocity->j *= remaining;
            velocity->k *= remaining;
            pd->ignore_object_index = hit->object_index;
        }
    } else if (response_type == projectileresponse_reflect) {
        
        
        
        
        real_vector3d parallel_component, perpendicular_component;
        halo::math::vector3d_project_onto_axis(parallel_component, hit->plane.normal, *velocity,
                                   perpendicular_component);
        velocity->i = (1.0f - response->perpendicular_friction) * perpendicular_component.i -
            (1.0f - response->parallel_friction) * parallel_component.i;
        velocity->j = (1.0f - response->perpendicular_friction) * perpendicular_component.j -
            (1.0f - response->parallel_friction) * parallel_component.j;
        velocity->k = (1.0f - response->perpendicular_friction) * perpendicular_component.k -
            (1.0f - response->parallel_friction) * parallel_component.k;
    } else {
    fall_back_to_up_vector: 
        *velocity = *(real_vector3d *)global_origin3d_pointer;
    }

    if (response->angular_noise != 0.0f) {
        
        
        halo::math::vector3d_randomize_direction(*(real_point3d *)velocity, velocity, halo::math::globals().random_seed_global, 0.0f,
            response->angular_noise);
    }
    {
        
        
        
        
        real pre_length;
        if (response->velocity_noise != 0.0f &&
            (pre_length = halo::math::vector3d_normalize_with_length(*velocity)) != 0.0f) {
            real scale = (real)((halo::math::globals().random_seed_global = advance_random_seed(halo::math::globals().random_seed_global), halo::math::globals().random_seed_global) >> k_random_high_shift) *
                halo::k_unit_word_scale * (response->velocity_noise - -response->velocity_noise) +
                -response->velocity_noise + pre_length;
            velocity->i *= scale;
            velocity->j *= scale;
            velocity->k *= scale;
        }
    }

    {
        real speed_sq = (velocity->i * velocity->i + velocity->j * velocity->j) + velocity->k * velocity->k;
        if (response_type != projectileresponse_attach && speed_sq < tag->minimum_velocity * tag->minimum_velocity) {
            projectile_request_state(projectile_index, _projectile_state_detonating);
        }
        if (speed_sq < 0.0001f) {
            pd->flags |= _projectile_at_rest_bit;
            if (0.3f < hit->plane.normal.k) {
                obj->flags |= _object_at_rest_bit;
                if (tag->timer[1] == 0.0f) {
                    projectile_request_state(projectile_index, _projectile_state_detonating);
                }
            }
        }
    }

    
    
    
    
    if (response->scale_effects_by == 0) {
        effect_scale = speed_fraction;
        if (effect_scale < 0.0f) {
            effect_scale = 0.0f;
        } else if (1.0f < effect_scale) {
            effect_scale = 1.0f;
        }
    } else if (response->scale_effects_by == 1) {
        effect_scale = angle_score * 0.63661975f; 
        if (effect_scale < 0.0f) {
            effect_scale = 0.0f;
        } else if (1.0f < effect_scale) {
            effect_scale = 1.0f;
        }
    }
    if (fade_out < 0.0f) {
        fade_out = 0.0f;
    } else if (1.0f < fade_out) {
        fade_out = 1.0f;
    }

    {
        real_vector3d reflected;
        real dot2 = 2.0f * ((unit_velocity.k * hit->plane.normal.k + unit_velocity.j * hit->plane.normal.j) +
            unit_velocity.i * hit->plane.normal.i);
        reflected.i = unit_velocity.i - dot2 * hit->plane.normal.i;
        reflected.j = unit_velocity.j - dot2 * hit->plane.normal.j;
        reflected.k = unit_velocity.k - dot2 * hit->plane.normal.k;

        real_vector3d coordinate_system[5]; 
        real_point3d positions[5];
        int32_t i;
        coordinate_system[0] = hit->plane.normal;
        coordinate_system[1].i = -unit_velocity.i;
        coordinate_system[1].j = -unit_velocity.j;
        coordinate_system[1].k = -unit_velocity.k;
        coordinate_system[2] = unit_velocity;
        coordinate_system[3] = reflected;
        coordinate_system[4] = *global_down3d_pointer;
        for (i = 0; i < 5; i++) {
            positions[i] = hit->point;
        }

        if (0.008333334f < alignment_score) {
            if (hit->type == _collision_result_type_object) {
                
                
                
                halo::effects::effect_new_on_object_with_node_table(projectile_index, response_effect_tag, hit->object_index,
                    (uint16_t)hit->node_index, 5, (uint32_t)projectile_effect_coordinate_system_names, (uint32_t)positions,
                    (uint32_t)coordinate_system, effect_scale, fade_out, 0, 0);
            } else {
                halo::effects::effect_new_with_color(response_effect_tag, projectile_index, 0, 5, (uint32_t)projectile_effect_coordinate_system_names, positions, (uint32_t)coordinate_system, effect_scale, fade_out, 0, 0, 1);
            }
        }
        
        if ((pd->flags & _projectile_detonation_timer_started_bit) == 0 &&
            ((pd->flags & _projectile_at_rest_bit) != 0 || response_type == projectileresponse_attach)) {
            if (hit->type == _collision_result_type_object) {
                
                halo::effects::effect_new_on_object_with_node_table(projectile_index, *(uint32_t *)&tag->detonation_started.tag_id,
                    hit->object_index, (uint16_t)hit->node_index, 5, (uint32_t)projectile_effect_coordinate_system_names,
                    (uint32_t)positions, (uint32_t)coordinate_system, effect_scale, fade_out, 0, 0);
            } else {
                halo::effects::effect_new_with_color(*(uint32_t *)&tag->detonation_started.tag_id, projectile_index, 0, 5, (uint32_t)projectile_effect_coordinate_system_names, positions, (uint32_t)coordinate_system, effect_scale, fade_out, 0, 0, 1);
            }
        }
    }

    if (response_type == projectileresponse_disappear) {
        projectile_request_state(projectile_index, _projectile_state_disappearing);
        return;
    }
    if (response_type == projectileresponse_detonate) {
        projectile_request_state(projectile_index, _projectile_state_detonating);
        return;
    }
    if (response_type != projectileresponse_attach) {
        return;
    }

    if (hit->type == _collision_result_type_object) {
        object *target = halo::objects::object_try_and_get(hit->object_index, _object_mask_all);
        if (network_game_mode != 0 && target != 0 && target->type == _object_type_biped &&
            (target->vitality_flags & _object_health_frozen_bit) != 0) {
            return;
        }
        if (obj->network_role == 1) {
            return;
        }
        if ((tag->projectile_flags & _projectile_definition_has_super_combining_explosion_bit) != 0) {
            object *parent = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(hit->object_index)].data;
            datum_index sibling_index = parent->first_child_object;
            int16_t sibling_count = 0;
            while (sibling_index != (datum_index)k_datum_index_none) {
                object *sibling = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(sibling_index)].data;
                projectile_data *sibling_pd = (projectile_data *)((uint8_t *)sibling + k_projectile_data_offset);
                if (sibling->definition_tag == obj->definition_tag &&
                    (sibling_pd->flags & _projectile_super_detonation_counted_bit) == 0) {
                    sibling_pd->arming_timer = 0.0f;
                    sibling_pd->detonation_timer = 0.0f;
                    sibling_count++;
                }
                if (k_projectile_super_combine_attach_threshold < sibling_count) {
                    pd->flags |= _projectile_super_detonation_bit;
                    break;
                }
                sibling_index = sibling->next_object;
            }
        }
    }

    velocity->i = 0.0f;
    velocity->j = 0.0f;
    velocity->k = 0.0f;
    obj->angular_velocity.i = 0.0f;
    obj->angular_velocity.j = 0.0f;
    obj->angular_velocity.k = 0.0f;
    pd->flags |= _projectile_attached_bit;
    obj->flags |= _object_at_rest_bit;

    halo::objects::object_unlink_cluster_or_notify_parent(projectile_index);
    obj->position = *out_position;
    halo::objects::object_set_cluster_and_parent(projectile_index, &hit->leaf);
    if (hit->type == _collision_result_type_object) {
        halo::objects::object_attach_to_object(hit->object_index, projectile_index, hit->node_index);
    }

    if ((tag->projectile_flags & _projectile_definition_detonation_max_time_if_attached_bit) != 0) {
        real t = tag->timer[1];
        if (1.0f <= t * 30.0f) {
            pd->detonation_timer_rate = 1.0f / (t * 30.0f);
        }
    } else if ((tag->projectile_flags & _projectile_definition_random_attached_detonation_time_bit) != 0) {
        real t = (real)((halo::math::globals().random_seed_global = advance_random_seed(halo::math::globals().random_seed_global), halo::math::globals().random_seed_global) >> k_random_high_shift) *
            halo::k_unit_word_scale * (tag->timer[1] - tag->timer[0]) + tag->timer[0];
        if (1.0f <= t * 30.0f) {
            pd->detonation_timer_rate = 1.0f / (t * 30.0f);
        }
    }

    if (hit->type != _collision_result_type_object) {
        return;
    }
    if (obj->network_role != 0) {
        return;
    }
    if (((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(hit->object_index)].data->network_role != 0) {
        return;
    }
    projectile_send_attach(projectile_index, hit->object_index, hit->node_index);
    obj->flags |= _object_changed_bit; 
}

}

namespace halo::projectiles {

void projectile_response(datum_index projectile_index, collision_result *hit, real_point3d *out_position, real_vector3d *velocity)
{
    halo::projectiles::ProjectileHandle(projectile_index).response(hit, out_position, velocity);
}

}
