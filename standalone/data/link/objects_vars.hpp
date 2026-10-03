/**
 * @file standalone/data/link/objects_vars.hpp
 * Link names of the engine variables owned by the objects module (halo::objects::vars()). The data image defines them under these C
 * names; only src/objects/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char ai_gc_callback_table[];
extern char antenna_data[];
extern char antenna_sprite_shader[];
extern char camera_forward_y[];
extern char camera_forward_z[];
extern char collideable_cluster_first[];
extern char collideable_cluster_partition[];
extern char default_collision_material[];
extern char flag_data[];
extern char flag_render_device_slot[];
extern char g_006f1cf4[];
extern char glow_data[];
extern char glow_particle_data[];
extern char glow_sprite_shader[];
extern char light_active_list[];
extern char light_active_list_count[];
extern char light_cluster_first[];
extern char light_cluster_references[];
extern char light_data[];
extern char light_frame_counter[];
extern char light_object_references[];
extern char light_render_unknown_7c0[];
extern char light_transient_count[];
extern char light_transient_count_or_queue[];
extern char light_transient_table[];
extern char light_volume_instances[];
extern char lightning_instances[];
extern char lights_enabled[];
extern char network_action_apply_active[];
extern char noncollideable_cluster_first[];
extern char noncollideable_cluster_partition[];
extern char noncollideable_object_references[];
extern char object_ambient_lightmap_default[];
extern char object_data[];
extern char object_delete_callbacks[];
extern char object_globals_pointer[];
extern char object_lighting_ambient_bias[];
extern char object_lighting_ambient_scale[];
extern char object_lighting_base_light_scale[];
extern char object_lightmap_probe_direction[];
extern char object_list_header_data[];
extern char object_list_reference_data[];
extern char object_marker_scratch[];
extern char object_memory_pool[];
extern char object_name_list[];
extern char object_sound_event_last_tick[];
extern char object_type_definition_list[];
extern char object_unknown_006b8c60[];
extern char object_visibility_computed_mask[];
extern char rasterizer_light_count[];
extern char rasterizer_lights[];
extern char shared_constant_vector_696704[];
extern char widget_data[];
extern char widget_type_definitions[];
}
