/**
 * @file include/halo/objects/vars.hpp
 * Addresses of the engine variables the objects module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/objects_vars.hpp.
 */
#pragma once

namespace halo::objects {

/** Address table of the engine variables owned by the objects module. */
struct Vars {
    void *ai_gc_callback_table;
    void *antenna_data;
    void *antenna_sprite_shader;
    void *camera_forward_y;
    void *camera_forward_z;
    void *collideable_cluster_first;
    void *collideable_cluster_partition;
    void *default_collision_material;
    void *flag_data;
    void *flag_render_device_slot;
    void *g_006f1cf4;
    void *glow_data;
    void *glow_particle_data;
    void *glow_sprite_shader;
    void *light_active_list;
    void *light_active_list_count;
    void *light_cluster_first;
    void *light_cluster_references;
    void *light_data;
    void *light_frame_counter;
    void *light_object_references;
    void *light_render_unknown_7c0;
    void *light_transient_count;
    void *light_transient_count_or_queue;
    void *light_transient_table;
    void *light_volume_instances;
    void *lightning_instances;
    void *lights_enabled;
    void *network_action_apply_active;
    void *noncollideable_cluster_first;
    void *noncollideable_cluster_partition;
    void *noncollideable_object_references;
    void *object_ambient_lightmap_default;
    void *object_data;
    void *object_delete_callbacks;
    void *object_globals_pointer;
    void *object_lighting_ambient_bias;
    void *object_lighting_ambient_scale;
    void *object_lighting_base_light_scale;
    void *object_lightmap_probe_direction;
    void *object_list_header_data;
    void *object_list_reference_data;
    void *object_marker_scratch;
    void *object_memory_pool;
    void *object_name_list;
    void *object_sound_event_last_tick;
    void *object_type_definition_list;
    void *object_unknown_006b8c60;
    void *object_visibility_computed_mask;
    void *rasterizer_light_count;
    void *rasterizer_lights;
    void *shared_constant_vector_696704;
    void *widget_data;
    void *widget_type_definitions;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::objects
