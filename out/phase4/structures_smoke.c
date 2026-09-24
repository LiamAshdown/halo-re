// Phase 4 syntax gate for types/structures.h. The host gcc is 64-bit, so a struct that holds a
// pointer is 4 bytes larger per pointer than in halo.exe; those checks are gated on PTRS32 and
// only fire with -m32. Every size in structures.h was checked against that rule by hand.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

#define PTRS32 (sizeof(void *) == 4)

typedef char check_polygon2d[(sizeof(polygon2d) == 0x804) ? 1 : -1];
typedef char check_polygon2d_points[(__builtin_offsetof(polygon2d, points) == 0x04) ? 1 : -1];
typedef char check_leaf_map[(sizeof(structure_bsp_leaf_map) == 0x1c) ? 1 : -1];
typedef char check_leaf_map_leaves[(__builtin_offsetof(structure_bsp_leaf_map, leaves) == 0x04
                                   && __builtin_offsetof(structure_bsp_leaf_map, portals) == 0x10) ? 1 : -1];
typedef char check_surface_run[(sizeof(structure_bsp_cluster_surface_run) == 0x0c) ? 1 : -1];
typedef char check_visible_cluster[(sizeof(structure_bsp_visible_cluster) == 0x1a0) ? 1 : -1];
typedef char check_visible_cluster_bounds[(__builtin_offsetof(structure_bsp_visible_cluster, screen_bounds_x) == 0x04
                                          && __builtin_offsetof(structure_bsp_visible_cluster, screen_bounds_y) == 0x0c) ? 1 : -1];
typedef char check_mirror_result[(sizeof(structure_bsp_mirror_result) == 0x1c) ? 1 : -1];
typedef char check_mirror_cluster[(__builtin_offsetof(structure_bsp_mirror_result, cluster_index) == 0x18) ? 1 : -1];
typedef char check_fog_env[(!PTRS32 || sizeof(structure_fog_environment) == 0x4c) ? 1 : -1];
typedef char check_fog_mode[(__builtin_offsetof(structure_fog_environment, plane_mode) == 0x1c) ? 1 : -1];
typedef char check_fog_plane[(__builtin_offsetof(structure_fog_environment, plane) == 0x20) ? 1 : -1];
typedef char check_fog_color[(__builtin_offsetof(structure_fog_environment, color_red) == 0x30) ? 1 : -1];
typedef char check_fog_density[(__builtin_offsetof(structure_fog_environment, maximum_density) == 0x3c) ? 1 : -1];
typedef char check_fog_screen[(!PTRS32 || __builtin_offsetof(structure_fog_environment, screen_parameters) == 0x48) ? 1 : -1];

typedef char check_do_batch[(!PTRS32 || sizeof(detail_object_batch) == 0x18) ? 1 : -1];
typedef char check_do_batch_cell[(__builtin_offsetof(detail_object_batch, cell_x) == 0x08
                                 && __builtin_offsetof(detail_object_batch, cell_z) == 0x0c) ? 1 : -1];
typedef char check_do_batch_zref[(!PTRS32 || __builtin_offsetof(detail_object_batch, z_reference) == 0x14) ? 1 : -1];
typedef char check_do_layer[(!PTRS32 || sizeof(detail_object_layer_batches) == 0x08) ? 1 : -1];
typedef char check_do_list[(!PTRS32 || sizeof(detail_object_render_list) == 0x08) ? 1 : -1];
typedef char check_do_frame[(!PTRS32 || sizeof(detail_object_frame) == 0x5210) ? 1 : -1];
typedef char check_do_frame_layers[(!PTRS32 || __builtin_offsetof(detail_object_frame, layers) == 0x5100) ? 1 : -1];
typedef char check_do_frame_list[(!PTRS32 || __builtin_offsetof(detail_object_frame, render_list) == 0x5200) ? 1 : -1];
typedef char check_do_frame_cell[(!PTRS32 || __builtin_offsetof(detail_object_frame, cell_x) == 0x5208) ? 1 : -1];
typedef char check_do_frame_valid[(!PTRS32 || __builtin_offsetof(detail_object_frame, valid) == 0x520e) ? 1 : -1];
typedef char check_do_globals[(!PTRS32 || sizeof(detail_object_globals) == 0xa430) ? 1 : -1];
typedef char check_do_globals_zref[(!PTRS32 || __builtin_offsetof(detail_object_globals, default_z_reference) == 0xa420) ? 1 : -1];
typedef char check_do_key[(sizeof(detail_object_cell_key) == 0x08) ? 1 : -1];

// the tag layouts every attribution above rests on
typedef char check_sbsp_size[(sizeof(ScenarioStructureBSP) == 0x288) ? 1 : -1];
typedef char check_sbsp_cbsp[(__builtin_offsetof(ScenarioStructureBSP, collision_bsp) == 0xb0) ? 1 : -1];
typedef char check_sbsp_bounds[(__builtin_offsetof(ScenarioStructureBSP, world_bounds_x) == 0xc8) ? 1 : -1];
typedef char check_sbsp_leaves2[(__builtin_offsetof(ScenarioStructureBSP, leaves) == 0xe0) ? 1 : -1];
typedef char check_sbsp_leafsurf[(__builtin_offsetof(ScenarioStructureBSP, leaf_surfaces) == 0xec) ? 1 : -1];
typedef char check_sbsp_surfaces[(__builtin_offsetof(ScenarioStructureBSP, surfaces) == 0xf8) ? 1 : -1];
typedef char check_sbsp_lightmaps[(__builtin_offsetof(ScenarioStructureBSP, lightmaps) == 0x104) ? 1 : -1];
typedef char check_sbsp_clusters[(__builtin_offsetof(ScenarioStructureBSP, clusters) == 0x134) ? 1 : -1];
typedef char check_sbsp_clusterdata[(__builtin_offsetof(ScenarioStructureBSP, cluster_data) == 0x140) ? 1 : -1];
typedef char check_sbsp_portals[(__builtin_offsetof(ScenarioStructureBSP, cluster_portals) == 0x154) ? 1 : -1];
typedef char check_sbsp_fogplanes[(__builtin_offsetof(ScenarioStructureBSP, fog_planes) == 0x178) ? 1 : -1];
typedef char check_sbsp_fogregions[(__builtin_offsetof(ScenarioStructureBSP, fog_regions) == 0x184) ? 1 : -1];
typedef char check_sbsp_fogpalette[(__builtin_offsetof(ScenarioStructureBSP, fog_palette) == 0x190) ? 1 : -1];
typedef char check_sbsp_bgsound[(__builtin_offsetof(ScenarioStructureBSP, background_sound_palette) == 0x1fc) ? 1 : -1];
typedef char check_sbsp_soundenv[(__builtin_offsetof(ScenarioStructureBSP, sound_environment_palette) == 0x208) ? 1 : -1];
typedef char check_sbsp_detail[(__builtin_offsetof(ScenarioStructureBSP, detail_objects) == 0x24c) ? 1 : -1];
typedef char check_sbsp_decals[(__builtin_offsetof(ScenarioStructureBSP, runtime_decals) == 0x258) ? 1 : -1];
typedef char check_sbsp_leafmap[(__builtin_offsetof(ScenarioStructureBSP, leaf_map_leaves) == 0x270
                                 && __builtin_offsetof(ScenarioStructureBSP, leaf_map_portals) == 0x27c) ? 1 : -1];

typedef char check_cluster_size[(sizeof(ScenarioStructureBSPCluster) == 0x68) ? 1 : -1];
typedef char check_cluster_fog[(__builtin_offsetof(ScenarioStructureBSPCluster, fog) == 0x02) ? 1 : -1];
typedef char check_cluster_bgsound[(__builtin_offsetof(ScenarioStructureBSPCluster, background_sound) == 0x04
                                    && __builtin_offsetof(ScenarioStructureBSPCluster, sound_environment) == 0x06) ? 1 : -1];
typedef char check_cluster_decals[(__builtin_offsetof(ScenarioStructureBSPCluster, first_decal_index) == 0x0c
                                   && __builtin_offsetof(ScenarioStructureBSPCluster, decal_count) == 0x0e) ? 1 : -1];
typedef char check_cluster_sub[(__builtin_offsetof(ScenarioStructureBSPCluster, subclusters) == 0x34) ? 1 : -1];
typedef char check_cluster_surf[(__builtin_offsetof(ScenarioStructureBSPCluster, surface_indices) == 0x44) ? 1 : -1];
typedef char check_cluster_mirrors[(__builtin_offsetof(ScenarioStructureBSPCluster, mirrors) == 0x50) ? 1 : -1];
typedef char check_cluster_portals[(__builtin_offsetof(ScenarioStructureBSPCluster, portals) == 0x5c) ? 1 : -1];

typedef char check_sub_size[(sizeof(ScenarioStructureBSPSubcluster) == 0x24) ? 1 : -1];
typedef char check_sub_surf[(__builtin_offsetof(ScenarioStructureBSPSubcluster, surface_indices) == 0x18) ? 1 : -1];
typedef char check_leaf_size[(sizeof(ScenarioStructureBSPLeaf) == 0x10) ? 1 : -1];
typedef char check_leaf_cluster[(__builtin_offsetof(ScenarioStructureBSPLeaf, cluster) == 0x08) ? 1 : -1];
typedef char check_leaf_refs[(__builtin_offsetof(ScenarioStructureBSPLeaf, surface_reference_count) == 0x0a
                              && __builtin_offsetof(ScenarioStructureBSPLeaf, surface_references) == 0x0c) ? 1 : -1];
typedef char check_surfref_size[(sizeof(ScenarioStructureBSPSurfaceReference) == 0x08) ? 1 : -1];
typedef char check_surface_size[(sizeof(ScenarioStructureBSPSurface) == 0x06) ? 1 : -1];
typedef char check_lightmap_size[(sizeof(ScenarioStructureBSPLightmap) == 0x20) ? 1 : -1];
typedef char check_lightmap_mats[(__builtin_offsetof(ScenarioStructureBSPLightmap, materials) == 0x14) ? 1 : -1];
typedef char check_material_size[(sizeof(ScenarioStructureBSPMaterial) == 0x100) ? 1 : -1];
typedef char check_material_perm[(__builtin_offsetof(ScenarioStructureBSPMaterial, shader_permutation) == 0x10
                                  && __builtin_offsetof(ScenarioStructureBSPMaterial, flags) == 0x12) ? 1 : -1];
typedef char check_material_surf[(__builtin_offsetof(ScenarioStructureBSPMaterial, surfaces) == 0x14
                                  && __builtin_offsetof(ScenarioStructureBSPMaterial, surface_count) == 0x18) ? 1 : -1];
typedef char check_material_centroid[(__builtin_offsetof(ScenarioStructureBSPMaterial, centroid) == 0x1c
                                      && __builtin_offsetof(ScenarioStructureBSPMaterial, ambient_color) == 0x28) ? 1 : -1];
typedef char check_material_plane[(__builtin_offsetof(ScenarioStructureBSPMaterial, plane) == 0x9c
                                   && __builtin_offsetof(ScenarioStructureBSPMaterial, breakable_surface) == 0xac) ? 1 : -1];
typedef char check_material_vtype[(__builtin_offsetof(ScenarioStructureBSPMaterial, rendered_vertices_type) == 0xb0) ? 1 : -1];
typedef char check_mirror_size[(sizeof(ScenarioStructureBSPMirror) == 0x40) ? 1 : -1];
typedef char check_mirror_shader[(__builtin_offsetof(ScenarioStructureBSPMirror, shader) == 0x24
                                  && __builtin_offsetof(ScenarioStructureBSPMirror, vertices) == 0x34) ? 1 : -1];
typedef char check_portal_size[(sizeof(ScenarioStructureBSPClusterPortal) == 0x40) ? 1 : -1];
typedef char check_portal_clusters[(__builtin_offsetof(ScenarioStructureBSPClusterPortal, front_cluster) == 0x00
                                    && __builtin_offsetof(ScenarioStructureBSPClusterPortal, back_cluster) == 0x02
                                    && __builtin_offsetof(ScenarioStructureBSPClusterPortal, plane_index) == 0x04) ? 1 : -1];
typedef char check_portal_centroid[(__builtin_offsetof(ScenarioStructureBSPClusterPortal, centroid) == 0x08
                                    && __builtin_offsetof(ScenarioStructureBSPClusterPortal, bounding_radius) == 0x14
                                    && __builtin_offsetof(ScenarioStructureBSPClusterPortal, vertices) == 0x34) ? 1 : -1];
typedef char check_fogplane_size[(sizeof(ScenarioStructureBSPFogPlane) == 0x20) ? 1 : -1];
typedef char check_fogplane_plane[(__builtin_offsetof(ScenarioStructureBSPFogPlane, plane) == 0x04) ? 1 : -1];
typedef char check_fogregion_size[(sizeof(ScenarioStructureBSPFogRegion) == 0x28) ? 1 : -1];
typedef char check_fogregion_fog[(__builtin_offsetof(ScenarioStructureBSPFogRegion, fog) == 0x24) ? 1 : -1];
typedef char check_fogpalette_size[(sizeof(ScenarioStructureBSPFogPalette) == 0x88) ? 1 : -1];
typedef char check_fogpalette_dep[(__builtin_offsetof(ScenarioStructureBSPFogPalette, fog) == 0x20) ? 1 : -1];
typedef char check_bgsound_size[(sizeof(ScenarioStructureBSPBackgroundSoundPalette) == 0x74) ? 1 : -1];
typedef char check_soundenvpal_size[(sizeof(ScenarioStructureBSPSoundEnvironmentPalette) == 0x50) ? 1 : -1];
typedef char check_decal_size[(sizeof(ScenarioStructureBSPRuntimeDecal) == 0x10) ? 1 : -1];
typedef char check_decal_fields[(__builtin_offsetof(ScenarioStructureBSPRuntimeDecal, decal_type) == 0x0c
                                 && __builtin_offsetof(ScenarioStructureBSPRuntimeDecal, yaw) == 0x0e
                                 && __builtin_offsetof(ScenarioStructureBSPRuntimeDecal, pitch) == 0x0f) ? 1 : -1];
typedef char check_dod_size[(sizeof(ScenarioStructureBSPDetailObjectData) == 0x40) ? 1 : -1];
typedef char check_dod_fields[(__builtin_offsetof(ScenarioStructureBSPDetailObjectData, cells) == 0x00
                               && __builtin_offsetof(ScenarioStructureBSPDetailObjectData, instances) == 0x0c
                               && __builtin_offsetof(ScenarioStructureBSPDetailObjectData, counts) == 0x18
                               && __builtin_offsetof(ScenarioStructureBSPDetailObjectData, z_reference_vectors) == 0x24
                               && __builtin_offsetof(ScenarioStructureBSPDetailObjectData, bullshit) == 0x30) ? 1 : -1];
typedef char check_cell_size[(sizeof(ScenarioStructureBSPGlobalDetailObjectCell) == 0x20) ? 1 : -1];
typedef char check_cell_fields[(__builtin_offsetof(ScenarioStructureBSPGlobalDetailObjectCell, offset_z) == 0x06
                                && __builtin_offsetof(ScenarioStructureBSPGlobalDetailObjectCell, valid_layers_flags) == 0x08
                                && __builtin_offsetof(ScenarioStructureBSPGlobalDetailObjectCell, start_index) == 0x0c
                                && __builtin_offsetof(ScenarioStructureBSPGlobalDetailObjectCell, count_index) == 0x10) ? 1 : -1];
typedef char check_zref_size[(sizeof(ScenarioStructureBSPGlobalZReferenceVector) == 0x10) ? 1 : -1];
typedef char check_mapleaf_size[(sizeof(ScenarioStructureBSPGlobalMapLeaf) == 0x18) ? 1 : -1];
typedef char check_mapleaf_portals[(__builtin_offsetof(ScenarioStructureBSPGlobalMapLeaf, portal_indices) == 0x0c) ? 1 : -1];
typedef char check_leafportal_size[(sizeof(ScenarioStructureBSPGlobalLeafPortal) == 0x18) ? 1 : -1];
typedef char check_leafportal_verts[(__builtin_offsetof(ScenarioStructureBSPGlobalLeafPortal, vertices) == 0x0c) ? 1 : -1];
typedef char check_bsp3dnode_size[(sizeof(ModelCollisionGeometryBSP3DNode) == 0x0c) ? 1 : -1];
typedef char check_cbsp_planes[(__builtin_offsetof(ModelCollisionGeometryBSP, planes) == 0x0c) ? 1 : -1];
typedef char check_fog_tag[(__builtin_offsetof(Fog, maximum_density) == 0x58
                            && __builtin_offsetof(Fog, opaque_distance) == 0x60
                            && __builtin_offsetof(Fog, opaque_depth) == 0x68
                            && __builtin_offsetof(Fog, color) == 0x78
                            && __builtin_offsetof(Fog, flags_1) == 0x84
                            && __builtin_offsetof(Fog, background_sound) == 0xf4
                            && __builtin_offsetof(Fog, sound_environment) == 0x104) ? 1 : -1];
typedef char check_sky_fog[(__builtin_offsetof(Sky, indoor_fog_screen) == 0x98) ? 1 : -1];
typedef char check_shader_type[(__builtin_offsetof(Shader, shader_type) == 0x24) ? 1 : -1];
typedef char check_shader_env[(sizeof(ShaderEnvironment) == 0x344) ? 1 : -1];
typedef char check_soundenv[(sizeof(SoundEnvironment) == 0x48
                             && __builtin_offsetof(SoundEnvironment, priority) == 0x04
                             && __builtin_offsetof(SoundEnvironment, room_intensity) == 0x08
                             && __builtin_offsetof(SoundEnvironment, hf_reference) == 0x34) ? 1 : -1];

// enum values the module dispatches on
typedef char check_overlap[(_structure_bsp_overlap_none == 0
                            && _structure_bsp_overlap_partial == 1
                            && _structure_bsp_overlap_contained == 2) ? 1 : -1];
typedef char check_plane_mode[(_structure_fog_plane_unbounded == 2) ? 1 : -1];
typedef char check_visible_extent[(k_maximum_visible_clusters * (int)sizeof(structure_bsp_visible_cluster)
                                   == 0xd000) ? 1 : -1];
typedef char check_index_extent[(k_maximum_visible_surfaces * 4 == 0x10000) ? 1 : -1];
typedef char check_layer_extent[(!PTRS32 || k_maximum_detail_object_layers * k_maximum_detail_object_batches_per_layer
                                  * (int)sizeof(detail_object_batch) == 0x5100) ? 1 : -1];

int main(void) {
  polygon2d polygon;
  structure_bsp_leaf_map leaf_map;
  structure_bsp_visible_cluster visible;
  structure_fog_environment fog;
  detail_object_globals detail;
  detail_object_cell_key key;
  polygon.point_count = 0;
  leaf_map.leaves.count = 0;
  visible.cluster_index = -1;
  fog.plane_mode = _structure_fog_plane_none;
  detail.frames[0].valid = 0;
  key.cell_x = 0;
  return (int)(sizeof(polygon) + sizeof(leaf_map) + sizeof(visible) + sizeof(fog)
             + sizeof(detail) + sizeof(key) + sizeof(structure_bsp_cluster_surface_run)
             + sizeof(structure_bsp_mirror_result) + sizeof(detail_object_batch)
             + sizeof(detail_object_layer_batches) + sizeof(detail_object_render_list)
             + sizeof(detail_object_frame) + sizeof(data_array) + sizeof(datum_index)
             + sizeof(ScenarioStructureBSP) + sizeof(real_rectangle3d) + sizeof(real_matrix4x3));
}
