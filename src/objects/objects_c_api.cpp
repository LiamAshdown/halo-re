#include "halo/objects/engine_types.hpp"
#include "halo/objects/hash_table.hpp"
#include "halo/objects/object_type_definitions.hpp"
#include "halo/objects/object_block_storage.hpp"
#include "halo/objects/object_manager.hpp"
#include "halo/objects/object_factory.hpp"
#include "halo/objects/object_queries.hpp"
#include "halo/objects/object_ref.hpp"
#include "halo/objects/object_lifetime.hpp"
#include "halo/objects/object_update.hpp"
#include "halo/objects/object_damage.hpp"
#include "halo/objects/object_lighting.hpp"
#include "halo/objects/light_system.hpp"
#include "halo/objects/light_volume.hpp"
#include "halo/objects/widgets.hpp"
#include "halo/objects/antenna.hpp"
#include "halo/objects/flag.hpp"
#include "halo/objects/glow.hpp"
#include "game.h"
#include "units.h"
#include "effects.h"
#include "networking.h"
#include "physics.h"
#include "structures.h"
#include "rasterizer.h"
#include "render.h"
#include "hs.h"
#include "projectiles.h"
#include "models.h"
#include "cutscene.h"
#include "bitmaps.h"

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_center_of_mass_and_scale.
 *
 * @address 0x004088e0
 */
extern "C" void object_get_center_of_mass_and_scale(real_point3d *out_center, uint32_t object_index,
    float *out_radius)
{
    halo::objects::ObjectRef(object_index).get_center_of_mass_and_scale(out_center, out_radius);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::apply_impulse_and_spin.
 *
 * @address 0x004bef80
 */
extern "C" void object_apply_impulse_and_spin(uint32_t object_index, real_vector3d *delta_velocity)
{
    halo::objects::ObjectRef(object_index).apply_impulse_and_spin(delta_velocity);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::initialize_shield_stun_thresholds.
 *
 * @address 0x004ed440
 */
extern "C" void object_initialize_shield_stun_thresholds(uint32_t object_index, float *override_max_body_vitality,
    float *override_max_shield_vitality)
{
    halo::objects::ObjectDamage(object_index).initialize_shield_stun_thresholds(override_max_body_vitality, override_max_shield_vitality);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::update_vitality_and_regeneration.
 *
 * @address 0x004ed510
 */
extern "C" void object_update_vitality_and_regeneration(uint32_t object_index)
{
    halo::objects::ObjectDamage(object_index).update_vitality_and_regeneration();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageDataView::initialize.
 *
 * @address 0x004ed990
 */
extern "C" void damage_data_initialize(damage_data *dd, datum_index damage_effect_tag)
{
    halo::objects::DamageDataView(dd).initialize(damage_effect_tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::restore_full_body_vitality.
 *
 * @address 0x004ed9d0
 */
extern "C" uint8_t object_restore_full_body_vitality(uint32_t object_index)
{
    return halo::objects::ObjectDamage(object_index).restore_full_body_vitality();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::set_health_frozen_flag.
 *
 * @address 0x004eda20
 */
extern "C" void object_set_health_frozen_flag(uint32_t object_index)
{
    halo::objects::ObjectDamage(object_index).set_health_frozen_flag();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::set_shield_depleted_flag.
 *
 * @address 0x004edb10
 */
extern "C" void object_set_shield_depleted_flag(uint32_t object_index)
{
    halo::objects::ObjectDamage(object_index).set_shield_depleted_flag();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::shield_recharge_start.
 *
 * @address 0x004edba0
 */
extern "C" uint8_t object_shield_recharge_start(uint32_t object_index)
{
    return halo::objects::ObjectDamage(object_index).shield_recharge_start();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::children_recurse_prune.
 *
 * @address 0x004edc10
 */
extern "C" void object_children_recurse_prune(uint32_t object_index)
{
    halo::objects::ObjectRef(object_index).children_recurse_prune();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::delete_teardown.
 *
 * @address 0x004edc80
 */
extern "C" void object_delete_teardown(uint32_t object_index)
{
    halo::objects::ObjectLifetime(object_index).delete_teardown();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageDataView::apply_area_effect.
 *
 * @address 0x004edd30
 */
extern "C" void damage_apply_area_effect(damage_data *dd)
{
    halo::objects::DamageDataView(dd).apply_area_effect();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::apply_line_of_sight.
 *
 * @address 0x004eddb0
 */
extern "C" void object_damage_apply_line_of_sight(damage_data *dd, datum_index target_index, int8_t continue_flag)
{
    halo::objects::ObjectDamage(target_index).apply_line_of_sight(dd, continue_flag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_controlling_player_index.
 *
 * @address 0x004ee2e0
 */
extern "C" int32_t object_get_controlling_player_index(datum_index object_index)
{
    return halo::objects::ObjectRef(object_index).get_controlling_player_index();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageSystem::throttled_multiplayer_sound_event.
 *
 * @address 0x004ee370
 */
extern "C" void object_throttled_multiplayer_sound_event()
{
    halo::objects::DamageSystem::throttled_multiplayer_sound_event();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::notify_pickup_or_refresh_probe.
 *
 * @address 0x004ee3c0
 */
extern "C" void object_notify_pickup_or_refresh_probe(uint32_t object_index, datum_index player_index)
{
    halo::objects::ObjectRef(object_index).notify_pickup_or_refresh_probe(player_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageSystem::apply_shield_charge_and_notify.
 *
 * @address 0x004ee4d0
 */
extern "C" void object_apply_shield_charge_and_notify(void **message)
{
    halo::objects::DamageSystem::apply_shield_charge_and_notify(message);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::apply_damage.
 *
 * @address 0x004ee5e0
 */
extern "C" void object_apply_damage(damage_data *dd, uint32_t target_object_index, int16_t hit_node_index,
    int16_t hit_region_index, int16_t hit_material_index, uint32_t hit_plane)
{
    halo::objects::ObjectDamage(target_object_index).apply_damage(dd, hit_node_index, hit_region_index, hit_material_index, hit_plane);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectHashFlags::clear_bit3.
 *
 * @address 0x004ef160
 */
extern "C" void object_hash_clear_flag_bit3(uint32_t key)
{
    halo::objects::ObjectHashFlags::clear_bit3(key);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectHashFlags::set_bit3.
 *
 * @address 0x004ef200
 */
extern "C" void object_hash_set_flag_bit3(uint32_t key)
{
    halo::objects::ObjectHashFlags::set_bit3(key);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::apply_body_damage.
 *
 * @address 0x004ef2a0
 */
extern "C" void object_apply_body_damage(uint32_t target_index, int32_t region_index, int32_t node_index, void *plane,
    uint8_t *geometry, uint8_t *material, uint8_t *effect_block, damage_data *dd, uint32_t *notify_flags,
    float *body_damage_out, float *material_multiplier_out, float damage, uint8_t is_local)
{
    halo::objects::ObjectDamage(target_index).apply_body_damage(region_index, node_index, plane, geometry, material, effect_block, dd, notify_flags, body_damage_out, material_multiplier_out, damage, is_local);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::apply_shield_damage.
 *
 * @address 0x004ef820
 */
extern "C" void object_apply_shield_damage(uint32_t target_index, uint8_t *geometry, uint8_t *material,
    uint8_t *effect_block, uint32_t *notify_flags, float *shield_damage_out, float *remaining_damage,
    uint8_t is_local, uint8_t apply_state, object_shield_impulse_result *record)
{
    halo::objects::ObjectDamage(target_index).apply_shield_damage(geometry, material, effect_block, notify_flags, shield_damage_out, remaining_damage, is_local, apply_state, record);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageSystem::queue_pickup_denied_event.
 *
 * @address 0x004efbf0
 */
extern "C" void object_queue_pickup_denied_event(void *param_1, int32_t key, uint32_t *source)
{
    halo::objects::DamageSystem::queue_pickup_denied_event(param_1, key, source);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageSystem::apply_linked_impulse.
 *
 * @address 0x004efc80
 */
extern "C" void object_apply_linked_impulse(void **message)
{
    halo::objects::DamageSystem::apply_linked_impulse(message);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::notify_and_impulse.
 *
 * @address 0x004efcf0
 */
extern "C" void object_damage_notify_and_impulse(uint32_t target_index, damage_data *dd, uint32_t notify_flags,
    float shield_damage, float body_damage, uint32_t unused_6, int32_t region_index, uint32_t is_local)
{
    halo::objects::ObjectDamage(target_index).notify_and_impulse(dd, notify_flags, shield_damage, body_damage, unused_6, region_index, is_local);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageSystem::dispatch_effect_notify.
 *
 * @address 0x004efff0
 */
extern "C" void object_dispatch_effect_notify(uint32_t forwarded_eax, uint32_t forwarded_ecx)
{
    halo::objects::DamageSystem::dispatch_effect_notify(forwarded_eax, forwarded_ecx);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageSystem::effect_new_at_location.
 *
 * @address 0x004f0010
 */
extern "C" void damage_effect_new_at_location(datum_index effect_tag, int16_t node_index, real_vector3d *normal,
    real_vector3d *incident, real_point3d *impact_position, uint32_t object_index)
{
    halo::objects::DamageSystem::effect_new_at_location(effect_tag, node_index, normal, incident, impact_position, object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageSystem::effect_dispatch.
 *
 * @address 0x004f0250
 */
extern "C" void object_damage_effect_dispatch(int32_t push_value, int32_t node_object)
{
    halo::objects::DamageSystem::effect_dispatch(push_value, node_object);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectDamage::destroy_region.
 *
 * @address 0x004f02d0
 */
extern "C" void object_destroy_region(uint32_t object_index, int32_t region_index)
{
    halo::objects::ObjectDamage(object_index).destroy_region(region_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::regions_reset_permutation_lock.
 *
 * @address 0x004f03e0
 */
extern "C" void object_regions_reset_permutation_lock(uint32_t object_index, int8_t unlock)
{
    halo::objects::ObjectUpdater(object_index).regions_reset_permutation_lock(unlock);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::HashTableView::initialize.
 *
 * @address 0x004f0470
 */
extern "C" void hash_table_initialize(hash_table *table, int32_t bucket_count)
{
    halo::objects::HashTableView(table).initialize(bucket_count);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::HashTableView::dispose.
 *
 * @address 0x004f04c0
 */
extern "C" void hash_table_dispose(hash_table *table)
{
    halo::objects::HashTableView(table).dispose();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::HashTableView::set_or_remove.
 *
 * @address 0x004f0530
 */
extern "C" void hash_table_set_or_remove(hash_table *table, int32_t key, int32_t value)
{
    halo::objects::HashTableView(table).set_or_remove(key, value);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::HashTableView::get.
 *
 * @address 0x004f05e0
 */
extern "C" int32_t hash_table_get(hash_table *table, int32_t key)
{
    return halo::objects::HashTableView(table).get(key);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::HashTableView::grow_freelist.
 *
 * @address 0x004f0620
 */
extern "C" void hash_table_grow_freelist(hash_table *table)
{
    halo::objects::HashTableView(table).grow_freelist();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::initialize.
 *
 * @address 0x004f0a20
 */
extern "C" void lights_initialize()
{
    halo::objects::LightSystem::initialize();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::dispose_all.
 *
 * @address 0x004f0aa0
 */
extern "C" void lights_dispose_all()
{
    halo::objects::LightSystem::dispose_all();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::new_attached.
 *
 * @address 0x004f0af0
 */
extern "C" datum_index light_new_attached(datum_index light_tag, datum_index owner_object, int16_t marker_index,
    int16_t marker_index_secondary, int16_t change_color_index)
{
    return halo::objects::LightSystem::new_attached(light_tag, owner_object, marker_index, marker_index_secondary, change_color_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::destroy.
 *
 * @address 0x004f0bd0
 */
extern "C" void light_delete(datum_index light_handle)
{
    halo::objects::LightSystem::destroy(light_handle);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::new_positioned.
 *
 * @address 0x004f0c10
 */
extern "C" datum_index light_new_positioned(datum_index light_tag, int32_t marker_index, int16_t marker_sub_index,
    real_point3d *position, uint32_t param_5, real_vector3d *direction)
{
    return halo::objects::LightSystem::new_positioned(light_tag, marker_index, marker_sub_index, position, param_5, direction);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::update_all.
 *
 * @address 0x004f0cf0
 */
extern "C" void object_lights_update_all()
{
    halo::objects::LightSystem::update_all();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::transient_add.
 *
 * @address 0x004f1600
 */
extern "C" void light_transient_add(datum_index light_tag, real_vector3d *color, real_point3d *position,
    uint32_t direction, uint32_t param_3, float intensity)
{
    halo::objects::LightSystem::transient_add(light_tag, color, position, direction, param_3, intensity);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::collect_object_references.
 *
 * @address 0x004f1700
 */
extern "C" int16_t light_collect_object_references(uint32_t light_handle, int16_t max_count, int16_t *out_buffer)
{
    return halo::objects::LightSystem::collect_object_references(light_handle, max_count, out_buffer);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::apply_spot_falloff.
 *
 * @address 0x004f1780
 */
extern "C" void lights_apply_spot_falloff()
{
    halo::objects::LightSystem::apply_spot_falloff();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::apply_spot_falloff_specular.
 *
 * @address 0x004f1950
 */
extern "C" void lights_apply_spot_falloff_specular()
{
    halo::objects::LightSystem::apply_spot_falloff_specular();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLighting::sum_attached_light_luminance.
 *
 * @address 0x004f1b30
 */
extern "C" real object_sum_attached_light_luminance(uint32_t object_index)
{
    return halo::objects::ObjectLighting(object_index).sum_attached_light_luminance();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLighting::sample_total_lighting_at_point.
 *
 * @address 0x004f1c20
 */
extern "C" void object_sample_total_lighting_at_point(real_point3d *point, bsp_leaf_reference *location,
    real_vector3d *color)
{
    halo::objects::ObjectLighting::sample_total_lighting_at_point(point, location, color);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLighting::sample_ambient_lightmap_point.
 *
 * @address 0x004f1e60
 */
extern "C" void object_sample_ambient_lightmap_point(real_point3d *point, real_vector3d *lightmap_color,
    real_vector3d *base_map_color, uint8_t wait_for_textures)
{
    halo::objects::ObjectLighting::sample_ambient_lightmap_point(point, lightmap_color, base_map_color, wait_for_textures);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLighting::sample_ambient_lighting.
 *
 * @address 0x004f20b0
 */
extern "C" void object_sample_ambient_lighting(uint32_t object_index, float *sample)
{
    halo::objects::ObjectLighting(object_index).sample_ambient_lighting(sample);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLighting::gather_light_list.
 *
 * @address 0x004f2430
 */
extern "C" void object_gather_light_list(datum_index object_index, uint8_t *out)
{
    halo::objects::ObjectLighting(object_index).gather_light_list(out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::clear_dirty_flag.
 *
 * @address 0x004f29c0
 */
extern "C" void object_light_clear_dirty_flag(uint32_t light_index)
{
    halo::objects::LightSystem::clear_dirty_flag(light_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::recompute_transform.
 *
 * @address 0x004f2a00
 */
extern "C" void object_light_recompute_transform(uint32_t light_index)
{
    halo::objects::LightSystem::recompute_transform(light_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::detach_from_structure_bsp.
 *
 * @address 0x004f2cb0
 */
extern "C" void object_lights_detach_from_structure_bsp()
{
    halo::objects::LightSystem::detach_from_structure_bsp();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::refresh_transforms.
 *
 * @address 0x004f2d50
 */
extern "C" void object_lights_refresh_transforms()
{
    halo::objects::LightSystem::refresh_transforms();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::gather_nearest.
 *
 * @address 0x004f2df0
 */
extern "C" void object_lights_gather_nearest(int16_t cluster_index, uint32_t self_object_index,
    real_point3d *probe_point, float search_margin, uint32_t *out_indices, float *out_intensities,
    uint32_t out_falloffs, int16_t *count, int16_t max_count)
{
    halo::objects::LightSystem::gather_nearest(cluster_index, self_object_index, probe_point, search_margin, out_indices, out_intensities, out_falloffs, count, max_count);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLighting::build_effect_parameter_block.
 *
 * @address 0x004f2ff0
 */
extern "C" void object_build_effect_parameter_block(uint8_t flags, real_vector3d *shading_normal, float intensity,
    ColorRGB *lightmap_color, real_vector3d *lightmap_normal, ColorRGB *base_map_color, render_lighting *lighting)
{
    halo::objects::ObjectLighting::build_effect_parameter_block(flags, shading_normal, intensity, lightmap_color, lightmap_normal, base_map_color, lighting);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLighting::color_clamp_to_intensity.
 *
 * @address 0x004f3410
 */
extern "C" void object_color_clamp_to_intensity(float intensity, ColorRGB *color)
{
    halo::objects::ObjectLighting::color_clamp_to_intensity(intensity, color);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::cluster_iterate_begin.
 *
 * @address 0x004f34c0
 */
extern "C" datum_index light_cluster_iterate_begin(datum_index *cursor, int16_t cluster_index)
{
    return halo::objects::LightSystem::cluster_iterate_begin(cursor, cluster_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::cluster_iterate_next.
 *
 * @address 0x004f3500
 */
extern "C" datum_index light_cluster_iterate_next(datum_index *cursor)
{
    return halo::objects::LightSystem::cluster_iterate_next(cursor);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::get_render_bounds.
 *
 * @address 0x004f3530
 */
extern "C" void light_get_render_bounds(datum_index handle, real_point3d *center_out, float *radius_out)
{
    halo::objects::LightSystem::get_render_bounds(handle, center_out, radius_out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::not_marked_this_frame.
 *
 * @address 0x004f3620
 */
extern "C" uint8_t light_not_marked_this_frame(datum_index handle)
{
    return halo::objects::LightSystem::not_marked_this_frame(handle);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightSystem::mark_this_frame.
 *
 * @address 0x004f3650
 */
extern "C" uint8_t light_mark_this_frame(datum_index handle)
{
    return halo::objects::LightSystem::mark_this_frame(handle);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectFactory::place_scenario.
 *
 * @address 0x004f3ba0
 */
extern "C" void scenario_objects_place(uint8_t *scenario)
{
    halo::objects::ObjectFactory::place_scenario(scenario);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::chain_build.
 *
 * @address 0x004f3db0
 */
extern "C" void object_type_definition_chain_build()
{
    halo::objects::ObjectTypeDefinitions::chain_build();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_0x24.
 *
 * @address 0x004f3e30
 */
extern "C" void object_type_definitions_notify_0x24(uint32_t object_index, uint32_t argument)
{
    halo::objects::ObjectTypeDefinitions::notify_0x24(object_index, argument);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::query_0x28.
 *
 * @address 0x004f3ea0
 */
extern "C" uint8_t object_type_definitions_query_0x28(uint32_t object_index)
{
    return halo::objects::ObjectTypeDefinitions::query_0x28(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_two_args_0x2c.
 *
 * @address 0x004f3f20
 */
extern "C" void object_type_definitions_notify_two_args_0x2c(uint32_t object_index, uint32_t event_argument)
{
    halo::objects::ObjectTypeDefinitions::notify_two_args_0x2c(object_index, event_argument);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_0x30.
 *
 * @address 0x004f3f90
 */
extern "C" void object_type_definitions_notify_0x30(uint32_t object_index)
{
    halo::objects::ObjectTypeDefinitions::notify_0x30(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::query_0x34.
 *
 * @address 0x004f4000
 */
extern "C" uint8_t object_type_definitions_query_0x34(uint32_t object_index)
{
    return halo::objects::ObjectTypeDefinitions::query_0x34(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_0x38.
 *
 * @address 0x004f4080
 */
extern "C" void object_type_definitions_notify_0x38(uint32_t object_index)
{
    halo::objects::ObjectTypeDefinitions::notify_0x38(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_0x3c.
 *
 * @address 0x004f40f0
 */
extern "C" void object_type_definitions_notify_0x3c(uint32_t object_index, uint32_t argument)
{
    halo::objects::ObjectTypeDefinitions::notify_0x3c(object_index, argument);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_region_damage.
 *
 * @address 0x004f4160
 */
extern "C" void object_type_definitions_notify_region_damage(uint32_t object_index, uint32_t argument_1,
    uint32_t argument_2)
{
    halo::objects::ObjectTypeDefinitions::notify_region_damage(object_index, argument_1, argument_2);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::query_0x44.
 *
 * @address 0x004f41d0
 */
extern "C" uint8_t object_type_definitions_query_0x44(uint32_t object_index)
{
    return halo::objects::ObjectTypeDefinitions::query_0x44(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_two_args_0x48.
 *
 * @address 0x004f4250
 */
extern "C" void object_type_definitions_notify_two_args_0x48(uint32_t object_index, uint32_t event_argument)
{
    halo::objects::ObjectTypeDefinitions::notify_two_args_0x48(object_index, event_argument);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_0x4c.
 *
 * @address 0x004f42c0
 */
extern "C" void object_type_definitions_notify_0x4c(uint32_t object_index, uint32_t argument)
{
    halo::objects::ObjectTypeDefinitions::notify_0x4c(object_index, argument);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_0x50.
 *
 * @address 0x004f4330
 */
extern "C" void object_type_definitions_notify_0x50(uint32_t object_index)
{
    halo::objects::ObjectTypeDefinitions::notify_0x50(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_0x54.
 *
 * @address 0x004f43a0
 */
extern "C" void object_type_definitions_notify_0x54(uint32_t object_index)
{
    halo::objects::ObjectTypeDefinitions::notify_0x54(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_0x5c.
 *
 * @address 0x004f4410
 */
extern "C" void object_type_definitions_notify_0x5c(uint32_t object_index)
{
    halo::objects::ObjectTypeDefinitions::notify_0x5c(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::notify_0x58.
 *
 * @address 0x004f4480
 */
extern "C" void object_type_definitions_notify_0x58(uint32_t object_index, uint32_t argument_1, uint32_t argument_2)
{
    halo::objects::ObjectTypeDefinitions::notify_0x58(object_index, argument_1, argument_2);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::override_get_0x64.
 *
 * @address 0x004f44f0
 */
extern "C" int object_type_override_get_0x64(uint32_t object_index, void *buffer, int32_t buffer_size)
{
    return halo::objects::ObjectTypeDefinitions::override_get_0x64(object_index, buffer, buffer_size);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::override_call_0x68.
 *
 * @address 0x004f4560
 */
extern "C" void object_type_override_call_0x68(uint32_t object_index)
{
    halo::objects::ObjectTypeDefinitions::override_call_0x68(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::override_call_0x6c.
 *
 * @address 0x004f45b0
 */
extern "C" int object_type_override_call_0x6c(uint32_t object_index, void *buffer, int32_t bit_budget,
    int32_t full_update)
{
    return halo::objects::ObjectTypeDefinitions::override_call_0x6c(object_index, buffer, bit_budget, full_update);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::override_call_0x70.
 *
 * @address 0x004f4620
 */
extern "C" void object_type_override_call_0x70(uint32_t object_index, uint32_t edi_argument, uint32_t stack_argument)
{
    halo::objects::ObjectTypeDefinitions::override_call_0x70(object_index, edi_argument, stack_argument);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::override_call_0x70_release_node.
 *
 * @address 0x004f4680
 */
extern "C" void object_type_override_call_0x70_release_node(int32_t *record, uint32_t client)
{
    halo::objects::ObjectTypeDefinitions::override_call_0x70_release_node(record, client);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::datum_consume_pending_flag.
 *
 * @address 0x004f46b0
 */
extern "C" uint8_t object_datum_consume_pending_flag(uint32_t object_index)
{
    return halo::objects::ObjectLifetime(object_index).datum_consume_pending_flag();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::override_call_0x74.
 *
 * @address 0x004f4700
 */
extern "C" uint8_t object_type_override_call_0x74(uint32_t object_index)
{
    return halo::objects::ObjectTypeDefinitions::override_call_0x74(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectTypeDefinitions::override_call_0x7c.
 *
 * @address 0x004f4760
 */
extern "C" void object_type_override_call_0x7c(uint32_t object_index)
{
    halo::objects::ObjectTypeDefinitions::override_call_0x7c(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::delete_unparented_of_type_mask.
 *
 * @address 0x004f47c0
 */
extern "C" void objects_delete_unparented_of_type_mask()
{
    halo::objects::ObjectManager::delete_unparented_of_type_mask();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectFactory::place_for_structure_bsp_on_activate.
 *
 * @address 0x004f4860
 */
extern "C" void scenario_objects_place_for_structure_bsp_on_activate()
{
    halo::objects::ObjectFactory::place_for_structure_bsp_on_activate();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectFactory::place_for_structure_bsp.
 *
 * @address 0x004f4880
 */
extern "C" void scenario_objects_place_for_structure_bsp(uint8_t place)
{
    halo::objects::ObjectFactory::place_for_structure_bsp(place);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::initialize.
 *
 * @address 0x004f4ad0
 */
extern "C" void objects_initialize()
{
    halo::objects::ObjectManager::initialize();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::reset.
 *
 * @address 0x004f4bb0
 */
extern "C" void objects_reset()
{
    halo::objects::ObjectManager::reset();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::flush_dirty_state.
 *
 * @address 0x004f4cc0
 */
extern "C" void objects_flush_dirty_state()
{
    halo::objects::ObjectManager::flush_dirty_state();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::dispose.
 *
 * @address 0x004f4db0
 */
extern "C" void objects_dispose()
{
    halo::objects::ObjectManager::dispose();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::update.
 *
 * @address 0x004f4e90
 */
extern "C" void objects_update()
{
    halo::objects::ObjectManager::update();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::mark_pending_delete.
 *
 * @address 0x004f50f0
 */
extern "C" void object_mark_pending_delete(uint32_t object_index)
{
    halo::objects::ObjectLifetime(object_index).mark_pending_delete();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::clear_pending_delete_flag.
 *
 * @address 0x004f5130
 */
extern "C" void object_clear_pending_delete_flag(uint32_t object_index)
{
    halo::objects::ObjectLifetime(object_index).clear_pending_delete_flag();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::reset_velocity_and_wake.
 *
 * @address 0x004f5160
 */
extern "C" void object_reset_velocity_and_wake(uint32_t object_index)
{
    halo::objects::ObjectRef(object_index).reset_velocity_and_wake();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::set_position_and_orientation.
 *
 * @address 0x004f51c0
 */
extern "C" void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward, real_vector3d *up,
    real_point3d *position)
{
    halo::objects::ObjectRef(object_index).set_position_and_orientation(forward, up, position);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::set_position_and_recalculate.
 *
 * @address 0x004f52c0
 */
extern "C" void object_set_position_and_recalculate(real_point3d *position, uint32_t object_index)
{
    halo::objects::ObjectRef(object_index).set_position_and_recalculate(position);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::set_position_and_relink.
 *
 * @address 0x004f5350
 */
extern "C" void object_set_position_and_relink(real_point3d *position, uint32_t object_index,
    bsp_leaf_reference *location)
{
    halo::objects::ObjectRef(object_index).set_position_and_relink(position, location);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectPlacementDataView::initialize.
 *
 * @address 0x004f53a0
 */
extern "C" void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,
    datum_index role)
{
    halo::objects::ObjectPlacementDataView(placement).initialize(definition_tag, role);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectFactory::create.
 *
 * @address 0x004f5460
 */
extern "C" datum_index object_new(object_placement_data *placement)
{
    return halo::objects::ObjectFactory::create(placement);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectFactory::create_with_role_control.
 *
 * @address 0x004f54b0
 */
extern "C" datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role)
{
    return halo::objects::ObjectFactory::create_with_role_control(placement, role);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::delete_recursive.
 *
 * @address 0x004f59d0
 */
extern "C" void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings)
{
    halo::objects::ObjectLifetime(object_index).delete_recursive(recurse_siblings);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::delete_unparented.
 *
 * @address 0x004f5aa0
 */
extern "C" void object_delete_unparented(uint32_t object_index)
{
    halo::objects::ObjectLifetime(object_index).delete_unparented();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::delete_by_pooled_node_id.
 *
 * @address 0x004f5b50
 */
extern "C" void object_delete_by_pooled_node_id(int32_t **record)
{
    halo::objects::ObjectLifetime::delete_by_pooled_node_id(record);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::destroy.
 *
 * @address 0x004f5bd0
 */
extern "C" void object_delete(uint32_t object_index)
{
    halo::objects::ObjectLifetime(object_index).destroy();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::is_delete_pending.
 *
 * @address 0x004f5c10
 */
extern "C" uint8_t object_is_delete_pending(uint32_t object_index)
{
    return halo::objects::ObjectLifetime(object_index).is_delete_pending();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::set_cluster_and_parent.
 *
 * @address 0x004f5c30
 */
extern "C" void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location)
{
    halo::objects::ObjectRef(object_index).set_cluster_and_parent(location);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::unlink_cluster_or_notify_parent.
 *
 * @address 0x004f5de0
 */
extern "C" void object_unlink_cluster_or_notify_parent(uint32_t object_index)
{
    halo::objects::ObjectRef(object_index).unlink_cluster_or_notify_parent();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::noncollideable_iterate_begin.
 *
 * @address 0x004f5e90
 */
extern "C" datum_index object_cluster_noncollideable_iterate_begin(datum_index *cursor, int16_t cluster_index)
{
    return halo::objects::ObjectQueries::noncollideable_iterate_begin(cursor, cluster_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::noncollideable_iterate_next.
 *
 * @address 0x004f5ed0
 */
extern "C" datum_index object_cluster_noncollideable_iterate_next(datum_index *cursor)
{
    return halo::objects::ObjectQueries::noncollideable_iterate_next(cursor);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::resolve_collideable_reference.
 *
 * @address 0x004f5f00
 */
extern "C" datum_index object_resolve_collideable_reference(datum_index *next_reference, int16_t cluster_index)
{
    return halo::objects::ObjectQueries::resolve_collideable_reference(next_reference, cluster_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::collideable_iterate_next.
 *
 * @address 0x004f5f40
 */
extern "C" datum_index object_cluster_collideable_iterate_next(datum_index *cursor)
{
    return halo::objects::ObjectQueries::collideable_iterate_next(cursor);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_root_parent_placement.
 *
 * @address 0x004f5f70
 */
extern "C" int16_t object_get_root_parent_placement(uint32_t object_index, object_placement_cursor *out_cursor)
{
    return halo::objects::ObjectRef(object_index).get_root_parent_placement(out_cursor);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_node_marker_address.
 *
 * @address 0x004f6000
 */
extern "C" real_matrix4x3 * object_get_node_marker_address(uint32_t object_index, int16_t node_index)
{
    return halo::objects::ObjectRef(object_index).get_node_marker_address(node_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_attachment_marker_name.
 *
 * @address 0x004f6030
 */
extern "C" char * object_get_attachment_marker_name(uint32_t object_index, int16_t attachment_index)
{
    return halo::objects::ObjectRef(object_index).get_attachment_marker_name(attachment_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_node_local_transform.
 *
 * @address 0x004f6080
 */
extern "C" int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t maximum_markers)
{
    return halo::objects::ObjectRef(object_index).get_node_local_transform(marker_name, marker, maximum_markers);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::reorient_relative_to_marker.
 *
 * @address 0x004f6180
 */
extern "C" void object_reorient_relative_to_marker(uint32_t parent_index, char *parent_marker_name,
    uint32_t object_index, char *object_marker_name)
{
    halo::objects::ObjectRef(object_index).reorient_relative_to_marker(parent_index, parent_marker_name, object_marker_name);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectView::recompute_basis_from_marker_delta.
 *
 * @address 0x004f62f0
 */
extern "C" void object_recompute_basis_from_marker_delta(object *obj, object_marker *marker,
    real_matrix4x3 *output_matrix)
{
    halo::objects::ObjectView(obj).recompute_basis_from_marker_delta(marker, output_matrix);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::attach_to_object.
 *
 * @address 0x004f6440
 */
extern "C" void object_attach_to_object(uint32_t parent_index, uint32_t child_index, int16_t marker_index)
{
    halo::objects::ObjectRef(parent_index).attach_to_object(child_index, marker_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::snap_to_parent_marker_and_detach.
 *
 * @address 0x004f6610
 */
extern "C" void object_snap_to_parent_marker_and_detach(uint32_t object_index)
{
    halo::objects::ObjectRef(object_index).snap_to_parent_marker_and_detach();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::set_in_pvs_pass_flag.
 *
 * @address 0x004f67e0
 */
extern "C" void object_set_in_pvs_pass_flag(uint32_t object_index, uint8_t in_pvs)
{
    halo::objects::ObjectRef(object_index).set_in_pvs_pass_flag(in_pvs);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::set_collision_enabled.
 *
 * @address 0x004f6850
 */
extern "C" void object_set_collision_enabled(uint32_t object_index, uint8_t enable)
{
    halo::objects::ObjectRef(object_index).set_collision_enabled(enable);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_position.
 *
 * @address 0x004f6900
 */
extern "C" void object_get_position(real_point3d *out, uint32_t object_index)
{
    halo::objects::ObjectRef(object_index).get_position(out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_orientation.
 *
 * @address 0x004f6970
 */
extern "C" void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up)
{
    halo::objects::ObjectRef(object_index).get_orientation(out_forward, out_up);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_world_matrix.
 *
 * @address 0x004f6a20
 */
extern "C" real_matrix4x3 * object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out)
{
    return halo::objects::ObjectRef(object_index).get_world_matrix(out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_root_object_velocities.
 *
 * @address 0x004f6aa0
 */
extern "C" void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity)
{
    halo::objects::ObjectRef(object_index).get_root_object_velocities(out_velocity, out_angular_velocity);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_root_location.
 *
 * @address 0x004f6b10
 */
extern "C" void object_get_root_location(int32_t *out, uint32_t object_index)
{
    halo::objects::ObjectRef(object_index).get_root_location(out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::copy_default_node_transforms.
 *
 * @address 0x004f6b70
 */
extern "C" void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count)
{
    halo::objects::ObjectRef(object_index).copy_default_node_transforms(requested_count);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::offset_node_translation.
 *
 * @address 0x004f6c10
 */
extern "C" void object_offset_node_translation(uint32_t object_index, real_vector3d *delta)
{
    halo::objects::ObjectRef(object_index).offset_node_translation(delta);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::set_permutation_by_name.
 *
 * @address 0x004f6c60
 */
extern "C" void object_set_permutation_by_name(uint32_t object_index, char *name, int16_t region_filter,
    char use_matched_index)
{
    halo::objects::ObjectUpdater(object_index).set_permutation_by_name(name, region_filter, use_matched_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::solve_two_bone_ik_to_marker.
 *
 * @address 0x004f6d60
 */
extern "C" void object_solve_two_bone_ik_to_marker(uint32_t object_index, char *marker_a_name,
    uint32_t marker_b_object_index, char *marker_b_name, uint8_t *node_base)
{
    halo::objects::ObjectRef(object_index).solve_two_bone_ik_to_marker(marker_a_name, marker_b_object_index, marker_b_name, node_base);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::function_get_value.
 *
 * @address 0x004f6e70
 */
extern "C" uint8_t object_function_get_value(uint32_t object_index, int16_t selector, float *out_value)
{
    return halo::objects::ObjectRef(object_index).function_get_value(selector, out_value);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::try_and_get.
 *
 * @address 0x004f6ec0
 */
extern "C" object * object_try_and_get(datum_index object_index, uint32_t type_mask)
{
    return halo::objects::ObjectQueries::try_and_get(object_index, type_mask);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectIteratorView::next.
 *
 * @address 0x004f6f20
 */
extern "C" object * object_iterator_next(object_iterator *iterator)
{
    return halo::objects::ObjectIteratorView(iterator).next();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::get_root_object_index.
 *
 * @address 0x004f6fb0
 */
extern "C" uint32_t object_get_root_object_index(uint32_t object_index)
{
    return halo::objects::ObjectRef(object_index).get_root_object_index();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::find_in_sphere.
 *
 * @address 0x004f6fe0
 */
extern "C" int16_t object_find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location,
    real_point3d *center, float radius, datum_index *out_objects, int16_t max_output)
{
    return halo::objects::ObjectQueries::find_in_sphere(search_mask, type_mask, location, center, radius, out_objects, max_output);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::collect_in_clusters.
 *
 * @address 0x004f7180
 */
extern "C" int16_t object_collect_in_clusters(uint32_t search_mask, int16_t cluster_count, int16_t *cluster_indices,
    int16_t max_output, datum_index *out_objects)
{
    return halo::objects::ObjectQueries::collect_in_clusters(search_mask, cluster_count, cluster_indices, max_output, out_objects);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectFactory::create_from_scenario_name.
 *
 * @address 0x004f7370
 */
extern "C" datum_index object_new_from_scenario_name(int16_t name_index)
{
    return halo::objects::ObjectFactory::create_from_scenario_name(name_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectFactory::lookup_by_name.
 *
 * @address 0x004f73c0
 */
extern "C" datum_index object_lookup_table_get(int16_t name_index)
{
    return halo::objects::ObjectFactory::lookup_by_name(name_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::clear_references_to_object.
 *
 * @address 0x004f73e0
 */
extern "C" void object_clear_references_to_object(uint32_t dying_object_index)
{
    halo::objects::ObjectLifetime(dying_object_index).clear_references_to_object();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::list_membership_set.
 *
 * @address 0x004f7450
 */
extern "C" void object_list_membership_set(uint32_t object_index, char add)
{
    halo::objects::ObjectRef(object_index).list_membership_set(add);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::sweep_refresh_cluster_membership.
 *
 * @address 0x004f74f0
 */
extern "C" void object_sweep_refresh_cluster_membership()
{
    halo::objects::ObjectManager::sweep_refresh_cluster_membership();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::recompute_cluster_membership.
 *
 * @address 0x004f7570
 */
extern "C" void objects_recompute_cluster_membership()
{
    halo::objects::ObjectManager::recompute_cluster_membership();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::test_in_atmosphere_zone.
 *
 * @address 0x004f76e0
 */
extern "C" uint8_t object_test_in_atmosphere_zone(uint32_t object_index)
{
    return halo::objects::ObjectRef(object_index).test_in_atmosphere_zone();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::get_statistics.
 *
 * @address 0x004f7950
 */
extern "C" void objects_get_statistics(object_statistics *out)
{
    halo::objects::ObjectManager::get_statistics(out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::set_ambient_cluster_override.
 *
 * @address 0x004f79d0
 */
extern "C" void objects_set_ambient_cluster_override(int16_t local_player_index)
{
    halo::objects::ObjectManager::set_ambient_cluster_override(local_player_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::get_ambient_cluster.
 *
 * @address 0x004f7a50
 */
extern "C" int16_t objects_get_ambient_cluster()
{
    return halo::objects::ObjectManager::get_ambient_cluster();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectFactory::notify_predicted_resources_if_valid.
 *
 * @address 0x004f7ad0
 */
extern "C" void object_notify_predicted_resources_if_valid(datum_index definition_tag)
{
    halo::objects::ObjectFactory::notify_predicted_resources_if_valid(definition_tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::notify_children_recursive.
 *
 * @address 0x004f7b00
 */
extern "C" void object_notify_children_recursive(uint32_t object_index)
{
    halo::objects::ObjectRef(object_index).notify_children_recursive();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::reposition_to_spawn_location.
 *
 * @address 0x004f7b70
 */
extern "C" uint8_t object_reposition_to_spawn_location(uint32_t object_index, real_point3d *target_position,
    uint32_t ignore_object_index)
{
    return halo::objects::ObjectRef(object_index).reposition_to_spawn_location(target_position, ignore_object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::nudge_position_by_velocity.
 *
 * @address 0x004f7c40
 */
extern "C" uint8_t object_nudge_position_by_velocity(uint32_t object_index, real_point3d *out)
{
    return halo::objects::ObjectRef(object_index).nudge_position_by_velocity(out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectBlockStorage::create.
 *
 * @address 0x004f7d50
 */
extern "C" datum_index object_block_data_new(int32_t specific_index, data_array *array, int16_t size)
{
    return halo::objects::ObjectBlockStorage::create(specific_index, array, size);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectBlockStorage::release.
 *
 * @address 0x004f7de0
 */
extern "C" void object_block_data_free(data_array *array, datum_index handle)
{
    halo::objects::ObjectBlockStorage::release(array, handle);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectBlockStorage::grow.
 *
 * @address 0x004f7e50
 */
extern "C" uint8_t object_block_data_grow(uint32_t object_index, int16_t field_offset, int16_t extra_size)
{
    return halo::objects::ObjectBlockStorage::grow(object_index, field_offset, extra_size);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::update.
 *
 * @address 0x004f7ef0
 */
extern "C" uint8_t object_update(uint32_t object_index)
{
    return halo::objects::ObjectUpdater(object_index).update();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::update_export_functions.
 *
 * @address 0x004f80d0
 */
extern "C" void object_update_export_functions(datum_index object_index)
{
    halo::objects::ObjectUpdater(object_index).update_export_functions();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::function_evaluate_input.
 *
 * @address 0x004f8207
 */
extern "C" void object_function_evaluate_input(float initial_angle_input, float initial_st0, int16_t *selectors,
    float *out_values, uint8_t *object_tag_data, int32_t object_index_scaled, int32_t remaining_count)
{
    halo::objects::ObjectUpdater::function_evaluate_input(initial_angle_input, initial_st0, selectors, out_values, object_tag_data, object_index_scaled, remaining_count);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::recalculate_bounding_radius_recursive.
 *
 * @address 0x004f82b0
 */
extern "C" void object_recalculate_bounding_radius_recursive(uint32_t object_index)
{
    halo::objects::ObjectUpdater(object_index).recalculate_bounding_radius_recursive();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::recalculate_bounding_radius.
 *
 * @address 0x004f8310
 */
extern "C" void object_recalculate_bounding_radius(uint32_t object_index)
{
    halo::objects::ObjectUpdater(object_index).recalculate_bounding_radius();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::notify_node_array_if_animated.
 *
 * @address 0x004f8b10
 */
extern "C" void object_notify_node_array_if_animated(uint32_t object_index)
{
    halo::objects::ObjectRef(object_index).notify_node_array_if_animated();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::initialize_change_colors.
 *
 * @address 0x004f8b70
 */
extern "C" void object_initialize_change_colors(uint32_t object_index, ColorRGB *colors)
{
    halo::objects::ObjectUpdater(object_index).initialize_change_colors(colors);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::permutation_find_matching_group.
 *
 * @address 0x004f8d80
 */
extern "C" int16_t object_permutation_find_matching_group(ModelRegion *region, int16_t group, int16_t *out)
{
    return halo::objects::ObjectUpdater::permutation_find_matching_group(region, group, out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::regions_initialize_permutations.
 *
 * @address 0x004f8dd0
 */
extern "C" uint8_t object_regions_initialize_permutations(uint32_t object_index, int16_t group, GBXModel *model)
{
    return halo::objects::ObjectUpdater(object_index).regions_initialize_permutations(group, model);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::get_first_region_probability_group.
 *
 * @address 0x004f8ef0
 */
extern "C" int16_t object_get_first_region_probability_group(uint32_t object_index, GBXModel *model)
{
    return halo::objects::ObjectUpdater(object_index).get_first_region_probability_group(model);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::refresh_region_permutations.
 *
 * @address 0x004f8f50
 */
extern "C" void object_refresh_region_permutations(uint32_t object_index)
{
    halo::objects::ObjectUpdater(object_index).refresh_region_permutations();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::remove_from_sibling_list.
 *
 * @address 0x004f8fe0
 */
extern "C" void object_remove_from_sibling_list(datum_index *slot, uint32_t target_object_index)
{
    halo::objects::ObjectRef(target_object_index).remove_from_sibling_list(slot);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::delete_4f9030.
 *
 * @address 0x004f9030
 */
extern "C" void object_delete_4f9030(uint32_t object_index, char recurse_siblings)
{
    halo::objects::ObjectLifetime(object_index).delete_4f9030(recurse_siblings);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::update_change_colors.
 *
 * @address 0x004f9110
 */
extern "C" void object_update_change_colors(uint32_t object_index)
{
    halo::objects::ObjectUpdater(object_index).update_change_colors();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::update_functions.
 *
 * @address 0x004f92f0
 */
extern "C" void object_update_functions(uint32_t object_index)
{
    halo::objects::ObjectUpdater(object_index).update_functions();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::set_scale_and_refresh_nodes.
 *
 * @address 0x004f96a0
 */
extern "C" void object_set_scale_and_refresh_nodes(uint32_t object_index, float scale, int16_t ticks)
{
    halo::objects::ObjectRef(object_index).set_scale_and_refresh_nodes(scale, ticks);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::disconnect_from_map.
 *
 * @address 0x004f96f0
 */
extern "C" uint8_t object_disconnect_from_map(uint32_t object_index)
{
    return halo::objects::ObjectRef(object_index).disconnect_from_map();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::cluster_stamp_mark_visited.
 *
 * @address 0x004f9720
 */
extern "C" uint8_t object_cluster_stamp_mark_visited(datum_index object_index)
{
    return halo::objects::ObjectQueries::cluster_stamp_mark_visited(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::create_attachments.
 *
 * @address 0x004f9750
 */
extern "C" void object_create_attachments(uint32_t object_index)
{
    halo::objects::ObjectLifetime(object_index).create_attachments();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLifetime::delete_attachments.
 *
 * @address 0x004f9900
 */
extern "C" void object_delete_attachments(uint32_t object_index)
{
    halo::objects::ObjectLifetime(object_index).delete_attachments();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectLighting::for_each_light_attachment.
 *
 * @address 0x004f9a20
 */
extern "C" void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
    int32_t invoke_callback)
{
    halo::objects::ObjectLighting(object_index).for_each_light_attachment(register_in_table, invoke_callback);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::reserve_render_cache_slot.
 *
 * @address 0x004f9ac0
 */
extern "C" void object_reserve_render_cache_slot(uint32_t object_index, int16_t slot)
{
    halo::objects::ObjectRef(object_index).reserve_render_cache_slot(slot);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::release_render_cache_slot.
 *
 * @address 0x004f9b00
 */
extern "C" void object_release_render_cache_slot(uint32_t object_index)
{
    halo::objects::ObjectRef(object_index).release_render_cache_slot();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectFactory::create_from_scenario_placement.
 *
 * @address 0x004f9b70
 */
extern "C" datum_index object_new_from_scenario_placement(uint8_t *placement, TagReflexive *palette)
{
    return halo::objects::ObjectFactory::create_from_scenario_placement(placement, palette);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::garbage_collection.
 *
 * @address 0x004f9c60
 */
extern "C" void objects_garbage_collection()
{
    halo::objects::ObjectManager::garbage_collection();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::tree_collect_matching.
 *
 * @address 0x004fa0f0
 */
extern "C" int32_t object_tree_collect_matching(uint32_t object_index, uint8_t (*filter)(uint32_t, void *),
    void *filter_context, int32_t count, int32_t max_count, datum_index *out)
{
    return halo::objects::ObjectQueries::tree_collect_matching(object_index, filter, filter_context, count, max_count, out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::collect_local_player_relevant_objects.
 *
 * @address 0x004fa1a0
 */
extern "C" int32_t object_collect_local_player_relevant_objects(real_point3d *point,
    uint8_t (*filter)(uint32_t, void *), void *filter_context, int32_t max_count, datum_index *out)
{
    return halo::objects::ObjectQueries::collect_local_player_relevant_objects(point, filter, filter_context, max_count, out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectQueries::collect_by_flag_bits.
 *
 * @address 0x004fa280
 */
extern "C" int32_t object_type_definitions_collect_by_flag_bits(int32_t bit_index, int32_t remaining_bits,
    int16_t range_index, int16_t range_count, int32_t *bit_array, int32_t cluster_stamp_snapshot,
    uint8_t (*filter)(uint32_t, void *), void *filter_context, int32_t count, int32_t max_count, datum_index *out)
{
    return halo::objects::ObjectQueries::collect_by_flag_bits(bit_index, remaining_bits, range_index, range_count, bit_array, cluster_stamp_snapshot, filter, filter_context, count, max_count, out);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectMemoryDumpRecordView::compare_by_total_size.
 *
 * @address 0x004fa3a0
 */
extern "C" int object_dump_compare_by_total_size(const object_memory_dump_record *a,
    const object_memory_dump_record *b)
{
    return halo::objects::ObjectMemoryDumpRecordView(const_cast<object_memory_dump_record *>(a)).compare_by_total_size(b);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectMemoryDumpRecordView::accumulate_stats.
 *
 * @address 0x004fa3d0
 */
extern "C" void object_dump_accumulate_stats(uint32_t object_index, object_memory_dump_record *record)
{
    halo::objects::ObjectMemoryDumpRecordView(record).accumulate_stats(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectMemoryDumpRecordView::write.
 *
 * @address 0x004fa490
 */
extern "C" void object_dump_write(object_memory_dump_record *record, void *file)
{
    halo::objects::ObjectMemoryDumpRecordView(record).write(file);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectManager::dump_memory.
 *
 * @address 0x004fa500
 */
extern "C" void objects_dump_memory()
{
    halo::objects::ObjectManager::dump_memory();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::SceneryObject::initialize.
 *
 * @address 0x004fa7e0
 */
extern "C" uint8_t scenery_new(datum_index object_index)
{
    return halo::objects::SceneryObject(object_index).initialize();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::SceneryObject::update.
 *
 * @address 0x004fa870
 */
extern "C" uint8_t scenery_update(datum_index object_index)
{
    return halo::objects::SceneryObject(object_index).update();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::start_animation.
 *
 * @address 0x004fa8d0
 */
extern "C" void object_start_animation(uint32_t object_index, datum_index graph_tag, char *name,
    int16_t requested_frame)
{
    halo::objects::ObjectRef(object_index).start_animation(graph_tag, name, requested_frame);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::animation_get_frames_remaining.
 *
 * @address 0x004fa9b0
 */
extern "C" uint32_t object_animation_get_frames_remaining(uint32_t object_index)
{
    return halo::objects::ObjectRef(object_index).animation_get_frames_remaining();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaSystem::initialize.
 *
 * @address 0x004faa20
 */
extern "C" void antennas_initialize()
{
    halo::objects::AntennaSystem::initialize();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaSystem::dispose.
 *
 * @address 0x004faa40
 */
extern "C" void antennas_dispose()
{
    halo::objects::AntennaSystem::dispose();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaSystem::clear_disposing_flag.
 *
 * @address 0x004faa60
 */
extern "C" void antennas_clear_disposing_flag()
{
    halo::objects::AntennaSystem::clear_disposing_flag();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaSystem::reset_data_pointer.
 *
 * @address 0x004faa70
 */
extern "C" void antennas_reset_data_pointer()
{
    halo::objects::AntennaSystem::reset_data_pointer();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaSystem::create.
 *
 * @address 0x004faa90
 */
extern "C" datum_index antenna_new(datum_index antenna_tag)
{
    return halo::objects::AntennaSystem::create(antenna_tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaSystem::destroy.
 *
 * @address 0x004fac80
 */
extern "C" void antenna_delete(datum_index antenna_index)
{
    halo::objects::AntennaSystem::destroy(antenna_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaSystem::render_callback.
 *
 * @address 0x004fac90
 */
extern "C" void antenna_render_callback(datum_index object_index, datum_index antenna_index)
{
    halo::objects::AntennaSystem::render_callback(object_index, antenna_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaSystem::update.
 *
 * @address 0x004fad20
 */
extern "C" void antennas_update(float dt)
{
    halo::objects::AntennaSystem::update(dt);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaView::update_physics.
 *
 * @address 0x004fae10
 */
extern "C" void antenna_update_physics(antenna *ant, Antenna *antenna_tag, float dt)
{
    halo::objects::AntennaView(ant).update_physics(antenna_tag, dt);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaView::apply_marker_delta.
 *
 * @address 0x004fb1c0
 */
extern "C" void antenna_apply_marker_delta(real_vector3d *out_forward, real_point3d *out_position, antenna *ant,
    Antenna *antenna_tag, bsp_leaf_reference *node_ref)
{
    halo::objects::AntennaView(ant).apply_marker_delta(out_forward, out_position, antenna_tag, node_ref);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaView::render_geometry.
 *
 * @address 0x004fb340
 */
extern "C" void antenna_render_geometry(Antenna *antenna_tag, antenna *ant)
{
    halo::objects::AntennaView(ant).render_geometry(antenna_tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaView::render_wire.
 *
 * @address 0x004fb3e0
 */
extern "C" void antenna_render_wire(uint32_t widget_flags, float scale, Antenna *antenna_tag, antenna *ant)
{
    halo::objects::AntennaView(ant).render_wire(widget_flags, scale, antenna_tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagSystem::initialize.
 *
 * @address 0x004fb4d0
 */
extern "C" void flags_initialize()
{
    halo::objects::FlagSystem::initialize();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagSystem::dispose.
 *
 * @address 0x004fb4f0
 */
extern "C" void flags_dispose()
{
    halo::objects::FlagSystem::dispose();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagSystem::clear_disposing_flag.
 *
 * @address 0x004fb510
 */
extern "C" void flags_clear_disposing_flag()
{
    halo::objects::FlagSystem::clear_disposing_flag();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagSystem::reset_data_pointer.
 *
 * @address 0x004fb520
 */
extern "C" void flags_reset_data_pointer()
{
    halo::objects::FlagSystem::reset_data_pointer();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagSystem::create.
 *
 * @address 0x004fb540
 */
extern "C" datum_index flag_new(datum_index flag_tag)
{
    return halo::objects::FlagSystem::create(flag_tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagView::cloth_mark_border_cells.
 *
 * @address 0x004fb6d0
 */
extern "C" void flag_cloth_mark_border_cells(flag *entry, Flag *tag)
{
    halo::objects::FlagView(entry).cloth_mark_border_cells(tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagView::cloth_init_shape_constraints.
 *
 * @address 0x004fb770
 */
extern "C" void flag_cloth_init_shape_constraints(flag *entry, Flag *tag)
{
    halo::objects::FlagView(entry).cloth_init_shape_constraints(tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagView::cloth_stamp_region_split_flags.
 *
 * @address 0x004fb840
 */
extern "C" void flag_cloth_stamp_region_split_flags(int16_t outer_start, Flag *tag, flag *entry, int16_t inner_start,
    int16_t size, uint16_t split_code)
{
    halo::objects::FlagView(entry).cloth_stamp_region_split_flags(outer_start, tag, inner_start, size, split_code);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagSystem::destroy.
 *
 * @address 0x004fb970
 */
extern "C" void flag_delete(datum_index flag_index)
{
    halo::objects::FlagSystem::destroy(flag_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagSystem::render_callback.
 *
 * @address 0x004fb980
 */
extern "C" void flag_render_callback(datum_index object_index, datum_index flag_index, uint32_t arg3, uint32_t arg4)
{
    halo::objects::FlagSystem::render_callback(object_index, flag_index, arg3, arg4);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagSystem::update.
 *
 * @address 0x004fba00
 */
extern "C" void flags_update(float dt)
{
    halo::objects::FlagSystem::update(dt);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagView::cloth_update.
 *
 * @address 0x004fbae0
 */
extern "C" void flag_cloth_update(flag *entry, Flag *tag, float dt)
{
    halo::objects::FlagView(entry).cloth_update(tag, dt);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagView::pole_get_marker_positions.
 *
 * @address 0x004fc020
 */
extern "C" void flag_pole_get_marker_positions(flag *entry, bsp_leaf_reference *node_ref,
    real_point3d *marker_positions, uint8_t *row_table, int16_t *row_start_scratch, int16_t *column_marker_index,
    Flag *tag)
{
    halo::objects::FlagView(entry).pole_get_marker_positions(node_ref, marker_positions, row_table, row_start_scratch, column_marker_index, tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::FlagSystem::render.
 *
 * @address 0x004fc350
 */
extern "C" void flag_render(uint32_t *entry, uint32_t *submission_block, Flag *tag, uint8_t *second_geometry)
{
    halo::objects::FlagSystem::render(entry, submission_block, tag, second_geometry);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowSystem::initialize.
 *
 * @address 0x004fcbb0
 */
extern "C" void glow_initialize()
{
    halo::objects::GlowSystem::initialize();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowSystem::dispose.
 *
 * @address 0x004fcc00
 */
extern "C" void glow_dispose()
{
    halo::objects::GlowSystem::dispose();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowSystem::clear_disposing_flag.
 *
 * @address 0x004fcc30
 */
extern "C" void glow_clear_disposing_flag()
{
    halo::objects::GlowSystem::clear_disposing_flag();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowSystem::create.
 *
 * @address 0x004fcc50
 */
extern "C" datum_index glow_new(datum_index glow_tag)
{
    return halo::objects::GlowSystem::create(glow_tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowSystem::destroy.
 *
 * @address 0x004fcd40
 */
extern "C" void glow_delete(datum_index glow_index)
{
    halo::objects::GlowSystem::destroy(glow_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowSystem::render_dispatch.
 *
 * @address 0x004fcdb0
 */
extern "C" void glow_render_dispatch(uint32_t object_index, datum_index glow_handle)
{
    halo::objects::GlowSystem::render_dispatch(object_index, glow_handle);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowView::update.
 *
 * @address 0x004fce80
 */
extern "C" void glow_update(uint32_t object_index, glow *entry)
{
    halo::objects::GlowView(entry).update(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowParticleView::compute_fade.
 *
 * @address 0x004fd3a0
 */
extern "C" void glow_particle_compute_fade(glow *entry, glow_particle *particle)
{
    halo::objects::GlowParticleView(particle).compute_fade(entry);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowParticleView::compute_color.
 *
 * @address 0x004fd420
 */
extern "C" void glow_particle_compute_color(glow *entry, glow_particle *particle)
{
    halo::objects::GlowParticleView(particle).compute_color(entry);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowParticleView::compute_position.
 *
 * @address 0x004fd4a0
 */
extern "C" void glow_particle_compute_position(uint32_t object_index, glow *entry, glow_particle *particle)
{
    halo::objects::GlowParticleView(particle).compute_position(object_index, entry);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowView::particle_advance_time.
 *
 * @address 0x004fd650
 */
extern "C" void glow_particle_advance_time(uint32_t object_index, glow *entry, uint8_t *particle, float rate)
{
    halo::objects::GlowView(entry).particle_advance_time(object_index, particle, rate);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowView::chain_build.
 *
 * @address 0x004fd830
 */
extern "C" void glow_chain_build(glow *entry)
{
    halo::objects::GlowView(entry).chain_build();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowView::particle_new.
 *
 * @address 0x004fd8e0
 */
extern "C" glow_particle * glow_particle_new(glow *entry, int16_t index, int16_t count)
{
    return halo::objects::GlowView(entry).particle_new(index, count);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowView::particle_spawn.
 *
 * @address 0x004fdb20
 */
extern "C" glow_particle * glow_particle_spawn(glow *entry)
{
    return halo::objects::GlowView(entry).particle_spawn();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowSystem::particle_datum_new.
 *
 * @address 0x004fdde0
 */
extern "C" glow_particle * glow_particle_datum_new()
{
    return halo::objects::GlowSystem::particle_datum_new();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowView::particle_reposition.
 *
 * @address 0x004fde40
 */
extern "C" void glow_particle_reposition(glow *entry, uint8_t *particle, float phase_rate)
{
    halo::objects::GlowView(entry).particle_reposition(particle, phase_rate);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::GlowSystem::render.
 *
 * @address 0x004fe570
 */
extern "C" void glow_render(datum_index glow_handle)
{
    halo::objects::GlowSystem::render(glow_handle);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightVolumeSystem::initialize.
 *
 * @address 0x004fe680
 */
extern "C" void light_volumes_initialize()
{
    halo::objects::LightVolumeSystem::initialize();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightVolumeSystem::dispose.
 *
 * @address 0x004fe6a0
 */
extern "C" void light_volumes_dispose()
{
    halo::objects::LightVolumeSystem::dispose();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightVolumeSystem::clear_disposing_flag.
 *
 * @address 0x004fe6c0
 */
extern "C" void light_volumes_clear_disposing_flag()
{
    halo::objects::LightVolumeSystem::clear_disposing_flag();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightVolumeSystem::create.
 *
 * @address 0x004fe6d0
 */
extern "C" datum_index light_volume_new(datum_index definition_tag)
{
    return halo::objects::LightVolumeSystem::create(definition_tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightVolumeSystem::destroy.
 *
 * @address 0x004fe720
 */
extern "C" void light_volume_delete(datum_index light_volume_index)
{
    halo::objects::LightVolumeSystem::destroy(light_volume_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectRef::attachment_get_blended_marker.
 *
 * @address 0x004fe740
 */
extern "C" uint8_t * object_attachment_get_blended_marker(uint32_t object_index, uint8_t *instance)
{
    return halo::objects::ObjectRef(object_index).attachment_get_blended_marker(instance);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightVolumeSystem::render.
 *
 * @address 0x004fe900
 */
extern "C" void light_volume_render(uint32_t object_index, datum_index light_volume_handle, uint32_t unused,
    uint8_t *function_context)
{
    halo::objects::LightVolumeSystem::render(object_index, light_volume_handle, unused, function_context);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::ObjectUpdater::curve_apply_exponent.
 *
 * @address 0x004fea50
 */
extern "C" float curve_apply_exponent(float value, float exponent)
{
    return halo::objects::ObjectUpdater::curve_apply_exponent(value, exponent);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightVolumeSystem::render_procedure.
 *
 * @address 0x004fea80
 */
extern "C" void light_volume_render_procedure(uint32_t object_index, datum_index light_volume_handle)
{
    halo::objects::LightVolumeSystem::render_procedure(object_index, light_volume_handle);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightningSystem::initialize.
 *
 * @address 0x004fee80
 */
extern "C" void lightnings_initialize()
{
    halo::objects::LightningSystem::initialize();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightningSystem::dispose.
 *
 * @address 0x004feea0
 */
extern "C" void lightnings_dispose()
{
    halo::objects::LightningSystem::dispose();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightningSystem::clear_disposing_flag.
 *
 * @address 0x004feec0
 */
extern "C" void lightnings_clear_disposing_flag()
{
    halo::objects::LightningSystem::clear_disposing_flag();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightningSystem::create.
 *
 * @address 0x004feed0
 */
extern "C" datum_index lightning_new(datum_index definition_tag)
{
    return halo::objects::LightningSystem::create(definition_tag);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightningSystem::destroy.
 *
 * @address 0x004fef20
 */
extern "C" void lightning_delete(datum_index lightning_index)
{
    halo::objects::LightningSystem::destroy(lightning_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::AntennaSystem::tip_jitter.
 *
 * @address 0x004fef40
 */
extern "C" void antenna_tip_jitter(real_vector3d *amplitude, real_point3d *position, real_matrix4x3 *m)
{
    halo::objects::AntennaSystem::tip_jitter(amplitude, position, m);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::LightningSystem::render.
 *
 * @address 0x004ff010
 */
extern "C" void lightning_render(uint32_t object_index, datum_index lightning_handle, uint32_t unused,
    int32_t *function_context)
{
    halo::objects::LightningSystem::render(object_index, lightning_handle, unused, function_context);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::WidgetSystem::initialize.
 *
 * @address 0x004ff9d0
 */
extern "C" void widgets_initialize()
{
    halo::objects::WidgetSystem::initialize();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::WidgetSystem::dispose.
 *
 * @address 0x004ffa10
 */
extern "C" void widgets_dispose()
{
    halo::objects::WidgetSystem::dispose();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::WidgetSystem::dispose_clear_flag.
 *
 * @address 0x004ffa50
 */
extern "C" void widgets_dispose_clear_flag()
{
    halo::objects::WidgetSystem::dispose_clear_flag();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::WidgetSystem::create.
 *
 * @address 0x004ffa80
 */
extern "C" void widget_new(uint32_t object_index)
{
    halo::objects::WidgetSystem::create(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::WidgetSystem::delete_all.
 *
 * @address 0x004ffbe0
 */
extern "C" void widget_delete_all(uint32_t object_index)
{
    halo::objects::WidgetSystem::delete_all(object_index);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::WidgetSystem::list_has_flag.
 *
 * @address 0x004ffc60
 */
extern "C" int8_t widget_list_has_flag(datum_index first_widget)
{
    return halo::objects::WidgetSystem::list_has_flag(first_widget);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::WidgetSystem::list_notify.
 *
 * @address 0x004ffca0
 */
extern "C" void widget_list_notify(uint32_t object_index, uint32_t render_arg, void *render_context)
{
    halo::objects::WidgetSystem::list_notify(object_index, render_arg, render_context);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::WidgetSystem::update_all.
 *
 * @address 0x004ffd10
 */
extern "C" void widgets_update_all(float dt)
{
    halo::objects::WidgetSystem::update_all(dt);
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageSystem::breakable_surfaces_reset.
 *
 * @address 0x004ffd40
 */
extern "C" void breakable_surfaces_reset()
{
    halo::objects::DamageSystem::breakable_surfaces_reset();
}

/**
 * C entry point kept for the link tables and unconverted modules; forwards to halo::objects::DamageSystem::breakable_surface_is_intact.
 *
 * @address 0x004ffda0
 */
extern "C" int8_t breakable_surface_is_intact(int16_t bit_index)
{
    return halo::objects::DamageSystem::breakable_surface_is_intact(bit_index);
}
