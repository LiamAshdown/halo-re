#include "halo/projectiles/projectile.hpp"
#include "halo/core/datum.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern data_array *object_data;
extern ScenarioStructureBSP *global_structure_bsp;
extern int16_t network_game_mode;
extern int16_t scenario_location_fog_region(bsp_leaf_reference *leaf, real_point3d *point);
extern void projectile_compute_rotation(uint32_t object_index);
extern void projectile_update_function_values(uint32_t object_index);
extern void projectile_compute_deceleration(uint32_t object_index);
extern real projectile_deceleration_from_range(Projectile *tag, real r0, real r1);
extern double sqrt(double x);
extern double fsin(double x);
extern double fcos(double x);
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object, void *scratch);
extern uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target, uint32_t collision_mask, uint32_t ignore_object_index, void *out_record);
extern char k_empty_string[1];
extern game_engine_definition *current_game_engine;
extern ProjectileMaterialResponse projectile_default_material_response;
extern real_vector3d *global_down3d_pointer;
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up);
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index, bsp_leaf_reference *location);
extern void object_recalculate_bounding_radius(uint32_t object_index);
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index);
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index, int16_t region_index, int16_t material_index, uint32_t plane);
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);
extern uint8_t object_reposition_to_spawn_location(uint32_t object_index, real_point3d *target_position, uint32_t ignore_object_index);
extern void contrail_advance(datum_index contrail_handle, uint8_t detach, real delta_time);
extern void ai_accumulate_repeated_event(datum_index object_index, real_point3d *origin, int32_t kind, ObjectNoise_t noise, int32_t param_5);
extern void effect_new_with_color(uint32_t effect, uint32_t target_or_index, void *velocity, int32_t kind, char **labels, void *position_block, void *direction_block, real fade_in, real fade_out, int32_t color, int32_t tint_source, int32_t force_create);
extern game_time_globals *game_time;
extern int32_t k_projectile_minimum_age_ticks;
uint8_t projectile_new(uint32_t object_index);
uint8_t projectile_collision_test(uint32_t object_index, real_point3d *target, void *out_record);
void projectile_detonate(uint32_t object_index, char first_collision, real remaining_tick_fraction);
uint8_t projectile_force_detonate(uint32_t object_index);
uint8_t projectile_is_old_enough(uint32_t object_index);
void projectile_notify_object_deleted(uint32_t object_index, datum_index dying_object_index);
uint8_t object_type_definition_return_false(void);
uint8_t object_type_definition_return_true(void);
}

namespace halo::projectiles {

/**
 * The projectile row's query_create hook (object_type_definition +0x28). Establishes a freshly
 * created projectile's initial state: zeroes/resets flags, state, material_response_index,
 * resolves ignore_object_index from the firing object's own root parent, finds its contrail
 * attachment index, computes the detonation and arming timer rates from the tag, applies the
 * tag's initial_velocity along the object's own forward vector, probes whether it starts
 * inside water fog, runs the rotation/function-value/deceleration setup passes, and clears the
 * network.
 *
 * Register convention in the original: object index is a plain stack cdecl parameter, matching
 * the rest of this directly-indexed (non object_try_and_get) family.
 *
 * @address 0x4bd7c0
 */
uint8_t ProjectileHandle::construct()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Projectile *tag = (Projectile *)halo::cache::globals().tag_instances[(uint16_t)obj->definition_tag].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    float rate;
    datum_index root;
    int16_t i;
    int16_t fog_region;
    uint8_t in_water;

    obj->flags |= to_bits(projectile_object_flag::unknown_2000); 

    proj->flags = _projectile_tracer_bit;
    proj->thrown_grenade = 0;
    proj->tracked_object_index = (datum_index)k_datum_index_none;
    proj->state = 0;
    proj->material_response_index = -1;

    
    
    root = (datum_index)k_datum_index_none;
    if (obj->creator_object != (uint32_t)k_datum_index_none) {
        datum_index cursor = (datum_index)obj->creator_object;
        do {
            root = cursor;
            cursor = ((object_header *)object_data->data)[halo::datum_slot(cursor)].data->parent_object;
        } while (cursor != (datum_index)k_datum_index_none);
    }
    proj->ignore_object_index = root;

    
    
    
    if ((tag->projectile_flags & to_bits(projectile_definition_flag::detonation_max_time_if_attached)) == 0) {
        if ((tag->projectile_flags & to_bits(projectile_definition_flag::minimum_unattached_detonation_time)) == 0) {
            halo::math::globals().random_seed_global = advance_random_seed(halo::math::globals().random_seed_global);
            rate = ((tag->timer[1] - tag->timer[0]) * (real)(halo::math::globals().random_seed_global >> k_random_high_shift) * 1.5259022e-05f +
                    tag->timer[0]) * 30.0f;
        } else {
            rate = tag->timer[0] * 30.0f;
        }
    } else {
        rate = tag->timer[0] * 30.0f;
    }
    if (!(rate < 1.0f)) { 
        proj->detonation_timer_rate = 1.0f / rate;
    }

    rate = tag->arming_time * 30.0f;
    if (!(rate < 1.0f)) {
        proj->arming_timer_rate = 1.0f / rate;
    }

    
    proj->contrail_attachment_index = -1;
    for (i = 0; i < (int32_t)tag->base.attachments.count; i++) {
        ObjectAttachment *attachment = &((ObjectAttachment *)tag->base.attachments.pointer)[i];
        if (attachment->type.tag_fourcc == k_contrail_group_tag) { 
            proj->contrail_attachment_index = i;
            break;
        }
    }

    
    obj->velocity.i = tag->initial_velocity * obj->forward.i + obj->velocity.i;
    obj->velocity.j = tag->initial_velocity * obj->forward.j + obj->velocity.j;
    obj->velocity.k = tag->initial_velocity * obj->forward.k + obj->velocity.k;

    
    in_water = 0;
    fog_region = scenario_location_fog_region((bsp_leaf_reference *)&obj->location_leaf_index,
                                               &obj->bounding_center);
    if (fog_region != -1) {
        ScenarioStructureBSPFogRegion *region =
            &((ScenarioStructureBSPFogRegion *)global_structure_bsp->fog_regions.pointer)[fog_region];
        if (region->fog != (uint16_t)-1) {
            ScenarioStructureBSPFogPalette *fog_entry =
                &((ScenarioStructureBSPFogPalette *)global_structure_bsp->fog_palette.pointer)[region->fog];
            datum_index fog_tag_id = *(datum_index *)&fog_entry->fog.tag_id;
            if (fog_tag_id != (datum_index)k_datum_index_none) {
                Fog *fog_tag = (Fog *)halo::cache::globals().tag_instances[halo::datum_slot(fog_tag_id)].data;
                in_water = (*(uint8_t *)fog_tag & 1) != 0;
            }
        }
    }
    if (in_water) {
        obj->flags |= _object_in_water_bit;
    } else {
        obj->flags &= ~(uint32_t)_object_in_water_bit;
    }

    projectile_compute_rotation(object_index);
    projectile_update_function_values(object_index);
    projectile_compute_deceleration(object_index);

    obj->flags |= _object_definition_flag0_bit | _object_connected_to_map_bit;

    if (network_game_mode == 1 || network_game_mode == 2) {
        proj->network_state_valid = 0;
        proj->network_baseline_index = 0;
        proj->network_sequence = 0;
        obj->network_state_009 = 0; 
    }

    return 1;
}

/**
 * FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or
 * only saves it); those parameters arrive on the stack (1 stack argument(s) read). blam-cc:
 * stack -> object_index
 *
 * @address 0x4c0250
 */
void ProjectileHandle::update_function_values()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Projectile *tag = (Projectile *)halo::cache::globals().tag_instances[(uint16_t)obj->definition_tag].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    ProjectileFunctionIn_t *function_in = &tag->projectile_a_in; 
    float *out = obj->function_in_values;
    int32_t i;

    for (i = 0; i < 4; i++) {
        ProjectileFunctionIn_t kind = function_in[i];
        float value = 0.0f; 
                             
                             

        if (kind == projectilefunctionin_none) {
            continue; 
        }
        if (kind == projectilefunctionin_range_remaining) {
            if (tag->maximum_range != 0.0f) {
                value = proj->distance_travelled / tag->maximum_range;
            }
        } else if (kind == projectilefunctionin_time_remaining) {
            value = proj->detonation_timer;
        } else if (kind == projectilefunctionin_tracer) {
            if ((proj->flags & _projectile_tracer_bit) != 0) {
                value = 1.0f;
            }
        }
        out[i] = value;
    }
}

/**
 * Original function projectile_compute_deceleration; the author notes are in
 * docs/original/projectiles/projectile_compute_deceleration.c.txt.
 *
 * Register convention in the original: object index in EAX (in_EAX).
 *
 * @address 0x4c0310
 */
void ProjectileHandle::compute_deceleration()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Projectile *tag = (Projectile *)halo::cache::globals().tag_instances[(uint16_t)obj->definition_tag].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    real near_range;

    if ((obj->flags & _object_in_water_bit) == 0) {
        near_range = tag->air_damage_range[0];
        proj->deceleration = projectile_deceleration_from_range(tag, tag->air_damage_range[0], tag->air_damage_range[1]);
    } else {
        near_range = tag->water_damage_range[0];
        proj->deceleration = projectile_deceleration_from_range(tag, tag->water_damage_range[0], tag->water_damage_range[1]);
    }
    proj->deceleration_end_range = tag->water_damage_range[1]; 

    if (near_range > 0.0f) {
        proj->deceleration_delay_rate = near_range / tag->initial_velocity;
        return;
    }
    proj->deceleration_delay = 1.0f;
    proj->deceleration_delay_rate = 0.0f;
}

/**
 * Recomputes a projectile's rotation_axis / rotation_sine / rotation_cosine from its current
 * angular velocity. Unlike item_compute_rotation, this one always refreshes the axis when the
 * angular velocity is nonzero, regardless of any "at rest" flag.
 *
 * Register convention in the original: object index in EAX (in_EAX).
 *
 * @address 0x4c0180
 */
void ProjectileHandle::compute_rotation()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    real magnitude = (real)sqrt((double)obj->angular_velocity.k * (double)obj->angular_velocity.k +
                                 (double)obj->angular_velocity.j * (double)obj->angular_velocity.j +
                                 (double)obj->angular_velocity.i * (double)obj->angular_velocity.i);

    if (magnitude != 0.0f) {
        real inverse = 1.0f / magnitude;
        proj->flags |= _projectile_rotation_valid_bit;
        proj->rotation_axis.i = inverse * obj->angular_velocity.i;
        proj->rotation_axis.j = inverse * obj->angular_velocity.j;
        proj->rotation_axis.k = inverse * obj->angular_velocity.k;
        proj->rotation_sine = (real)fsin((double)magnitude);
        proj->rotation_cosine = (real)fcos((double)magnitude);
        return;
    }
    proj->flags &= ~_projectile_rotation_valid_bit;
    proj->rotation_sine = 0.0f;
    proj->rotation_cosine = 1.0f;
}

/**
 * Original function projectile_collision_test; the author notes are in
 * docs/original/projectiles/projectile_collision_test.c.txt.
 *
 * Register convention in the original: object index in EAX (in_EAX); the swept-to point
 * (real_point3d *) in EDI (unaff_EDI); the caller's collision_result output buffer is Ghidra's
 * one recognized stack parameter (param_1).
 *
 * @address 0x4c0450
 */
uint8_t ProjectileHandle::collision_test(real_point3d *target, void *out_record)
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Projectile *tag = (Projectile *)halo::cache::globals().tag_instances[(uint16_t)obj->definition_tag].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    real_vector3d sweep_delta;
    uint8_t hit;

    sweep_delta.i = target->x - obj->position.x;
    sweep_delta.j = target->y - obj->position.y;
    sweep_delta.k = target->z - obj->position.z;

    hit = collision_test_movement_segment(k_projectile_collision_mask_point, &obj->position, &sweep_delta,
                        (uint32_t)proj->ignore_object_index, out_record);
    if (hit != 0) {
        return 1;
    }
    if (tag->collision_radius < 0.0001f) {
        return 0;
    }

    {
        real_vector3d direction, perpendicular;
        real_point3d plus_origin, minus_origin, minus_target;
        real_vector3d plus_delta;
        real radius;

        direction.i = target->x - obj->position.x;
        direction.j = target->y - obj->position.y;
        direction.k = target->z - obj->position.z;

        halo::math::vector3d_cross_product(perpendicular, direction, *halo::math::globals().global_up3d_pointer); 
        if (halo::math::vector3d_normalize_with_length(perpendicular) == 0.0f) {
            perpendicular = *halo::math::globals().global_left3d_pointer;
        }

        radius = tag->collision_radius;
        plus_origin.x = perpendicular.i * radius + obj->position.x;
        plus_origin.y = perpendicular.j * radius + obj->position.y;
        plus_origin.z = perpendicular.k * radius + obj->position.z;
        plus_delta.i = (perpendicular.i * radius + target->x) - plus_origin.x;
        plus_delta.j = (perpendicular.j * radius + target->y) - plus_origin.y;
        plus_delta.k = (radius * perpendicular.k + target->z) - plus_origin.z;

        radius = -tag->collision_radius;
        minus_origin.x = perpendicular.i * radius + obj->position.x;
        minus_origin.y = perpendicular.j * radius + obj->position.y;
        minus_origin.z = perpendicular.k * radius + obj->position.z;
        
        
        minus_target.x = perpendicular.i * radius + target->x;
        minus_target.y = perpendicular.j * radius + target->y;
        minus_target.z = perpendicular.k * radius + target->z;

        hit = collision_test_movement_segment(k_projectile_collision_mask_radius, &plus_origin, &plus_delta,
                            (uint32_t)proj->ignore_object_index, out_record);
        if (hit == 0) {
            hit = collision_test_movement_segment_between_points(&minus_origin, &minus_target, k_projectile_collision_mask_radius, 
                                   (uint32_t)proj->ignore_object_index, out_record);
            if (hit == 0) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * Runs the ProjectileResponse "detonate" side effect: an optional super-combining explosion
 * sweep across sibling projectiles attached to the same (biped) parent, a contrail/attachment
 * bookkeeping step on the first collision of the tick, the primary detonation effect spawn,
 * attached_detonation_damage applied to the parent object, and a second effect spawn keyed by
 * the last-hit material's ProjectileMaterialResponse.
 *
 * Register convention in the original: object index in EBX (unaff_EBX); the two remaining
 * arguments are Ghidra-recognized stack parameters.
 *
 * @address 0x4c0670
 */
void ProjectileHandle::detonate(char first_collision, real remaining_tick_fraction)
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Projectile *tag = (Projectile *)halo::cache::globals().tag_instances[(uint16_t)obj->definition_tag].data;
    char *effect_names[2];
    datum_index effect_tag_id;
    real_point3d position_block[2];   
    real_point3d relink_position;      
    real_vector3d direction_block[2]; 
                                       
                                       

    effect_names[0] = k_empty_string;
    effect_names[1] = (char *)"gravity";
    effect_tag_id = *(datum_index *)&tag->effect.tag_id;

    if ((tag->projectile_flags & _projectile_definition_has_super_combining_explosion_bit) != 0 &&
        (((projectile_data *)((uint8_t *)obj + k_projectile_data_offset))->flags &
         _projectile_super_detonation_counted_bit) == 0 &&
        obj->parent_object != (datum_index)k_datum_index_none) {

        object *parent = ((object_header *)object_data->data)[halo::datum_slot(obj->parent_object)].data;
        datum_index first_child = parent->first_child_object;
        datum_index cursor;
        int16_t sibling_count = 0;

        cursor = first_child;
        while (cursor != (datum_index)k_datum_index_none) {
            object *sibling = ((object_header *)object_data->data)[halo::datum_slot(cursor)].data;
            projectile_data *sibling_proj = (projectile_data *)((uint8_t *)sibling + k_projectile_data_offset);
            if (sibling->definition_tag == obj->definition_tag &&
                (sibling_proj->flags & _projectile_super_detonation_counted_bit) == 0) {
                sibling_count = sibling_count + 1;
            }
            cursor = sibling->next_object;
        }

        if (parent->type == _object_type_biped &&
            (((unit_data *)((uint8_t *)parent + k_unit_data_offset))->controlling_player == (datum_index)k_datum_index_none ||
             current_game_engine != 0) &&
            sibling_count > k_projectile_super_combine_detonate_threshold) {

            cursor = first_child;
            while (cursor != (datum_index)k_datum_index_none) {
                object *sibling = ((object_header *)object_data->data)[halo::datum_slot(cursor)].data;
                projectile_data *sibling_proj = (projectile_data *)((uint8_t *)sibling + k_projectile_data_offset);
                if (sibling->definition_tag == obj->definition_tag &&
                    (sibling_proj->flags & _projectile_super_detonation_counted_bit) == 0) {
                    if (sibling_count < k_projectile_super_combine_detonate_threshold + 1) {
                        sibling_proj->flags |= _projectile_super_detonation_counted_bit;
                        sibling_proj->detonation_timer = halo::math::random_real() * sibling_proj->detonation_timer;
                        sibling_proj->arming_timer = halo::math::random_real() * sibling_proj->arming_timer;
                    } else {
                        sibling_proj->detonation_timer = 0.0f;
                        sibling_proj->arming_timer = 0.0f;
                    }
                    sibling_count = sibling_count - 1;
                }
                cursor = sibling->next_object;
            }

            effect_tag_id = *(datum_index *)&tag->super_detonation.tag_id;

            
            
            
            
            real_point3d parent_position;   

            object_get_position(&parent_position, obj->parent_object);
            object_snap_to_parent_marker_and_detach(object_index);
            relink_position = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data->position;
            object_set_position_and_relink(&parent_position, object_index, 0);
            object_reposition_to_spawn_location(object_index, &relink_position, k_datum_index_none);
            object_recalculate_bounding_radius_recursive(object_index);
        }
    }

    {
        projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
        if (first_collision != 0 && proj->contrail_attachment_index != -1 &&
            obj->attachment_handles[proj->contrail_attachment_index] != (datum_index)k_datum_index_none) {
            object_recalculate_bounding_radius(object_index);
            
            contrail_advance(obj->attachment_handles[proj->contrail_attachment_index], 0,
                (1.0f - remaining_tick_fraction) * 0.033333335f);
        }
    }

    {
        real_vector3d forward_scratch; 

        object_get_position(&position_block[0], object_index); 
        object_get_orientation(&forward_scratch, object_index, &direction_block[0]);
        position_block[1] = position_block[0]; 
        direction_block[1] = *global_down3d_pointer;

        effect_new_with_color(effect_tag_id, obj->creator_object, 0, 2, effect_names, position_block,
                     direction_block, 0, 0, 0, 0, 1);
    }

    if (obj->parent_object != (datum_index)k_datum_index_none &&
        *(int32_t *)&tag->attached_detonation_damage.tag_id != -1) {
        damage_data dd;
        uint8_t *zero = (uint8_t *)&dd;
        int32_t i;

        for (i = 0; i < (int32_t)sizeof(dd); i++) {
            zero[i] = 0;
        }
        dd.flags |= 0x08;
        dd.responsible_player = (datum_index)k_datum_index_none;
        dd.responsible_object = (datum_index)k_datum_index_none;
        dd.team_index = -1;
        dd.location_cluster_index = -1;
        dd.material_type = -1; 
        dd.random_blend = 1.0f;
        dd.multiplier = 1.0f;
        dd.damage_effect_tag = *(datum_index *)&tag->attached_detonation_damage.tag_id;

        object_get_orientation(0, object_index, 0); 
        object_get_position(&dd.epicentre, object_index); 
        dd.origin = dd.epicentre;
        dd.responsible_object = obj->creator_object;   
        dd.responsible_player = obj->owner_linkage; 
        dd.team_index = (int16_t)obj->owner_team; 

        object_apply_damage(&dd, obj->parent_object, -1, -1, -1, 0);
    }

    {
        projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
        int16_t index = proj->material_response_index;

        if (index != -1) {
            ProjectileMaterialResponse *response;

            if (index < 0 || tag->projectile_material_response.count <= (uint32_t)index) {
                response = &projectile_default_material_response;
            } else {
                response = (ProjectileMaterialResponse *)tag->projectile_material_response.pointer + index;
            }
            effect_new_with_color(*(uint32_t *)&response->detonation_effect.tag_id, obj->creator_object, 0, 2,
                         effect_names, position_block, direction_block, 0, 0, 0, 0, 1);
        }
    }

    ai_accumulate_repeated_event(object_index, &position_block[0], 2, tag->detonation_noise, 1);
}

/**
 * The projectile row's +0x44 hook. Forces both timers to their completed value (1.0), clears
 * _projectile_attached_bit (it can no longer combine as an attached fragment once it is being
 * forced to detonate), and detaches it from whatever parent marker it was snapped to. Always
 * reports success.
 *
 * Register convention in the original: object index is a plain stack cdecl parameter, matching
 * the rest of this directly-indexed (non object_try_and_get) family.
 *
 * @address 0x4c0ac0
 */
uint8_t ProjectileHandle::force_detonate()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);

    proj->arming_timer = 1.0f;
    proj->detonation_timer = 1.0f;
    proj->flags &= ~(uint32_t)_projectile_attached_bit;
    object_snap_to_parent_marker_and_detach(object_index);

    return 1;
}

/**
 * The projectile row's "is old enough" hook (object_type_definition +0x74). An object that has
 * never been stamped (object.network_update_tick == -1) always counts as old enough; otherwise
 * it is old enough once the game tick has advanced past the stamped tick plus this type's
 * minimum age.
 *
 * Register convention in the original: object index is a plain stack cdecl parameter, matching
 * the rest of this directly-indexed (non object_try_and_get) family.
 *
 * @address 0x4c1270
 */
uint8_t ProjectileHandle::is_old_enough()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    int32_t stamp = obj->network_update_tick;

    if (stamp == -1) {
        return 1;
    }
    return stamp + k_projectile_minimum_age_ticks <= game_time->game_time;
}

/**
 * The projectile row's notify_3c hook (object_type_definition +0x3c), run for every object
 * when some other object dies (object_clear_references_to_object's sweep). If this projectile
 * was tracking the object that just died, forgets it.
 *
 * Register convention in the original: both arguments are Ghidra-recognized stack parameters;
 * confirmed against objdump -d -M intel bin/halo.exe (0x4bf0c0 mov eax,[esp+0x4] / cmp
 * ecx,[esp+0x8]) -- pure cdecl, matching object_clear_references_to_object's own call.
 *
 * @address 0x4bf0c0
 */
void ProjectileHandle::notify_object_deleted(datum_index dying_object_index)
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);

    if (proj->tracked_object_index == dying_object_index) {
        proj->tracked_object_index = (datum_index)k_datum_index_none;
    }
}

/**
 * Returns the constant deceleration that would take a projectile from initial_velocity to
 * final_velocity over the world-unit span [r0, r1], or 0 when either velocity pair or range
 * pair is degenerate.
 *
 * Register convention in the original: ECX = Projectile tag pointer (unaff/implicit, not a
 * Ghidra-recognized parameter); param_1/param_2 (r0, r1) are Ghidra-recognized stack floats.
 *
 * @address 0x4c03f0
 */
real ProjectileHandle::deceleration_from_range(Projectile *tag, real r0, real r1)
{
    real result = 0.0f;

    if (tag->initial_velocity != tag->final_velocity && (r1 - r0) != 0.0f) {
        result = (tag->initial_velocity * tag->initial_velocity -
                  tag->final_velocity * tag->final_velocity) / ((r1 - r0) + (r1 - r0));
    }
    return result;
}

/**
 * Shared "always false" filler used in several object_type_definition rows for a query-style
 * vtable column that this build never actually wires to type-specific behaviour. Touches
 * nothing but AL.
 *
 * Register convention in the original: none.
 *
 * @address 0x00571dd0
 */
uint8_t ObjectTypeStubs::return_false()
{
    return 0;
}

/**
 * Shared "always true" filler used in several object_type_definition rows for a query-style
 * vtable column that this build never actually wires to type-specific behaviour. Touches
 * nothing but AL.
 *
 * Register convention in the original: none.
 *
 * @address 0x00572a80
 */
uint8_t ObjectTypeStubs::return_true()
{
    return 1;
}

}

extern "C" {

uint8_t projectile_new(uint32_t object_index)
{
    return halo::projectiles::ProjectileHandle(object_index).construct();
}

void projectile_update_function_values(uint32_t object_index)
{
    halo::projectiles::ProjectileHandle(object_index).update_function_values();
}

void projectile_compute_deceleration(uint32_t object_index)
{
    halo::projectiles::ProjectileHandle(object_index).compute_deceleration();
}

void projectile_compute_rotation(uint32_t object_index)
{
    halo::projectiles::ProjectileHandle(object_index).compute_rotation();
}

uint8_t projectile_collision_test(uint32_t object_index, real_point3d *target, void *out_record)
{
    return halo::projectiles::ProjectileHandle(object_index).collision_test(target, out_record);
}

void projectile_detonate(uint32_t object_index, char first_collision, real remaining_tick_fraction)
{
    halo::projectiles::ProjectileHandle(object_index).detonate(first_collision, remaining_tick_fraction);
}

uint8_t projectile_force_detonate(uint32_t object_index)
{
    return halo::projectiles::ProjectileHandle(object_index).force_detonate();
}

uint8_t projectile_is_old_enough(uint32_t object_index)
{
    return halo::projectiles::ProjectileHandle(object_index).is_old_enough();
}

void projectile_notify_object_deleted(uint32_t object_index, datum_index dying_object_index)
{
    halo::projectiles::ProjectileHandle(object_index).notify_object_deleted(dying_object_index);
}

real projectile_deceleration_from_range(Projectile *tag, real r0, real r1)
{
    return halo::projectiles::ProjectileHandle::deceleration_from_range(tag, r0, r1);
}

uint8_t object_type_definition_return_false(void)
{
    return halo::projectiles::ObjectTypeStubs::return_false();
}

uint8_t object_type_definition_return_true(void)
{
    return halo::projectiles::ObjectTypeStubs::return_true();
}

}
