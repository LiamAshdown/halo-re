// Phase 4 syntax gate for types/objects.h. The host gcc is 64-bit, so struct sizes here are
// the 32-bit sizes plus 4 per pointer; every size in objects.h was checked against that rule
// by hand. math.h is included because objects.h uses real_point3d / real_vector3d /
// real_matrix4x3 rather than redefining them.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

// Offsets pinned by the phase 4 reconciliation (object has no pointers, so these hold for the
// host and -m32 alike; object_type_definition carries pointers, so its checks are 32-bit only).
#define CHK(name, cond) typedef char chk_##name[(cond) ? 1 : -1]
CHK(object_size, sizeof(object) == 0x1f4);
CHK(object_network_state_009, __builtin_offsetof(object, network_state_009) == 0x009);
CHK(object_network_update_tick, __builtin_offsetof(object, network_update_tick) == 0x00c);
CHK(object_network_position_valid, __builtin_offsetof(object, network_position_valid) == 0x018);
CHK(object_network_position, __builtin_offsetof(object, network_position) == 0x01c);
CHK(object_network_velocity_valid, __builtin_offsetof(object, network_velocity_valid) == 0x044);
CHK(object_network_velocity, __builtin_offsetof(object, network_velocity) == 0x048);
CHK(object_network_timestamp_valid, __builtin_offsetof(object, network_timestamp_valid) == 0x054);
CHK(object_network_timestamp, __builtin_offsetof(object, network_timestamp) == 0x058);
CHK(object_position, __builtin_offsetof(object, position) == 0x05c);
CHK(object_owner_team, __builtin_offsetof(object, owner_team) == 0x0b8);
CHK(object_creator_object, __builtin_offsetof(object, creator_object) == 0x0c4);
CHK(object_cached_render_state, __builtin_offsetof(object, cached_render_state_index) == 0x170);
CHK(placement_owner_team, __builtin_offsetof(object_placement_data, owner_team) == 0x14);
CHK(damage_material_type, __builtin_offsetof(damage_data, material_type) == 0x4c);
CHK(damage_size, sizeof(damage_data) == 0x54);
CHK(marker_node_transform, __builtin_offsetof(object_marker, node_transform) == 0x38);
CHK(marker_mirrored_left, __builtin_offsetof(object_marker, node_transform.left) == 0x48);
#if __SIZEOF_POINTER__ == 4
CHK(otd_group_tag, __builtin_offsetof(object_type_definition, group_tag) == 0x04);
CHK(otd_placement_offset, __builtin_offsetof(object_type_definition, scenario_placement_offset) == 0x0a);
CHK(otd_palette_offset, __builtin_offsetof(object_type_definition, scenario_palette_offset) == 0x0c);
CHK(otd_placement_size, __builtin_offsetof(object_type_definition, scenario_placement_size) == 0x0e);
CHK(otd_message_type, __builtin_offsetof(object_type_definition, network_delta_message_type) == 0x10);
CHK(otd_initialize, __builtin_offsetof(object_type_definition, initialize) == 0x14);
#endif
int main(void){
  object o;
  object_header header;
  object_placement_data placement;
  damage_data damage;
  object_iterator iterator;
  o.type = _object_type_biped;
  o.flags = _object_mirrored_geometry_bit | _object_connected_to_map_bit;
  o.vitality_flags = _object_shield_recharging_bit | _object_hash_flag_bit;
  o.attachment_types[0] = _object_attachment_type_light;
  header.flags = _object_header_active_bit | _object_header_delete_pending_bit;
  placement.height_above_origin = 0.0f;
  damage.multiplier = 1.0f;
  iterator.type_mask = _object_mask_unit;
  return (int)(sizeof(o) + sizeof(header) + sizeof(placement) + sizeof(damage)
             + sizeof(iterator) + sizeof(object_type_definition)
             + sizeof(object_block_reference) + sizeof(object_globals)
             + sizeof(object_cluster_reference) + sizeof(object_marker)
             + sizeof(object_memory_dump_record) + sizeof(hash_table) + sizeof(hash_bucket)
             + sizeof(hash_node) + sizeof(hash_node_block) + sizeof(light)
             + sizeof(light_transient) + sizeof(widget) + sizeof(widget_type_definition)
             + sizeof(antenna) + sizeof(antenna_vertex) + sizeof(flag) + sizeof(flag_vertex)
             + sizeof(glow) + sizeof(glow_particle)
             + sizeof(bsp_leaf_reference) + sizeof(object_placement_cursor)
             + sizeof(object_statistics) + sizeof(damage_effect_vector_block)
             + sizeof(object_shield_impulse_result)
             + sizeof(data_array) + sizeof(real_matrix4x3) + sizeof(Object)
             + sizeof(GBXModel) + sizeof(ModelCollisionGeometryRegion)
             + (int)_object_widget_type_glow + (int)_object_function_input_d_out
             + (int)_object_ambient_cluster_override + (int)_light_attached_bit
             + (int)k_maximum_objects + (int)k_maximum_widget_types);
}
