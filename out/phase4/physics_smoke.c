#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "projectiles.h"
#include "physics.h"

// Host gcc is 64-bit by default, where a pointer member is 8 bytes instead of the 4 bytes
// halo.exe uses. Checks on the structs that hold pointers are therefore only exercised when
// the compiler is targeting 32 bits (add -m32); everything else is checked either way.
#define PTRS32 (sizeof(void *) == 4)

// ---------------------------------------------------------------------------
// struct sizes
// ---------------------------------------------------------------------------
typedef char check_sphere_query[(!PTRS32 || sizeof(collision_bsp_sphere_query) == 0x228) ? 1 : -1];
typedef char check_sphere_result[(sizeof(collision_bsp_sphere_result) == 0x1010) ? 1 : -1];
typedef char check_segment_query[(!PTRS32 || sizeof(collision_bsp_segment_query) == 0x28) ? 1 : -1];
typedef char check_segment_result[(!PTRS32 || sizeof(collision_bsp_segment_result) == 0x418) ? 1 : -1];
typedef char check_pill_query[(!PTRS32 || sizeof(collision_bsp_pill_query) == 0x22c) ? 1 : -1];
typedef char check_pill_result[(sizeof(collision_bsp_pill_result) == 0x420) ? 1 : -1];
typedef char check_boundary_hit[(sizeof(collision_bsp_boundary_hit) == 0x0c) ? 1 : -1];
typedef char check_boundary_clip[(sizeof(collision_bsp_boundary_clip) == 0x18) ? 1 : -1];
typedef char check_model_sphere[(sizeof(physics_model_sphere) == 0x1c) ? 1 : -1];
typedef char check_model_pill[(sizeof(physics_model_pill) == 0x28) ? 1 : -1];
typedef char check_model_shape[(sizeof(physics_model_shape) == 0x68) ? 1 : -1];
typedef char check_model[(sizeof(physics_model) == 0xac08) ? 1 : -1];
typedef char check_model_contact[(sizeof(physics_model_contact) == 0x2c) ? 1 : -1];
typedef char check_obj_collision_ctx[(!PTRS32 || sizeof(object_collision_context) == 0x10) ? 1 : -1];
typedef char check_node_result[(!PTRS32 || sizeof(object_node_collision_result) == 0x420) ? 1 : -1];
typedef char check_obj_physics_ctx[(!PTRS32 || sizeof(object_physics_context) == 0x3c) ? 1 : -1];
typedef char check_obj_physics_ray[(sizeof(object_physics_ray_result) == 0x14) ? 1 : -1];
typedef char check_mass_point_state[(sizeof(mass_point_state) == 0x130) ? 1 : -1];
typedef char check_powered_state[(sizeof(powered_mass_point_state) == 0x60) ? 1 : -1];
typedef char check_breakable_globals[(sizeof(breakable_surface_globals) == 0x4204) ? 1 : -1];
typedef char check_scalar_range[(sizeof(physics_scalar_range) == 0x08) ? 1 : -1];
typedef char check_scalar_rates[(sizeof(physics_scalar_rates) == 0x10) ? 1 : -1];

// ---------------------------------------------------------------------------
// collision_bsp query records, as the recursive descents index them
// ---------------------------------------------------------------------------
typedef char check_sq_breakable[(!PTRS32 || __builtin_offsetof(collision_bsp_sphere_query, breakable_surfaces) == 0x08) ? 1 : -1];
typedef char check_sq_center[(!PTRS32 || __builtin_offsetof(collision_bsp_sphere_query, center) == 0x0c) ? 1 : -1];
typedef char check_sq_radius[(!PTRS32 || __builtin_offsetof(collision_bsp_sphere_query, radius) == 0x10) ? 1 : -1];
typedef char check_sq_result[(!PTRS32 || __builtin_offsetof(collision_bsp_sphere_query, result) == 0x14) ? 1 : -1];
typedef char check_sq_planes[(!PTRS32 || __builtin_offsetof(collision_bsp_sphere_query, planes) == 0x1c) ? 1 : -1];
typedef char check_sq_axis[(!PTRS32 || __builtin_offsetof(collision_bsp_sphere_query, projection_axis) == 0x21c) ? 1 : -1];
typedef char check_sq_sign[(!PTRS32 || __builtin_offsetof(collision_bsp_sphere_query, projection_sign) == 0x21e) ? 1 : -1];
typedef char check_sq_pi[(!PTRS32 || __builtin_offsetof(collision_bsp_sphere_query, projected_center_i) == 0x220) ? 1 : -1];
typedef char check_sq_pj[(!PTRS32 || __builtin_offsetof(collision_bsp_sphere_query, projected_center_j) == 0x224) ? 1 : -1];

typedef char check_sr_edges_n[(__builtin_offsetof(collision_bsp_sphere_result, edge_count) == 0x404) ? 1 : -1];
typedef char check_sr_verts_n[(__builtin_offsetof(collision_bsp_sphere_result, vertex_count) == 0x808) ? 1 : -1];
typedef char check_sr_leaves_n[(__builtin_offsetof(collision_bsp_sphere_result, leaf_count) == 0xc0c) ? 1 : -1];
typedef char check_sr_leaves[(__builtin_offsetof(collision_bsp_sphere_result, leaves) == 0xc10) ? 1 : -1];

typedef char check_gq_bsp[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_query, bsp) == 0x04) ? 1 : -1];
typedef char check_gq_origin[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_query, origin) == 0x10) ? 1 : -1];
typedef char check_gq_delta[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_query, delta) == 0x14) ? 1 : -1];
typedef char check_gq_result[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_query, result) == 0x18) ? 1 : -1];
typedef char check_gq_last[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_query, last_leaf) == 0x1c) ? 1 : -1];
typedef char check_gq_type[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_query, last_leaf_type) == 0x20) ? 1 : -1];
typedef char check_gq_plane[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_query, crossing_plane) == 0x24) ? 1 : -1];

typedef char check_gr_plane[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_result, plane) == 0x04) ? 1 : -1];
typedef char check_gr_surface[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_result, surface_index) == 0x08) ? 1 : -1];
typedef char check_gr_plane_i[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_result, plane_index) == 0x0c) ? 1 : -1];
typedef char check_gr_flags[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_result, surface_flags) == 0x10) ? 1 : -1];
typedef char check_gr_break[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_result, breakable_surface_index) == 0x11) ? 1 : -1];
typedef char check_gr_material[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_result, material_index) == 0x12) ? 1 : -1];
typedef char check_gr_leafn[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_result, leaf_count) == 0x14) ? 1 : -1];
typedef char check_gr_leaves[(!PTRS32 || __builtin_offsetof(collision_bsp_segment_result, leaves) == 0x18) ? 1 : -1];

typedef char check_pq_radius[(!PTRS32 || __builtin_offsetof(collision_bsp_pill_query, radius) == 0x0c) ? 1 : -1];
typedef char check_pq_result[(!PTRS32 || __builtin_offsetof(collision_bsp_pill_query, result) == 0x10) ? 1 : -1];
typedef char check_pq_count[(!PTRS32 || __builtin_offsetof(collision_bsp_pill_query, plane_count) == 0x14) ? 1 : -1];
typedef char check_pq_planes[(!PTRS32 || __builtin_offsetof(collision_bsp_pill_query, planes) == 0x18) ? 1 : -1];
typedef char check_pq_axis[(!PTRS32 || __builtin_offsetof(collision_bsp_pill_query, projection_axis) == 0x218) ? 1 : -1];
typedef char check_pq_sign[(!PTRS32 || __builtin_offsetof(collision_bsp_pill_query, projection_sign) == 0x21a) ? 1 : -1];
typedef char check_pq_dj[(!PTRS32 || __builtin_offsetof(collision_bsp_pill_query, projected_delta_j) == 0x228) ? 1 : -1];

typedef char check_pr_surface[(__builtin_offsetof(collision_bsp_pill_result, surface_index) == 0x14) ? 1 : -1];
typedef char check_pr_material[(__builtin_offsetof(collision_bsp_pill_result, material_index) == 0x1a) ? 1 : -1];
typedef char check_pr_leafn[(__builtin_offsetof(collision_bsp_pill_result, leaf_count) == 0x1c) ? 1 : -1];
typedef char check_pr_leaves[(__builtin_offsetof(collision_bsp_pill_result, leaves) == 0x20) ? 1 : -1];

// ---------------------------------------------------------------------------
// physics_model: the bases 0x00504260 and 0x00504bb0 index proxies from
// ---------------------------------------------------------------------------
typedef char check_pm_spheres[(__builtin_offsetof(physics_model, spheres) == 0x0008) ? 1 : -1];
typedef char check_pm_pills[(__builtin_offsetof(physics_model, pills) == 0x1c08) ? 1 : -1];
typedef char check_pm_shapes[(__builtin_offsetof(physics_model, shapes) == 0x4408) ? 1 : -1];
typedef char check_pm_counts[(__builtin_offsetof(physics_model, pill_count) == 0x02
                              && __builtin_offsetof(physics_model, shape_count) == 0x04) ? 1 : -1];

typedef char check_ps_center[(__builtin_offsetof(physics_model_sphere, center_x) == 0x0c) ? 1 : -1];
typedef char check_ps_radius[(__builtin_offsetof(physics_model_sphere, radius) == 0x18) ? 1 : -1];
typedef char check_pp_origin[(__builtin_offsetof(physics_model_pill, origin_x) == 0x0c) ? 1 : -1];
typedef char check_pp_extent[(__builtin_offsetof(physics_model_pill, extent_i) == 0x18) ? 1 : -1];
typedef char check_pp_radius[(__builtin_offsetof(physics_model_pill, radius) == 0x24) ? 1 : -1];
typedef char check_ph_plane[(__builtin_offsetof(physics_model_shape, plane_i) == 0x0c) ? 1 : -1];
typedef char check_ph_thick[(__builtin_offsetof(physics_model_shape, thickness) == 0x1c) ? 1 : -1];
typedef char check_ph_axis[(__builtin_offsetof(physics_model_shape, projection_axis) == 0x20) ? 1 : -1];
typedef char check_ph_sign[(__builtin_offsetof(physics_model_shape, projection_sign) == 0x22) ? 1 : -1];
typedef char check_ph_count[(__builtin_offsetof(physics_model_shape, vertex_count) == 0x24) ? 1 : -1];
typedef char check_ph_verts[(__builtin_offsetof(physics_model_shape, vertices) == 0x28) ? 1 : -1];

typedef char check_mc_plane[(__builtin_offsetof(physics_model_contact, plane_i) == 0x10) ? 1 : -1];
typedef char check_mc_object[(__builtin_offsetof(physics_model_contact, object_index) == 0x20) ? 1 : -1];
typedef char check_mc_flags[(__builtin_offsetof(physics_model_contact, surface_flags) == 0x28) ? 1 : -1];
typedef char check_mc_material[(__builtin_offsetof(physics_model_contact, material_type) == 0x2a) ? 1 : -1];

// the six provenance bytes are at the same place in all three proxies and in the contact
typedef char check_prov_sphere[(__builtin_offsetof(physics_model_sphere, surface_flags) == 0x08
                                && __builtin_offsetof(physics_model_sphere, material_type) == 0x0a) ? 1 : -1];
typedef char check_prov_pill[(__builtin_offsetof(physics_model_pill, surface_flags) == 0x08
                              && __builtin_offsetof(physics_model_pill, material_type) == 0x0a) ? 1 : -1];
typedef char check_prov_shape[(__builtin_offsetof(physics_model_shape, surface_flags) == 0x08
                               && __builtin_offsetof(physics_model_shape, material_type) == 0x0a) ? 1 : -1];

// ---------------------------------------------------------------------------
// object contexts and mass points
// ---------------------------------------------------------------------------
typedef char check_occ_def[(!PTRS32 || __builtin_offsetof(object_collision_context, definition) == 0x04) ? 1 : -1];
typedef char check_occ_perm[(!PTRS32 || __builtin_offsetof(object_collision_context, region_permutations) == 0x08) ? 1 : -1];
typedef char check_occ_nodes[(!PTRS32 || __builtin_offsetof(object_collision_context, nodes) == 0x0c) ? 1 : -1];
typedef char check_oncr_seg[(!PTRS32 || __builtin_offsetof(object_node_collision_result, segment) == 0x08) ? 1 : -1];
typedef char check_opc_scale[(!PTRS32 || __builtin_offsetof(object_physics_context, scale) == 0x08) ? 1 : -1];
typedef char check_opc_fwd[(!PTRS32 || __builtin_offsetof(object_physics_context, forward_i) == 0x0c) ? 1 : -1];
typedef char check_opc_left[(!PTRS32 || __builtin_offsetof(object_physics_context, left_i) == 0x18) ? 1 : -1];
typedef char check_opc_up[(!PTRS32 || __builtin_offsetof(object_physics_context, up_i) == 0x24) ? 1 : -1];
typedef char check_opc_pos[(!PTRS32 || __builtin_offsetof(object_physics_context, position_x) == 0x30) ? 1 : -1];

typedef char check_mps_pos[(__builtin_offsetof(mass_point_state, position_x) == 0x004) ? 1 : -1];
typedef char check_mps_fwd[(__builtin_offsetof(mass_point_state, forward_i) == 0x010) ? 1 : -1];
typedef char check_mps_up[(__builtin_offsetof(mass_point_state, up_i) == 0x028) ? 1 : -1];
typedef char check_mps_leaf[(__builtin_offsetof(mass_point_state, leaf_index) == 0x034) ? 1 : -1];
typedef char check_mps_cluster[(__builtin_offsetof(mass_point_state, cluster_index) == 0x038) ? 1 : -1];
typedef char check_mps_offset[(__builtin_offsetof(mass_point_state, offset_x) == 0x03c) ? 1 : -1];
typedef char check_mps_vel[(__builtin_offsetof(mass_point_state, velocity_i) == 0x048) ? 1 : -1];
typedef char check_mps_tan[(__builtin_offsetof(mass_point_state, tangential_velocity_i) == 0x054) ? 1 : -1];
typedef char check_mps_plane[(__builtin_offsetof(mass_point_state, resting_plane_i) == 0x060) ? 1 : -1];
typedef char check_mps_material[(__builtin_offsetof(mass_point_state, material_type) == 0x070) ? 1 : -1];
typedef char check_mps_ground[(__builtin_offsetof(mass_point_state, ground_depth) == 0x074) ? 1 : -1];
typedef char check_mps_water[(__builtin_offsetof(mass_point_state, water_depth) == 0x07c) ? 1 : -1];
typedef char check_mps_nmag[(__builtin_offsetof(mass_point_state, ground_normal_magnitude) == 0x080) ? 1 : -1];
typedef char check_mps_nforce[(__builtin_offsetof(mass_point_state, ground_normal_force_i) == 0x084) ? 1 : -1];
typedef char check_mps_gfric[(__builtin_offsetof(mass_point_state, ground_friction_force) == 0x090) ? 1 : -1];
typedef char check_mps_buoy[(__builtin_offsetof(mass_point_state, buoyancy_magnitude) == 0x0b4) ? 1 : -1];
typedef char check_mps_wfric[(__builtin_offsetof(mass_point_state, water_friction_force) == 0x0c4) ? 1 : -1];
typedef char check_mps_afric[(__builtin_offsetof(mass_point_state, air_friction_force) == 0x0e8) ? 1 : -1];
typedef char check_mps_powered[(__builtin_offsetof(mass_point_state, powered_force_i) == 0x10c) ? 1 : -1];
typedef char check_mps_total[(__builtin_offsetof(mass_point_state, total_force_i) == 0x118) ? 1 : -1];
typedef char check_mps_torque[(__builtin_offsetof(mass_point_state, torque_i) == 0x124) ? 1 : -1];

typedef char check_pmps_antigrav[(__builtin_offsetof(powered_mass_point_state, antigrav) == 0x18) ? 1 : -1];
typedef char check_pmps_matrix[(__builtin_offsetof(powered_mass_point_state, matrix_scale) == 0x2c) ? 1 : -1];

typedef char check_bsg_active[(__builtin_offsetof(breakable_surface_globals, active) == 0x001) ? 1 : -1];
typedef char check_bsg_health[(__builtin_offsetof(breakable_surface_globals, health) == 0x204) ? 1 : -1];

// ---------------------------------------------------------------------------
// the tag layouts every attribution above rests on
// ---------------------------------------------------------------------------
typedef char check_bsp_size[(sizeof(ModelCollisionGeometryBSP) == 0x60) ? 1 : -1];
typedef char check_bsp_nodes[(__builtin_offsetof(ModelCollisionGeometryBSP, bsp3d_nodes) == 0x00) ? 1 : -1];
typedef char check_bsp_planes[(__builtin_offsetof(ModelCollisionGeometryBSP, planes) == 0x0c) ? 1 : -1];
typedef char check_bsp_leaves[(__builtin_offsetof(ModelCollisionGeometryBSP, leaves) == 0x18) ? 1 : -1];
typedef char check_bsp_refs[(__builtin_offsetof(ModelCollisionGeometryBSP, bsp2d_references) == 0x24) ? 1 : -1];
typedef char check_bsp_2dnodes[(__builtin_offsetof(ModelCollisionGeometryBSP, bsp2d_nodes) == 0x30) ? 1 : -1];
typedef char check_bsp_surfaces[(__builtin_offsetof(ModelCollisionGeometryBSP, surfaces) == 0x3c) ? 1 : -1];
typedef char check_bsp_edges[(__builtin_offsetof(ModelCollisionGeometryBSP, edges) == 0x48) ? 1 : -1];
typedef char check_bsp_verts[(__builtin_offsetof(ModelCollisionGeometryBSP, vertices) == 0x54) ? 1 : -1];
typedef char check_bsp3d_size[(sizeof(ModelCollisionGeometryBSP3DNode) == 0x0c) ? 1 : -1];
typedef char check_bspplane_size[(sizeof(ModelCollisionGeometryBSPPlane) == 0x10) ? 1 : -1];
typedef char check_bspleaf_size[(sizeof(ModelCollisionGeometryBSPLeaf) == 0x08) ? 1 : -1];
typedef char check_bsp2dref_size[(sizeof(ModelCollisionGeometryBSP2DReference) == 0x08) ? 1 : -1];
typedef char check_bsp2dnode_size[(sizeof(ModelCollisionGeometryBSP2DNode) == 0x14) ? 1 : -1];
typedef char check_bspsurface_size[(sizeof(ModelCollisionGeometryBSPSurface) == 0x0c) ? 1 : -1];
typedef char check_bspsurface_flags[(__builtin_offsetof(ModelCollisionGeometryBSPSurface, flags) == 0x08
                                     && __builtin_offsetof(ModelCollisionGeometryBSPSurface, breakable_surface) == 0x09
                                     && __builtin_offsetof(ModelCollisionGeometryBSPSurface, material) == 0x0a) ? 1 : -1];
typedef char check_bspedge_size[(sizeof(ModelCollisionGeometryBSPEdge) == 0x18) ? 1 : -1];
typedef char check_bspedge_fields[(__builtin_offsetof(ModelCollisionGeometryBSPEdge, forward_edge) == 0x08
                                   && __builtin_offsetof(ModelCollisionGeometryBSPEdge, left_surface) == 0x10
                                   && __builtin_offsetof(ModelCollisionGeometryBSPEdge, right_surface) == 0x14) ? 1 : -1];
typedef char check_bspvertex_size[(sizeof(ModelCollisionGeometryBSPVertex) == 0x10) ? 1 : -1];

typedef char check_mcg_size[(sizeof(ModelCollisionGeometry) == 0x298) ? 1 : -1];
typedef char check_mcg_materials[(__builtin_offsetof(ModelCollisionGeometry, materials) == 0x234) ? 1 : -1];
typedef char check_mcg_nodes[(__builtin_offsetof(ModelCollisionGeometry, nodes) == 0x28c) ? 1 : -1];
typedef char check_mcgmat_size[(sizeof(ModelCollisionGeometryMaterial) == 0x48) ? 1 : -1];
typedef char check_mcgmat_type[(__builtin_offsetof(ModelCollisionGeometryMaterial, material_type) == 0x24) ? 1 : -1];
typedef char check_mcgnode_size[(sizeof(ModelCollisionGeometryNode) == 0x40) ? 1 : -1];
typedef char check_mcgnode_region[(__builtin_offsetof(ModelCollisionGeometryNode, region) == 0x20) ? 1 : -1];
typedef char check_mcgnode_bsps[(__builtin_offsetof(ModelCollisionGeometryNode, bsps) == 0x34) ? 1 : -1];

typedef char check_physics_size[(sizeof(Physics) == 0x80) ? 1 : -1];
typedef char check_physics_com[(__builtin_offsetof(Physics, center_of_mass) == 0x0c) ? 1 : -1];
typedef char check_physics_mass[(__builtin_offsetof(Physics, mass) == 0x08) ? 1 : -1];
typedef char check_physics_density[(__builtin_offsetof(Physics, density) == 0x18) ? 1 : -1];
typedef char check_physics_gravity[(__builtin_offsetof(Physics, gravity_scale) == 0x1c) ? 1 : -1];
typedef char check_physics_gfric[(__builtin_offsetof(Physics, ground_friction) == 0x20) ? 1 : -1];
typedef char check_physics_gdepth[(__builtin_offsetof(Physics, ground_depth) == 0x24) ? 1 : -1];
typedef char check_physics_gdamp[(__builtin_offsetof(Physics, ground_damp_fraction) == 0x28) ? 1 : -1];
typedef char check_physics_gk1[(__builtin_offsetof(Physics, ground_normal_k1) == 0x2c) ? 1 : -1];
typedef char check_physics_gk0[(__builtin_offsetof(Physics, ground_normal_k0) == 0x30) ? 1 : -1];
typedef char check_physics_wfric[(__builtin_offsetof(Physics, water_friction) == 0x38) ? 1 : -1];
typedef char check_physics_wdepth[(__builtin_offsetof(Physics, water_depth) == 0x3c) ? 1 : -1];
typedef char check_physics_wdens[(__builtin_offsetof(Physics, water_density) == 0x40) ? 1 : -1];
typedef char check_physics_afric[(__builtin_offsetof(Physics, air_friction) == 0x48) ? 1 : -1];
typedef char check_physics_inertial[(__builtin_offsetof(Physics, inertial_matrix_and_inverse) == 0x5c) ? 1 : -1];
typedef char check_physics_powered[(__builtin_offsetof(Physics, powered_mass_points) == 0x68) ? 1 : -1];
typedef char check_physics_points[(__builtin_offsetof(Physics, mass_points) == 0x74) ? 1 : -1];
typedef char check_masspoint_size[(sizeof(PhysicsMassPoint) == 0x80) ? 1 : -1];
typedef char check_masspoint_powered[(__builtin_offsetof(PhysicsMassPoint, powered_mass_point) == 0x20) ? 1 : -1];
typedef char check_masspoint_rel[(__builtin_offsetof(PhysicsMassPoint, relative_mass) == 0x28) ? 1 : -1];
typedef char check_masspoint_mass[(__builtin_offsetof(PhysicsMassPoint, mass) == 0x2c) ? 1 : -1];
typedef char check_masspoint_reldens[(__builtin_offsetof(PhysicsMassPoint, relative_density) == 0x30) ? 1 : -1];
typedef char check_masspoint_dens[(__builtin_offsetof(PhysicsMassPoint, density) == 0x34) ? 1 : -1];
typedef char check_masspoint_pos[(__builtin_offsetof(PhysicsMassPoint, position) == 0x38) ? 1 : -1];
typedef char check_masspoint_fwd[(__builtin_offsetof(PhysicsMassPoint, forward) == 0x44) ? 1 : -1];
typedef char check_masspoint_up[(__builtin_offsetof(PhysicsMassPoint, up) == 0x50) ? 1 : -1];
typedef char check_masspoint_fric[(__builtin_offsetof(PhysicsMassPoint, friction_type) == 0x5c) ? 1 : -1];
typedef char check_masspoint_pscale[(__builtin_offsetof(PhysicsMassPoint, friction_parallel_scale) == 0x60) ? 1 : -1];
typedef char check_masspoint_radius[(__builtin_offsetof(PhysicsMassPoint, radius) == 0x68) ? 1 : -1];
typedef char check_powered_size[(sizeof(PhysicsPoweredMassPoint) == 0x80) ? 1 : -1];
typedef char check_powered_flags[(__builtin_offsetof(PhysicsPoweredMassPoint, flags) == 0x20) ? 1 : -1];
typedef char check_powered_strength[(__builtin_offsetof(PhysicsPoweredMassPoint, antigrav_strength) == 0x24) ? 1 : -1];
typedef char check_powered_height[(__builtin_offsetof(PhysicsPoweredMassPoint, antigrav_height) == 0x2c) ? 1 : -1];
typedef char check_powered_damp[(__builtin_offsetof(PhysicsPoweredMassPoint, antigrav_damp_fraction) == 0x30) ? 1 : -1];
typedef char check_powered_k1[(__builtin_offsetof(PhysicsPoweredMassPoint, antigrav_normal_k1) == 0x34) ? 1 : -1];
typedef char check_powered_k0[(__builtin_offsetof(PhysicsPoweredMassPoint, antigrav_normal_k0) == 0x38) ? 1 : -1];

typedef char check_pointphysics_mass[(__builtin_offsetof(PointPhysics, mass_scale) == 0x04) ? 1 : -1];
typedef char check_pointphysics_wgrav[(__builtin_offsetof(PointPhysics, water_gravity_scale) == 0x08) ? 1 : -1];
typedef char check_pointphysics_agrav[(__builtin_offsetof(PointPhysics, air_gravity_scale) == 0x0c) ? 1 : -1];
typedef char check_pointphysics_afric[(__builtin_offsetof(PointPhysics, air_friction) == 0x24) ? 1 : -1];
typedef char check_pointphysics_wfric[(__builtin_offsetof(PointPhysics, water_friction) == 0x28) ? 1 : -1];
typedef char check_pointphysics_sfric[(__builtin_offsetof(PointPhysics, surface_friction) == 0x2c) ? 1 : -1];
typedef char check_pointphysics_elast[(__builtin_offsetof(PointPhysics, elasticity) == 0x30) ? 1 : -1];

typedef char check_object_collision[(__builtin_offsetof(Object, collision_model) == 0x70) ? 1 : -1];
typedef char check_object_physics[(__builtin_offsetof(Object, physics) == 0x80) ? 1 : -1];

typedef char check_sbsp_materials[(__builtin_offsetof(ScenarioStructureBSP, collision_materials) == 0xa4) ? 1 : -1];
typedef char check_sbsp_leaves[(__builtin_offsetof(ScenarioStructureBSP, leaves) == 0xe0) ? 1 : -1];
typedef char check_sbsp_breakable[(__builtin_offsetof(ScenarioStructureBSP, breakable_surfaces) == 0x16c) ? 1 : -1];
typedef char check_sbspmat_size[(sizeof(ScenarioStructureBSPCollisionMaterial) == 0x14) ? 1 : -1];
typedef char check_sbspmat_type[(__builtin_offsetof(ScenarioStructureBSPCollisionMaterial, material) == 0x12) ? 1 : -1];
typedef char check_sbspleaf_size[(sizeof(ScenarioStructureBSPLeaf) == 0x10) ? 1 : -1];
typedef char check_sbspleaf_cluster[(__builtin_offsetof(ScenarioStructureBSPLeaf, cluster) == 0x08) ? 1 : -1];
typedef char check_sbspbreak_size[(sizeof(ScenarioStructureBSPBreakableSurface) == 0x30) ? 1 : -1];
typedef char check_sbspbreak_radius[(__builtin_offsetof(ScenarioStructureBSPBreakableSurface, radius) == 0x0c) ? 1 : -1];
typedef char check_sbspbreak_surface[(__builtin_offsetof(ScenarioStructureBSPBreakableSurface, collision_surface_index) == 0x10) ? 1 : -1];

typedef char check_globals_material_size[(sizeof(GlobalsMaterial) == 0x374) ? 1 : -1];
typedef char check_globals_gfric[(__builtin_offsetof(GlobalsMaterial, ground_friction_scale) == 0x94) ? 1 : -1];

typedef char check_dmg_lower[(__builtin_offsetof(DamageEffect, damage_lower_bound) == 0x1d0) ? 1 : -1];
typedef char check_dmg_upper[(__builtin_offsetof(DamageEffect, damage_upper_bound) == 0x1d4) ? 1 : -1];
typedef char check_dmg_dirt[(__builtin_offsetof(DamageEffect, dirt) == 0x200) ? 1 : -1];

// ---------------------------------------------------------------------------
// collision_result, as this module drives it (definition lives in projectiles.h)
// ---------------------------------------------------------------------------
typedef char check_cr_size[(sizeof(collision_result) == 0x50) ? 1 : -1];
typedef char check_cr_t2[(__builtin_offsetof(collision_result, t) == 0x14) ? 1 : -1];
typedef char check_cr_point2[(__builtin_offsetof(collision_result, point) == 0x18) ? 1 : -1];
typedef char check_cr_normal2[(__builtin_offsetof(collision_result, normal) == 0x24) ? 1 : -1];
typedef char check_cr_plane_d[(__builtin_offsetof(collision_result, unknown_30) == 0x30) ? 1 : -1];
typedef char check_cr_material2[(__builtin_offsetof(collision_result, material_type) == 0x34) ? 1 : -1];
typedef char check_cr_object2[(__builtin_offsetof(collision_result, object_index) == 0x38) ? 1 : -1];
typedef char check_cr_region[(__builtin_offsetof(collision_result, unknown_3c) == 0x3c) ? 1 : -1];
typedef char check_cr_node[(__builtin_offsetof(collision_result, marker_index) == 0x3e) ? 1 : -1];
typedef char check_cr_perm[(__builtin_offsetof(collision_result, unknown_40) == 0x40) ? 1 : -1];
typedef char check_cr_surface2[(__builtin_offsetof(collision_result, surface_index) == 0x44) ? 1 : -1];
typedef char check_cr_planeidx[(__builtin_offsetof(collision_result, unknown_48) == 0x48) ? 1 : -1];
typedef char check_cr_flags2[(__builtin_offsetof(collision_result, surface_flags) == 0x4c) ? 1 : -1];
typedef char check_cr_break2[(__builtin_offsetof(collision_result, unknown_4d) == 0x4d) ? 1 : -1];
typedef char check_cr_matidx[(__builtin_offsetof(collision_result, unknown_4e) == 0x4e) ? 1 : -1];

// ---------------------------------------------------------------------------
// enum values the module dispatches on
// ---------------------------------------------------------------------------
typedef char check_leaf_types[(_collision_bsp_leaf_type_double_sided == 1
                               && _collision_bsp_leaf_type_normal == 2
                               && _collision_bsp_leaf_type_none == 3) ? 1 : -1];
typedef char check_shape_types[(_physics_model_shape_sphere == 0
                                && _physics_model_shape_pill == 1
                                && _physics_model_shape_polygon == 2) ? 1 : -1];
typedef char check_point_flags[(_point_physics_in_air_bit == 1
                                && _point_physics_in_water_bit == 2
                                && _point_physics_collided_bit == 4
                                && _point_physics_hit_water_surface_bit == 8) ? 1 : -1];
typedef char check_mass_flags[(_mass_point_at_rest_bit == 1
                               && _mass_point_ground_contact_bit == 2
                               && _mass_point_on_ground_surface_bit == 4
                               && _mass_point_water_contact_bit == 8
                               && _mass_point_antigrav_bit == 0x10) ? 1 : -1];
typedef char check_constants[(4 + k_maximum_collision_bsp_query_results * 4
                              == __builtin_offsetof(collision_bsp_sphere_result, edge_count)) ? 1 : -1];
