// Phase 4 syntax gate for types/objects.h. The host gcc is 64-bit, so struct sizes here are
// the 32-bit sizes plus 4 per pointer; every size in objects.h was checked against that rule
// by hand. math.h is included because objects.h uses real_point3d / real_vector3d /
// real_matrix4x3 rather than redefining them.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
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
             + sizeof(glow) + sizeof(glow_particle) + sizeof(object_zone_light_table)
             + sizeof(bsp_leaf_reference) + sizeof(object_placement_cursor)
             + sizeof(object_statistics) + sizeof(damage_effect_vector_block)
             + sizeof(object_shield_impulse_result)
             + sizeof(data_array) + sizeof(real_matrix4x3) + sizeof(Object)
             + sizeof(GBXModel) + sizeof(ModelCollisionGeometryRegion)
             + (int)_object_widget_type_glow + (int)_object_function_input_d_out
             + (int)_object_ambient_cluster_override + (int)_light_attached_bit
             + (int)k_maximum_objects + (int)k_maximum_widget_types);
}
