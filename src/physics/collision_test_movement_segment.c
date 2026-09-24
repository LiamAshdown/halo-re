// collision_test_movement_segment  (Ghidra: FUN_00505880; renamed -- this is the producer every
//   other collision_result consumer in the game reads, per types/physics.h's bottom section)
// address 0x505880, size 1972 bytes
// name confidence: 0.35   rewrite confidence: 0.50 (raised from 0.45: phase-4 integration pass separated the first-leaf pair at 0x04/0x08 from the last-leaf pair at 0x0c/0x10 and moved the fog-plane material write to 0x34)
// evidence: types/physics.h struct notes for collision_bsp_segment_query/_result (0x505880
//   "independently declares int local_404[257] at exactly result+0x14, which closes the struct
//   at 0x418", fixing the seven Ghidra locals local_418..local_404[] as one
//   collision_bsp_segment_result in this function's own frame); types/physics.h bottom section
//   "what this module adds to collision_result" (every field this function stores into its
//   uint16_t* out-parameter is named there); src/objects/object_collect_in_clusters.c and
//   src/objects/object_find_in_sphere.c (the object_data / object_cluster_stamp /
//   collideable_cluster_first dedup idiom this function inlines by hand for a per-cluster
//   object walk, and the DAT_006e3f01/04 cluster-visit-stamp pair those files already name
//   cluster_flood_fill_recursion_guard / cluster_flood_fill_call_count); types/tags.h
//   ScenarioStructureBSP (leaves +0xe4 stride 0x10 cluster +0x08, collision_materials +0xa8
//   stride 0x14 material +0x12, clusters +0x138 stride 0x68, fog_planes +0x17c stride 0x20),
//   ScenarioStructureBSPFogPlane (front_region +0x00, material_type +0x02, plane +0x04).
// register convention: none observed -- Ghidra recognized all five as ordinary stack
//   parameters (`FUN_00505880(uint param_1, float *param_2, float *param_3, undefined4
//   param_4, undefined2 *param_5)`); param_4 is confirmed as the excluded-object datum index by
//   cross-referencing the call into object_collision_test_ray_nearby_chain, whose own signature names that slot
//   `param_6` and tests `param_1 != param_6` to skip the mover's own object.
// UNSURE: the fog/water-plane branch (flags bit 0x40) chases a tag-reference chain
//   (fog_region -> weather/fog palette entry -> loaded tag data +0x74) that types/physics.h
//   explicitly scopes out as belonging to a fog/atmosphere module; the offsets 0x24, 0x88,
//   0x2c and 0x194 below are kept as raw pointer arithmetic rather than named fields for that
//   reason. UNSURE also covers the exact hidden-register arguments (flags, result pointer)
//   that this function's own callees (FUN_00502060, bsp3d_node_find_leaf, scenario_location_from_point) receive --
//   Ghidra lost every one of those stores, and the prototypes below reconstruct them only from
//   the struct layouts types/physics.h already proves.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#include "cache.h"
#include "physics.h"

// collision_test_movement_segment_flags now lives in types/physics.h.

extern ModelCollisionGeometryBSP *structure_collision_bsp; // 0x00746f98
extern ScenarioStructureBSP *structure_bsp_tag_data;                // 0x00746f9c

extern data_array *object_data;                    // 0x008603b0
extern object_globals *object_globals_pointer;     // 0x006b8cbc
extern int32_t object_cluster_stamp;                // 0x008603cc
extern datum_index *collideable_cluster_first;      // 0x008603d0
extern data_array *collideable_object_references;   // 0x008603d4
extern uint8_t cluster_flood_fill_recursion_guard;  // 0x006e3f01, shared with object_find_in_sphere
extern int32_t cluster_flood_fill_call_count;       // 0x006e3f04, shared with object_find_in_sphere
extern int32_t cluster_visit_stamp[];               // 0x006e3f08, per-cluster stamp array indexed
                                                     //   by cluster index, compared against
                                                     //   cluster_flood_fill_call_count; UNSURE name

// blam-cc: EAX -> flags, ECX -> result, stack -> bsp, breakable_surface_count,
//          breakable_surfaces, origin, delta, max_fraction. Declaration copied verbatim from
//          collision_bsp_query_segment_init.c so every call site in this module agrees; the two
//          register arguments are lost in both decompiles.
extern uint8_t collision_bsp_query_segment_init(uint32_t flags,
    collision_bsp_segment_result *result, ModelCollisionGeometryBSP *bsp,
    int16_t breakable_surface_count, uint32_t *breakable_surfaces, real_point3d *origin,
    real_vector3d *delta, float max_fraction); // 0x502060, this module
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
    real_point3d *point); // 0x5013a0, this module (lower half)
// blam-cc: ESI -> out_leaf_reference (writes leaf_index and cluster_index); point comes from
// whatever bsp3d_node_find_leaf was last called with
extern void scenario_location_from_point(void *out_leaf_reference); // 0x53e780, scenario module
extern void plane3d_negate(real_plane3d *out, real_plane3d *in); // 0x44da20, effects module
// blam-cc: EAX -> object_index (the object whose chain is being walked)
extern uint8_t object_collision_test_ray_nearby_chain(uint32_t object_index, uint32_t flags, uint32_t sanitized_flags,
    real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index,
    collision_result *result); // 0x5055b0, this module (lower half)

extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78
extern int16_t global_structure_bsp_index;        // 0x0069e8d8
extern tag_instance *tag_instances;               // 0x0087bc14
extern double fabs(double x); // ABS is a single x87 FABS instruction

// Casts a movement segment (origin, origin+delta) through the world and writes the collision it
// found (if any) into *result. With none of _collision_test_flag_structure_bsp/_water_surface/
// _nearby_objects set, this only resolves the endpoint's leaf and returns false. Otherwise it
// always runs the structure-BSP segment query (to learn every leaf the segment passed through),
// applies that hit to *result when _collision_test_flag_structure_bsp is set, tests the
// destination cluster's fog plane for a water-surface crossing when _collision_test_flag_water_surface
// is set, and walks every object in every touched cluster (excluding exclude_object_index) when
// _collision_test_flag_nearby_objects is set. Finally, when _collision_test_flag_unstick is set and
// something was hit, nudges the contact point forward off the surface and re-resolves its leaf;
// if that still lands outside all geometry it backs the point down along -delta in small steps
// until a valid leaf is found again.
uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result)
{
    uint8_t hit;
    real_point3d *point;
    bsp_leaf_reference *first_leaf_ref;
    bsp_leaf_reference *last_leaf_ref;
    int32_t leaf_index;

    hit = 0;
    // collision_result carries TWO {leaf, cluster} pairs: the FIRST leaf of the segment walk at
    // 0x04/0x08 (inside what types/projectiles.h still calls unknown_04[8]) and the LAST at
    // 0x0c/0x10, which is the one projectiles.h already names `leaf`. See types/physics.h's
    // "what this module adds to collision_result" table. An earlier rewrite of this file aliased
    // both onto result->leaf, so 0x04/0x08 were never written at all.
    first_leaf_ref = (bsp_leaf_reference *)&result->unknown_04[0];
    last_leaf_ref = (bsp_leaf_reference *)&result->leaf;
    result->type = -1;
    first_leaf_ref->leaf_index = -1;
    first_leaf_ref->cluster_index = -1;
    last_leaf_ref->leaf_index = -1;
    last_leaf_ref->cluster_index = -1;
    result->t = 1.0f;

    if ((flags & (_collision_test_flag_structure_bsp | _collision_test_flag_water_surface |
                  _collision_test_flag_nearby_objects)) == 0) {
        result->t = 1.0f;
        result->point.x = origin->x + delta->i;
        result->point.y = origin->y + delta->j;
        result->point.z = origin->z + delta->k;
        leaf_index = bsp3d_node_find_leaf(0, structure_collision_bsp, &result->point);
        last_leaf_ref->leaf_index = leaf_index;
        if (leaf_index == -1) {
            last_leaf_ref->cluster_index = -1;
            return 0;
        }
        last_leaf_ref->cluster_index =
            ((ScenarioStructureBSPLeaf *)structure_bsp_tag_data->leaves.pointer)[leaf_index].cluster;
        return 0;
    }

    {
        uint32_t object_type_mask = flags >> 7;
        uint32_t sanitized_flags = flags;
        uint32_t segment_flags;
        collision_bsp_segment_result seg_result;
        uint32_t found_surface;

        if ((sanitized_flags & 3) == 0) {
            sanitized_flags |= 3; // default: test both front and back faces
        }
        segment_flags = sanitized_flags & 0x1f;

        found_surface = collision_bsp_query_segment_init(segment_flags, &seg_result,
            structure_collision_bsp, k_maximum_breakable_surfaces_per_bsp,
            breakable_surface_state->active[global_structure_bsp_index],
            origin, delta, 3.4028235e+38f /* 0x7f7fffff, FLT_MAX */);

        if (found_surface && (flags & _collision_test_flag_structure_bsp) != 0) {
            result->t = seg_result.t;
            result->type = 2;
            result->normal.i = ((real_plane3d *)seg_result.plane)->normal.i;
            result->normal.j = ((real_plane3d *)seg_result.plane)->normal.j;
            result->normal.k = ((real_plane3d *)seg_result.plane)->normal.k;
            result->unknown_30 = ((real_plane3d *)seg_result.plane)->d;
            if (seg_result.plane_index < 0) {
                result->normal.i = -result->normal.i;
                result->normal.j = -result->normal.j;
                result->normal.k = -result->normal.k;
                result->unknown_30 = -result->unknown_30;
            }
            if (seg_result.material_index == -1) {
                result->material_type = -1;
            } else {
                result->material_type = ((ScenarioStructureBSPCollisionMaterial *)
                    structure_bsp_tag_data->collision_materials.pointer)[seg_result.material_index].material;
            }
            result->surface_index = seg_result.surface_index;
            result->unknown_48 = (uint32_t)seg_result.plane_index; // 0x48; physics.h names this
                                                                    // plane_index, sign bit set
                                                                    // on a back-face hit
            result->surface_flags = seg_result.surface_flags;
            result->unknown_4d = seg_result.breakable_surface_index;
            result->unknown_4e = seg_result.material_index;
            hit = 1;
        }

        if (seg_result.leaf_count > 0) {
            int32_t first_leaf = seg_result.leaves[0];
            int32_t last_leaf = seg_result.leaves[seg_result.leaf_count - 1];

            first_leaf_ref->leaf_index = first_leaf;
            first_leaf_ref->cluster_index = (first_leaf == -1) ? -1 :
                ((ScenarioStructureBSPLeaf *)structure_bsp_tag_data->leaves.pointer)[first_leaf].cluster;

            last_leaf_ref->leaf_index = last_leaf;
            last_leaf_ref->cluster_index = (last_leaf == -1) ? -1 :
                ((ScenarioStructureBSPLeaf *)structure_bsp_tag_data->leaves.pointer)[last_leaf].cluster;
        }

        // Water-surface test: does the destination cluster have a fog plane, and does the
        // segment cross its altitude? UNSURE: the tag-reference chase below (fog_region ->
        // palette entry -> loaded tag data +0x74) is foreign to this module; see file header.
        if ((flags & _collision_test_flag_water_surface) != 0 && last_leaf_ref->cluster_index != -1) {
            ScenarioStructureBSPCluster *cluster = &((ScenarioStructureBSPCluster *)
                structure_bsp_tag_data->clusters.pointer)[last_leaf_ref->cluster_index];
            int16_t fog = (int16_t)cluster->fog;
            if (fog != -1 && fog < 0) {
                ScenarioStructureBSPFogPlane *fog_plane = &((ScenarioStructureBSPFogPlane *)
                    structure_bsp_tag_data->fog_planes.pointer)[fog & 0x7fff];
                if (fog_plane->material_type != -1) {
                    float ni = fog_plane->plane.vector.i;
                    float nj = fog_plane->plane.vector.j;
                    float nk = fog_plane->plane.vector.k;
                    // UNSURE: fog_region -> fog_palette -> loaded Fog tag +0x74 chase; the Fog
                    // tag itself (and its +0x74 field, presumably a world-space altitude) is
                    // foreign to this module, see file header.
                    uint16_t fog_palette_index = ((ScenarioStructureBSPFogRegion *)
                        structure_bsp_tag_data->fog_regions.pointer)[fog_plane->front_region].fog;
                    ScenarioStructureBSPFogPalette *palette = &((ScenarioStructureBSPFogPalette *)
                        structure_bsp_tag_data->fog_palette.pointer)[fog_palette_index];
                    uint32_t fog_tag_index = palette->fog.tag_id.index;
                    float world_offset = *(float *)((uint8_t *)tag_instances[fog_tag_index].data + 0x74);
                    float d = fog_plane->plane.w - world_offset;
                    float side_a = (ni * origin->x + nk * origin->z + nj * origin->y) - d;
                    float side_b = ni * delta->i + nk * delta->k + nj * delta->j;
                    if ((0.0f < side_a) != (0.0f < side_b) &&
                        (float)fabs((double)side_a) < (float)fabs((double)side_b) &&
                        0.0001f <= (float)fabs((double)side_b) &&
                        -(side_a / side_b) < result->t) {
                        result->t = -(side_a / side_b);
                        result->normal.i = ni;
                        result->normal.j = nj;
                        result->normal.k = nk;
                        result->type = 0;
                        result->unknown_30 = d;
                        if (0.0f <= side_a) {
                            // param_5[0x1a] is byte offset 0x34 -- material_type, not 0x4e
                            result->material_type = fog_plane->material_type;
                            hit = 1;
                        } else {
                            real_plane3d negated;
                            plane3d_negate(&negated, (real_plane3d *)&result->normal);
                            result->normal.i = negated.normal.i;
                            result->normal.j = negated.normal.j;
                            result->normal.k = negated.normal.k;
                            result->unknown_30 = negated.d;
                            result->material_type = 0x1c; // param_5[0x1a] == +0x34
                            hit = 1;
                        }
                    }
                }
            }
        }

        // Nearby-object test: walk every object in every cluster the segment's leaf list
        // touched, deduping clusters and objects the same way object_collect_in_clusters does.
        if ((object_type_mask & 1) != 0 && seg_result.leaf_count > 0) {
            int32_t stamp;
            int32_t i;

            if ((flags & _collision_test_object_type_mask_default) == 0) {
                flags |= _collision_test_object_type_mask_default;
            }
            cluster_flood_fill_call_count++;
            object_globals_pointer->collecting_in_clusters = 1;
            stamp = object_cluster_stamp + 1;
            cluster_flood_fill_recursion_guard = 1;
            object_cluster_stamp = stamp;

            for (i = 0; i < seg_result.leaf_count; i++) {
                int32_t leaf = seg_result.leaves[i];
                int16_t cluster_index = (leaf == -1) ? -1 :
                    ((ScenarioStructureBSPLeaf *)structure_bsp_tag_data->leaves.pointer)[leaf].cluster;

                // cluster_index == -1 indexes cluster_visit_stamp[-1], which IS
                // cluster_flood_fill_call_count itself (0x006e3f04 sits immediately before
                // 0x006e3f08) -- so the test always fails and a clusterless leaf is skipped.
                // The original does exactly the same thing, via `sVar11 * 4`.
                if (cluster_visit_stamp[cluster_index] != cluster_flood_fill_call_count) {
                    datum_index ref;

                    cluster_visit_stamp[cluster_index] = cluster_flood_fill_call_count;
                    ref = collideable_cluster_first[cluster_index];
                    while (ref != k_datum_index_none) {
                        object_cluster_reference *node = (object_cluster_reference *)
                            collideable_object_references->data + (ref & 0xffff);
                        datum_index object_index = node->object_index;
                        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

                        if (obj->cluster_stamp != stamp) {
                            obj->cluster_stamp = stamp;
                            if (object_collision_test_ray_nearby_chain(object_index, flags, segment_flags, origin, delta,
                                    exclude_object_index, result) != 0) {
                                hit = 1;
                            }
                        }
                        ref = node->next_reference;
                    }
                }
            }

            object_globals_pointer->collecting_in_clusters = 0;
            cluster_flood_fill_recursion_guard = 0;
        }

        if (hit == 0) {
            result->t = 1.0f;
        }
        point = &result->point;
        point->x = result->t * delta->i + origin->x;
        point->y = result->t * delta->j + origin->y;
        point->z = result->t * delta->k + origin->z;

        if ((flags & _collision_test_flag_unstick) != 0 && hit != 0) {
            int32_t resolved_leaf = last_leaf_ref->leaf_index;

            if (resolved_leaf != -1) {
                int32_t new_leaf = bsp3d_node_find_leaf(0, structure_collision_bsp, point);
                if (new_leaf != resolved_leaf) {
                    point->x += result->normal.i * 0.00024414062f;
                    point->y += result->normal.j * 0.00024414062f;
                    point->z += result->normal.k * 0.00024414062f;
                    scenario_location_from_point(last_leaf_ref);
                    if (last_leaf_ref->leaf_index == -1) {
                        float facing = delta->i * result->normal.i + delta->j * result->normal.j +
                            delta->k * result->normal.k;
                        float step = (facing == 0.0f) ? 0.03125f :
                            0.00024414062f / (float)fabs((double)facing);

                        while (1) {
                            float t = result->t - step;
                            if (t <= 0.0f) {
                                t = 0.0f;
                            }
                            result->t = t;
                            point->x = t * delta->i + origin->x;
                            point->y = t * delta->j + origin->y;
                            point->z = t * delta->k + origin->z;
                            resolved_leaf = bsp3d_node_find_leaf(0, structure_collision_bsp, point);
                            last_leaf_ref->leaf_index = resolved_leaf;
                            last_leaf_ref->cluster_index = (resolved_leaf == -1) ? -1 :
                                ((ScenarioStructureBSPLeaf *)structure_bsp_tag_data->leaves.pointer)[resolved_leaf].cluster;
                            if (result->t <= 0.0f) {
                                break;
                            }
                            if (last_leaf_ref->leaf_index != -1) {
                                return hit;
                            }
                        }
                    }
                }
            }
        }
    }

    return hit;
}

#if 0
Original Ghidra decompilation (0x505880):

char FUN_00505880(uint param_1,float *param_2,float *param_3,undefined4 param_4,undefined2 *param_5)

{
  int *piVar1;
  float *pfVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  char cVar9;
  undefined2 uVar10;
  short sVar11;
  uint uVar12;
  uint uVar13;
  short *psVar14;
  int iVar15;
  uint uVar16;
  int iVar17;
  int iVar18;
  char local_436;
  int local_434;
  undefined4 local_418;
  float *local_414;
  undefined4 local_410;
  int local_40c;
  undefined1 local_408;
  undefined1 local_407;
  short local_406;
  int local_404 [257];

  local_436 = '\0';
  piVar1 = (int *)(param_5 + 6);
  *param_5 = 0xffff;
  *(undefined4 *)(param_5 + 2) = 0xffffffff;
  param_5[4] = 0xffff;
  *piVar1 = -1;
  param_5[8] = 0xffff;
  *(undefined4 *)(param_5 + 10) = 0x3f800000;
  if ((param_1 & 0xe0) == 0) {
    *(undefined4 *)(param_5 + 10) = 0x3f800000;
    *(float *)(param_5 + 0xc) = *param_2 + *param_3;
    *(float *)(param_5 + 0xe) = param_2[1] + param_3[1];
    *(float *)(param_5 + 0x10) = param_2[2] + param_3[2];
    iVar17 = FUN_005013a0();
    *piVar1 = iVar17;
    if (iVar17 == -1) {
      param_5[8] = 0xffff;
      return '\0';
    }
    param_5[8] = *(undefined2 *)(iVar17 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  }
  else {
    uVar12 = param_1 >> 7;
    if ((param_1 & 3) == 0) {
      param_1 = param_1 | 3;
    }
    uVar13 = param_1 & 1;
    if ((param_1 & 2) != 0) {
      uVar13 = uVar13 | 2;
    }
    if ((param_1 & 4) != 0) {
      uVar13 = uVar13 | 4;
    }
    if ((param_1 & 8) != 0) {
      uVar13 = uVar13 | 8;
    }
    if ((param_1 & 0x10) != 0) {
      uVar13 = uVar13 | 0x10;
    }
    cVar9 = FUN_00502060(DAT_00746f98,0x100,DAT_0069e8d8 * 0x20 + 1 + DAT_006b8d78,param_2,param_3,
                         0x7f7fffff);
    if ((cVar9 != '\0') && ((param_1 & 0x20) != 0)) {
      *(undefined4 *)(param_5 + 10) = local_418;
      *param_5 = 2;
      pfVar2 = (float *)(param_5 + 0x12);
      *pfVar2 = *local_414;
      *(float *)(param_5 + 0x14) = local_414[1];
      *(float *)(param_5 + 0x16) = local_414[2];
      *(float *)(param_5 + 0x18) = local_414[3];
      if (local_40c < 0) {
        *pfVar2 = -*pfVar2;
        *(float *)(param_5 + 0x14) = -*(float *)(param_5 + 0x14);
        *(float *)(param_5 + 0x16) = -*(float *)(param_5 + 0x16);
        *(float *)(param_5 + 0x18) = -*(float *)(param_5 + 0x18);
      }
      if (local_406 == -1) {
        uVar10 = 0xffff;
      }
      else {
        uVar10 = *(undefined2 *)(*(int *)(DAT_00746f9c + 0xa8) + 0x12 + local_406 * 0x14);
      }
      param_5[0x1a] = uVar10;
      *(undefined4 *)(param_5 + 0x22) = local_410;
      *(int *)(param_5 + 0x24) = local_40c;
      *(undefined1 *)(param_5 + 0x26) = local_408;
      *(undefined1 *)((int)param_5 + 0x4d) = local_407;
      param_5[0x27] = local_406;
      local_436 = '\x01';
    }
    if (0 < local_404[0]) {
      *(int *)(param_5 + 2) = local_404[1];
      if (local_404[1] == -1) {
        uVar10 = 0xffff;
      }
      else {
        uVar10 = *(undefined2 *)(local_404[1] * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
      }
      param_5[4] = uVar10;
      iVar17 = local_404[local_404[0]];
      *piVar1 = iVar17;
      if (iVar17 == -1) {
        uVar10 = 0xffff;
      }
      else {
        uVar10 = *(undefined2 *)(iVar17 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
      }
      param_5[8] = uVar10;
    }
    if (((param_1 & 0x40) != 0) && (param_5[8] != -1)) {
      sVar11 = *(short *)((short)param_5[8] * 0x68 + *(int *)(DAT_00746f9c + 0x138) + 2);
      if ((sVar11 != -1) &&
         ((sVar11 < 0 &&
          (psVar14 = (short *)(((int)sVar11 & 0x7fffU) * 0x20 + *(int *)(DAT_00746f9c + 0x17c)),
          psVar14[1] != -1)))) {
        fVar3 = *(float *)(psVar14 + 2);
        fVar4 = *(float *)(psVar14 + 4);
        fVar5 = *(float *)(psVar14 + 6);
        fVar6 = *(float *)(psVar14 + 8) -
                *(float *)(*(int *)((*(uint *)(*(short *)(*(int *)(DAT_00746f9c + 0x188) + 0x24 +
                                                         *psVar14 * 0x28) * 0x88 + 0x2c +
                                              *(int *)(DAT_00746f9c + 0x194)) & 0xffff) * 0x20 +
                                    0x14 + DAT_0087bc14) + 0x74);
        fVar7 = (fVar3 * *param_2 + fVar5 * param_2[2] + fVar4 * param_2[1]) - fVar6;
        fVar8 = fVar3 * *param_3 + fVar5 * param_3[2] + fVar4 * param_3[1];
        if ((0.0 < fVar7 != 0.0 < fVar8) &&
           (((ABS(fVar7) < ABS(fVar8) && (0.0001 <= ABS(fVar8))) &&
            (-(fVar7 / fVar8) < *(float *)(param_5 + 10))))) {
          *(float *)(param_5 + 10) = -(fVar7 / fVar8);
          *(float *)(param_5 + 0x12) = fVar3;
          *(float *)(param_5 + 0x14) = fVar4;
          *(float *)(param_5 + 0x16) = fVar5;
          *param_5 = 0;
          *(float *)(param_5 + 0x18) = fVar6;
          if (0.0 <= fVar7) {
            param_5[0x1a] = psVar14[1];
            local_436 = '\x01';
          }
          else {
            plane3d_negate();
            param_5[0x1a] = 0x1c;
            local_436 = '\x01';
          }
        }
      }
    }
    if (((uVar12 & 1) != 0) && (0 < local_404[0])) {
      if ((param_1 & 0xfff00) == 0) {
        param_1 = param_1 | 0xfff00;
      }
      DAT_006e3f04 = DAT_006e3f04 + 1;
      *(undefined1 *)(DAT_006b8cbc + 1) = 1;
      iVar17 = DAT_008603b0;
      iVar18 = DAT_008603cc + 1;
      DAT_006e3f01 = 1;
      local_434 = 0;
      DAT_008603cc = iVar18;
      if (0 < local_404[0]) {
        do {
          if (local_404[local_434 + 1] == -1) {
            sVar11 = -1;
          }
          else {
            sVar11 = *(short *)(local_404[local_434 + 1] * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4))
            ;
          }
          iVar15 = sVar11 * 4;
          if (*(int *)(&DAT_006e3f08 + iVar15) != DAT_006e3f04) {
            *(int *)(&DAT_006e3f08 + iVar15) = DAT_006e3f04;
            if (*(uint *)(iVar15 + DAT_008603d0) == 0xffffffff) {
              uVar16 = 0xffffffff;
              uVar12 = 0xffffffff;
            }
            else {
              uVar16 = *(uint *)(iVar15 + DAT_008603d0) & 0xffff;
              uVar12 = *(uint *)(*(int *)(DAT_008603d4 + 0x34) + 8 + uVar16 * 0xc);
              uVar16 = *(uint *)(*(int *)(DAT_008603d4 + 0x34) + uVar16 * 0xc + 4);
            }
            while (uVar16 != 0xffffffff) {
              iVar15 = *(int *)(*(int *)(iVar17 + 0x34) + 8 + (uVar16 & 0xffff) * 0xc);
              if (*(int *)(iVar15 + 0x14) != iVar18) {
                *(int *)(iVar15 + 0x14) = iVar18;
                cVar9 = FUN_005055b0(uVar16,param_1,uVar13,param_2,param_3,param_4,param_5);
                if (cVar9 != '\0') {
                  local_436 = '\x01';
                }
              }
              if (uVar12 == 0xffffffff) {
                uVar16 = 0xffffffff;
                uVar12 = 0xffffffff;
              }
              else {
                uVar16 = uVar12 & 0xffff;
                uVar12 = *(uint *)(*(int *)(DAT_008603d4 + 0x34) + 8 + uVar16 * 0xc);
                uVar16 = *(uint *)(*(int *)(DAT_008603d4 + 0x34) + uVar16 * 0xc + 4);
              }
            }
          }
          local_434 = local_434 + 1;
        } while (local_434 < local_404[0]);
      }
      *(undefined1 *)(DAT_006b8cbc + 1) = 0;
      DAT_006e3f01 = 0;
    }
    if (local_436 == '\0') {
      *(undefined4 *)(param_5 + 10) = 0x3f800000;
    }
    fVar3 = *(float *)(param_5 + 10);
    pfVar2 = (float *)(param_5 + 0xc);
    *pfVar2 = fVar3 * *param_3 + *param_2;
    *(float *)(param_5 + 0xe) = fVar3 * param_3[1] + param_2[1];
    *(float *)(param_5 + 0x10) = fVar3 * param_3[2] + param_2[2];
    if (((param_1 & 0x100000) != 0) && (local_436 != '\0')) {
      iVar17 = *(int *)(param_5 + 6);
      piVar1 = (int *)(param_5 + 6);
      if ((iVar17 != -1) && (iVar18 = FUN_005013a0(), iVar18 != iVar17)) {
        *pfVar2 = *(float *)(param_5 + 0x12) * 0.00024414062 + *pfVar2;
        *(float *)(param_5 + 0xe) =
             *(float *)(param_5 + 0x14) * 0.00024414062 + *(float *)(param_5 + 0xe);
        *(float *)(param_5 + 0x10) =
             *(float *)(param_5 + 0x16) * 0.00024414062 + *(float *)(param_5 + 0x10);
        FUN_0053e780();
        if (*piVar1 == -1) {
          fVar3 = *param_3 * *(float *)(param_5 + 0x12) +
                  *(float *)(param_5 + 0x14) * param_3[1] + *(float *)(param_5 + 0x16) * param_3[2];
          if (fVar3 == 0.0) {
            fVar3 = 0.03125;
          }
          else {
            fVar3 = 0.00024414062 / ABS(fVar3);
          }
          while( true ) {
            fVar4 = *(float *)(param_5 + 10) - fVar3;
            if (fVar4 <= 0.0) {
              fVar4 = 0.0;
            }
            *(float *)(param_5 + 10) = fVar4;
            *pfVar2 = fVar4 * *param_3 + *param_2;
            *(float *)(param_5 + 0xe) = fVar4 * param_3[1] + param_2[1];
            *(float *)(param_5 + 0x10) = fVar4 * param_3[2] + param_2[2];
            iVar17 = FUN_005013a0();
            *piVar1 = iVar17;
            if (iVar17 == -1) {
              uVar10 = 0xffff;
            }
            else {
              uVar10 = *(undefined2 *)(iVar17 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
            }
            param_5[8] = uVar10;
            if (*(float *)(param_5 + 10) < 0.0 != (*(float *)(param_5 + 10) == 0.0)) break;
            if (*piVar1 != -1) {
              return local_436;
            }
          }
        }
      }
    }
  }
  return local_436;
}
#endif
