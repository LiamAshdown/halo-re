#include "tags.h"
#include "memory.h"
#include "math.h"
#include "models.h"

#define CHECK(name, cond) typedef char check_##name[(cond) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

// struct sizes
CHECK(real_orientation, sizeof(real_orientation) == 0x20);
CHECK(animation_state, sizeof(animation_state) == 0x04);
CHECK(animation_quaternion48, sizeof(animation_quaternion48) == 0x06);
CHECK(animation_compressed_header, sizeof(animation_compressed_header) == 0x2c);
CHECK(animation_aiming_screen, sizeof(animation_aiming_screen) == 0x18);
CHECK(model_part_group_link, sizeof(model_part_group_link) == 0x10);

// real_orientation (0x4d7610 field writes, 0x4d6880 / 0x4d7690 reads)
CHECK(ro_rotation_w, OFF(real_orientation, rotation.w) == 0x0c);
CHECK(ro_translation, OFF(real_orientation, translation) == 0x10);
CHECK(ro_scale, OFF(real_orientation, scale) == 0x1c);
CHECK(ro_stack_array, k_maximum_nodes_per_model * sizeof(real_orientation) == 0x800);
CHECK(matrix_stack_array, k_maximum_nodes_per_model * sizeof(real_matrix4x3) == 0xd00);

// animation_state (0x4d48d0 ESI)
CHECK(as_frame, OFF(animation_state, frame_index) == 0x02);

// animation_compressed_header (0x4d6b60, 0x4d6cf0, 0x4d6e80)
CHECK(ach_rot_defaults, OFF(animation_compressed_header, rotation_defaults) == 0x04);
CHECK(ach_rot_keys, OFF(animation_compressed_header, rotation_keyframes) == 0x08);
CHECK(ach_tr_headers, OFF(animation_compressed_header, translation_keyframe_headers) == 0x0c);
CHECK(ach_tr_times, OFF(animation_compressed_header, translation_keyframe_times) == 0x10);
CHECK(ach_tr_defaults, OFF(animation_compressed_header, translation_defaults) == 0x14);
CHECK(ach_tr_keys, OFF(animation_compressed_header, translation_keyframes) == 0x18);
CHECK(ach_sc_headers, OFF(animation_compressed_header, scale_keyframe_headers) == 0x1c);
CHECK(ach_sc_times, OFF(animation_compressed_header, scale_keyframe_times) == 0x20);
CHECK(ach_sc_defaults, OFF(animation_compressed_header, scale_defaults) == 0x24);
CHECK(ach_sc_keys, OFF(animation_compressed_header, scale_keyframes) == 0x28);

// animation_aiming_screen (0x4d5c00) against the three tag blocks that carry it
CHECK(aim_left, OFF(animation_aiming_screen, left_yaw_per_frame) == 0x04);
CHECK(aim_right_count, OFF(animation_aiming_screen, right_frame_count) == 0x08);
CHECK(aim_left_count, OFF(animation_aiming_screen, left_frame_count) == 0x0a);
CHECK(aim_down, OFF(animation_aiming_screen, down_pitch_per_frame) == 0x0c);
CHECK(aim_up, OFF(animation_aiming_screen, up_pitch_per_frame) == 0x10);
CHECK(aim_down_count, OFF(animation_aiming_screen, down_pitch_frame_count) == 0x14);
CHECK(aim_up_count, OFF(animation_aiming_screen, up_pitch_frame_count) == 0x16);
CHECK(aim_seat, OFF(ModelAnimationsAnimationGraphUnitSeat, right_yaw_per_frame) == 0x20
      && OFF(ModelAnimationsAnimationGraphUnitSeat, up_pitch_frame_count) == 0x20 + 0x16);
CHECK(aim_weapon, OFF(ModelAnimationsAnimationGraphWeapon, right_yaw_per_frame) == 0x60
      && OFF(ModelAnimationsAnimationGraphWeapon, up_pitch_frame_count) == 0x60 + 0x16);
CHECK(aim_vehicle, OFF(ModelAnimationsAnimationGraphVehicleAnimations, up_pitch_frame_count) == 0x16);

// model_part_group_link (0x4d72a0 esp+0x38 + r*0x10)
CHECK(pgl_next, OFF(model_part_group_link, next_group_index) == 0x04);
CHECK(pgl_group, OFF(model_part_group_link, group_index) == 0x08);
CHECK(pgl_linked, OFF(model_part_group_link, linked_part_index) == 0x0a);
CHECK(pgl_part, OFF(model_part_group_link, part_index) == 0x0c);

// tags.h offsets this module relies on (re-derived from the arithmetic)
CHECK(anim_type, OFF(ModelAnimationsAnimation, type) == 0x20);
CHECK(anim_frame_count, OFF(ModelAnimationsAnimation, frame_count) == 0x22);
CHECK(anim_frame_size, OFF(ModelAnimationsAnimation, frame_size) == 0x24);
CHECK(anim_frame_info_type, OFF(ModelAnimationsAnimation, frame_info_type) == 0x26);
CHECK(anim_checksum, OFF(ModelAnimationsAnimation, node_list_checksum) == 0x28);
CHECK(anim_node_count, OFF(ModelAnimationsAnimation, node_count) == 0x2c);
CHECK(anim_loop, OFF(ModelAnimationsAnimation, loop_frame_index) == 0x2e);
CHECK(anim_key, OFF(ModelAnimationsAnimation, key_frame_index) == 0x34);
CHECK(anim_key2, OFF(ModelAnimationsAnimation, second_key_frame_index) == 0x36);
CHECK(anim_next, OFF(ModelAnimationsAnimation, next_animation) == 0x38);
CHECK(anim_flags, OFF(ModelAnimationsAnimation, flags) == 0x3a);
CHECK(anim_sound, OFF(ModelAnimationsAnimation, sound) == 0x3c);
CHECK(anim_sound_frame, OFF(ModelAnimationsAnimation, sound_frame_index) == 0x3e);
CHECK(anim_main, OFF(ModelAnimationsAnimation, main_animation_index) == 0x42);
CHECK(anim_weight, OFF(ModelAnimationsAnimation, relative_weight) == 0x44);
CHECK(anim_frame_info, OFF(ModelAnimationsAnimation, frame_info.pointer) == 0x54);
CHECK(anim_trans_bits, OFF(ModelAnimationsAnimation, node_transform_flag_data) == 0x5c);
CHECK(anim_rot_bits, OFF(ModelAnimationsAnimation, node_rotation_flag_data) == 0x6c);
CHECK(anim_scale_bits, OFF(ModelAnimationsAnimation, node_scale_flag_data) == 0x7c);
CHECK(anim_compressed, OFF(ModelAnimationsAnimation, offset_to_compressed_data) == 0x88);
CHECK(anim_default, OFF(ModelAnimationsAnimation, default_data.pointer) == 0x98);
CHECK(anim_frames, OFF(ModelAnimationsAnimation, frame_data.pointer) == 0xac);
CHECK(anim_size, sizeof(ModelAnimationsAnimation) == 0xb4);
CHECK(graph_sounds, OFF(ModelAnimations, sound_references.pointer) == 0x58);
CHECK(graph_nodes, OFF(ModelAnimations, nodes.count) == 0x68);
CHECK(graph_nodes_ptr, OFF(ModelAnimations, nodes.pointer) == 0x6c);
CHECK(graph_anims, OFF(ModelAnimations, animations.count) == 0x74);
CHECK(graph_anims_ptr, OFF(ModelAnimations, animations.pointer) == 0x78);
CHECK(graph_vehicles_ptr, OFF(ModelAnimations, vehicles.pointer) == 0x28);
CHECK(graph_node_size, sizeof(ModelAnimationsAnimationGraphNode) == 0x40);
CHECK(graph_node_sibling, OFF(ModelAnimationsAnimationGraphNode, next_sibling_node_index) == 0x20);
CHECK(graph_node_child, OFF(ModelAnimationsAnimationGraphNode, first_child_node_index) == 0x22);
CHECK(graph_node_parent, OFF(ModelAnimationsAnimationGraphNode, parent_node_index) == 0x24);
CHECK(sound_ref_size, sizeof(ModelAnimationsAnimationGraphSoundReference) == 0x14);
CHECK(sound_ref_id, OFF(ModelAnimationsAnimationGraphSoundReference, sound.tag_id) == 0x0c);
CHECK(model_checksum, OFF(GBXModel, node_list_checksum) == 0x04);
CHECK(model_cutoff0, OFF(GBXModel, super_high_detail_cutoff) == 0x08);
CHECK(model_cutoff4, OFF(GBXModel, super_low_detail_cutoff) == 0x18);
CHECK(model_u_scale, OFF(GBXModel, base_map_u_scale) == 0x30);
CHECK(model_v_scale, OFF(GBXModel, base_map_v_scale) == 0x34);
CHECK(model_markers, OFF(GBXModel, markers.count) == 0xac);
CHECK(model_markers_ptr, OFF(GBXModel, markers.pointer) == 0xb0);
CHECK(model_nodes, OFF(GBXModel, nodes.count) == 0xb8);
CHECK(model_nodes_ptr, OFF(GBXModel, nodes.pointer) == 0xbc);
CHECK(model_regions, OFF(GBXModel, regions.count) == 0xc4);
CHECK(model_regions_ptr, OFF(GBXModel, regions.pointer) == 0xc8);
CHECK(model_geometries_ptr, OFF(GBXModel, geometries.pointer) == 0xd4);
CHECK(model_shaders_ptr, OFF(GBXModel, shaders.pointer) == 0xe0);
CHECK(node_size, sizeof(ModelNode) == 0x9c);
CHECK(node_default_translation, OFF(ModelNode, default_translation) == 0x28);
CHECK(node_default_rotation, OFF(ModelNode, default_rotation) == 0x34);
CHECK(node_bind_matrix, OFF(ModelNode, scale) == 0x68 && sizeof(ModelNode) - 0x68 == sizeof(real_matrix4x3));
CHECK(marker_size, sizeof(ModelMarker) == 0x40);
CHECK(marker_instances, OFF(ModelMarker, instances.count) == 0x34);
CHECK(marker_instances_ptr, OFF(ModelMarker, instances.pointer) == 0x38);
CHECK(marker_instance_size, sizeof(ModelMarkerInstance) == 0x20);
CHECK(marker_instance_node, OFF(ModelMarkerInstance, node_index) == 0x02);
CHECK(marker_instance_translation, OFF(ModelMarkerInstance, translation) == 0x04);
CHECK(marker_instance_rotation, OFF(ModelMarkerInstance, rotation) == 0x10);
CHECK(region_size, sizeof(ModelRegion) == 0x4c);
CHECK(region_perms_ptr, OFF(ModelRegion, permutations.pointer) == 0x44);
CHECK(perm_size, sizeof(ModelRegionPermutation) == 0x58);
CHECK(perm_lod0, OFF(ModelRegionPermutation, super_low) == 0x40);
CHECK(perm_lod4, OFF(ModelRegionPermutation, super_high) == 0x48);
CHECK(geometry_size, sizeof(GBXModelGeometry) == 0x30);
CHECK(geometry_parts, OFF(GBXModelGeometry, parts.count) == 0x24);
CHECK(geometry_parts_ptr, OFF(GBXModelGeometry, parts.pointer) == 0x28);
CHECK(part_size, sizeof(GBXModelGeometryPart) == 0x84);
CHECK(part_shader, OFF(ModelGeometryPart, shader_index) == 0x04);
CHECK(part_prev_filthy, OFF(ModelGeometryPart, prev_filthy_part_index) == 0x06);
CHECK(part_next_filthy, OFF(ModelGeometryPart, next_filthy_part_index) == 0x07);
CHECK(part_centroid_node, OFF(ModelGeometryPart, centroid_primary_node) == 0x08);
CHECK(part_centroid, OFF(ModelGeometryPart, centroid) == 0x14);
CHECK(part_triangle_buffer, OFF(ModelGeometryPart, triangle_buffer_type) == 0x44);
CHECK(part_triangle_count, OFF(ModelGeometryPart, triangle_count) == 0x48);
CHECK(part_vertex_buffer, OFF(ModelGeometryPart, vertex_type) == 0x54);
CHECK(shader_ref_size, sizeof(ModelShaderReference) == 0x20);
CHECK(shader_ref_id, OFF(ModelShaderReference, shader.tag_id) == 0x0c);
CHECK(shader_ref_perm, OFF(ModelShaderReference, permutation) == 0x10);
CHECK(shader_type, OFF(Shader, shader_type) == 0x24);
CHECK(shader_model_flags, OFF(ShaderModel, shader_model_flags) == 0x28);

int main(void) { return 0; }
