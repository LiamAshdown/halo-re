/**
 * @file src/objects/vars.cpp
 * Binds halo::objects::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/objects/vars.hpp"
#include "link/objects_vars.hpp"
#include "halo/objects/api.hpp"

namespace halo::objects {

const Vars &vars()
{
    static const Vars table{
        ai_gc_callback_table,
        antenna_data,
        antenna_sprite_shader,
        camera_forward_y,
        camera_forward_z,
        collideable_cluster_first,
        collideable_cluster_partition,
        default_collision_material,
        flag_data,
        flag_render_device_slot,
        g_006f1cf4,
        glow_data,
        glow_particle_data,
        glow_sprite_shader,
        light_active_list,
        light_active_list_count,
        light_cluster_first,
        light_cluster_references,
        light_data,
        light_frame_counter,
        light_object_references,
        light_render_unknown_7c0,
        light_transient_count,
        light_transient_count_or_queue,
        light_transient_table,
        light_volume_instances,
        lightning_instances,
        lights_enabled,
        network_action_apply_active,
        noncollideable_cluster_first,
        noncollideable_cluster_partition,
        noncollideable_object_references,
        object_ambient_lightmap_default,
        object_data,
        object_delete_callbacks,
        object_globals_pointer,
        object_lighting_ambient_bias,
        object_lighting_ambient_scale,
        object_lighting_base_light_scale,
        object_lightmap_probe_direction,
        object_list_header_data,
        object_list_reference_data,
        object_marker_scratch,
        object_memory_pool,
        object_name_list,
        object_sound_event_last_tick,
        object_type_definition_list,
        object_unknown_006b8c60,
        object_visibility_computed_mask,
        rasterizer_light_count,
        rasterizer_lights,
        shared_constant_vector_696704,
        widget_data,
        widget_type_definitions,
    };
    return table;
}

}  // namespace halo::objects
