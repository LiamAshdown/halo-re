// decal_flood_surfaces  (Ghidra: FUN_0044e730; named per out/phase4/effects_types_notes.md,
// which refers to this address directly: "decal_flood_surfaces 0x44e730 reads +0x00 through the
// 0.017453292 degrees-to-radians literal as the maximum angle it will wrap a decal across, and
// +0x08 as the radius multiplier")
// address 0x44e730, size 1444 bytes
// name confidence: 0.4   rewrite confidence: 0.45 (raised from 0.2 by the phase-4 integration
// pass: the BSP tables, the clip helper and the projection-axis table all resolved to types and
// signatures that already exist in the tree, so the loop body is no longer guesswork)
// evidence: types/effects.h decal_projection (param_1: major_axis 0x54, normal_positive 0x56,
// corners 0x58, du/dv_edge0/1 0x78-0x84, inverse_determinant 0x88 -- every one of this
// function's param_1 offsets lands exactly on a decal_projection field) and
// k_decal_type_parameters (maximum_edge_angle 0x00, radius_scale 0x08, and a previously
// "no reader" unknown_04 which this function DOES read as a second, looser angle threshold on
// its single-surface fallback path -- a correction to that field's doc comment).
// 0x00746f98 is global_structure_collision_bsp, a ModelCollisionGeometryBSP: this function's
// +0x40 / +0x4c / +0x58 reads with strides 0xc / 0x18 / 0x10 are its surfaces.pointer /
// edges.pointer / vertices.pointer and the matching tag records, which is how src/physics and
// src/items already type that global.
// 0x006b0a18 is a pair of 0x60-byte polygon clip buffers: the code ping-pongs between
// &DAT_006b0a18 and &DAT_006b0a18 + 0x60 by the low bit of the edge ordinal, and the maximum
// vertex count it hands polygon2d_clip_to_plane is 0xc, so each buffer is real_point2d[12].
// register convention: param_1 (decal_projection*) through param_10 are Ghidra's own recognised
// stack parameters; nothing register-passed survived decompilation for this one. The three
// register arguments of polygon2d_clip_to_plane's siblings are taken from
// src/math/polygon2d_clip_to_planes.c, which establishes both call conventions used here.
//
// UNSURE: structure_bsp_plane_fetch_signed 0x44dad0 (the indexed BSP plane fetch, a structures/math
// function that is in this address range but belongs to another module and has no file yet) is
// called with every register argument elided. The index passed below is the surface's own
// `plane` field, which is the only plane index in scope and is what an indexed plane fetch
// wants; the earlier draft of this file passed `surface_index` instead. Its output is Ghidra's
// local_10/local_c/local_8, the same three floats the 1/256 nudge below adds in, so the output
// is a normal (or the first three components of a plane).
// UNSURE: ray_intersects_sphere_test 0x4ce6c0 takes origin/center/direction in EAX/ECX/EDX
// (src/math/ray_intersects_sphere_test.c) and only the radius survives decompilation here.
// Origin and direction are read straight off the edge's two vertices; the sphere **center** is
// not visible at all, and is taken below to be the decal's own placement position
// (projection->placement.position), which is what a "does the decal reach across this edge"
// test needs. That is inference, not evidence.
// UNSURE: param_2's exact capacity. The two counters at +0x5000 and +0x5802 and the 0x14 and 2
// byte strides are forced by this function's arithmetic; 0x400 elements is what the span
// between them allows. See decal_flood_accumulator in types/effects.h.
// UNSURE: param_7/param_8 is the surface queue the caller floods into next and param_9/param_10
// the single-surface fallback list. Neither element type nor either consumer is visible here;
// both are kept as int32_t arrays with in/out uint16_t counts.
// UNSURE: Ghidra reuses one 4-byte slot (local_58) as both the float angle and the int16 vertex
// count. The two readings are split below. The count's initial value is local_58's initial bit
// pattern, 5.60519e-45, which is integer 4 -- the four corners of the unclipped projected quad,
// which is exactly what the first clip pass consumes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98, same declaration as
    // src/physics/collision_test_movement_segment.c

// 0x006b0a18, the two ping-pong clip buffers; 0x60 bytes each, indexed by the low bit of the
// edge ordinal. Ghidra sees the individual floats as DAT_006b0a18/1c/20/24.
extern real_point2d decal_clip_buffers[2][12];

extern void structure_bsp_plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index);
    // 0x44dad0, src/structures; blam-cc: EAX out, stack planes_owner, EDX signed_index
extern real_point3d *decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis,
    const real_plane3d *plane, const real_point2d *known);
    // 0x44d860, src/math; blam-cc: stack out, AL component_sign, SI dominant_axis, EBX plane, EDI known
extern real vector3d_angle_between_4cd5e0(const real_vector3d *a, const real_vector3d *b); // 0x4cd5e0,
    // math module; blam-cc: EAX -> a, ECX -> b
extern real_plane2d *plane2d_from_points(real_plane2d *out_plane, const real_point2d *a,
    const real_point2d *b); // 0x44d950, math module, no file yet;
    // blam-cc: ECX -> out_plane, EAX -> a, EDX -> b (src/math/polygon2d_clip_to_planes.c)
extern int16_t polygon2d_clip_to_plane(real_point2d *out, int16_t count, real_point2d *in,
    real_plane2d *plane, int16_t maximum_count, uint32_t *edge_bitmask, uint8_t *clipped_flag,
    real epsilon); // 0x4caff0, math module; out in EDX
extern uint8_t ray_intersects_sphere_test(real_point3d *origin, real_point3d *center,
    real_vector3d *direction, real radius); // 0x4ce6c0, math module;
    // blam-cc: EAX -> origin, ECX -> center, EDX -> direction, stack -> radius

extern const decal_type_parameters k_decal_type_parameters[4]; // 0x006573f8
extern const projection_axis_pair k_projection_axes[6];        // 0x0065c29c, types/math.h

// Walks the edge loop of BSP surface `surface_index`, clipping the decal's projected quad
// against each edge in turn. Where an edge's opposite surface is reachable (the decal's
// placement sphere, scaled by k_decal_type_parameters[decal_type].radius_scale, crosses the
// edge), that neighbouring surface index is appended to `surface_queue` for the caller to flood
// into next. On the surfaces the decal actually covers (is_first_surface set, and the surface
// within maximum_edge_angle of the decal plane) the clipped polygon is appended to
// `accumulator`. Surfaces that fail the tight angle test but pass the looser unknown_04 one go
// on `fallback_queue` instead.
void decal_flood_surfaces(decal_projection *projection, decal_flood_accumulator *accumulator,
    int32_t surface_index, uint8_t is_first_surface, real radius, int16_t decal_type,
    int32_t *surface_queue, uint16_t *surface_queue_count, int32_t *fallback_queue,
    uint16_t *fallback_queue_count)
{
    ModelCollisionGeometryBSPSurface *surfaces;
    ModelCollisionGeometryBSPEdge *edges;
    ModelCollisionGeometryBSPVertex *bsp_vertices;
    ModelCollisionGeometryBSPSurface *surface;
    const projection_axis_pair *axes;
    real_plane3d surface_plane;     // esp+0x60: the surface's (possibly flipped) plane; Ghidra local_10 / local_c / local_8
    real angle;                     // Ghidra local_58, read as a float
    int16_t queued_count = 0;       // Ghidra local_5c, the surface_queue cursor
    int16_t fallback_count = 0;     // Ghidra local_4c, the fallback_queue cursor

    if (surface_index == -1) {
        return;
    }

    surfaces = (ModelCollisionGeometryBSPSurface *)global_structure_collision_bsp->surfaces.pointer;
    edges = (ModelCollisionGeometryBSPEdge *)global_structure_collision_bsp->edges.pointer;
    bsp_vertices = (ModelCollisionGeometryBSPVertex *)global_structure_collision_bsp->vertices.pointer;
    surface = &surfaces[surface_index];

    // Both cursors are seeded from the caller only on the first-surface pass; the fallback path
    // below is only reachable when is_first_surface is set, so neither is ever read unseeded.
    if (is_first_surface != 0) {
        queued_count = (int16_t)*surface_queue_count;
        fallback_count = (int16_t)*fallback_queue_count;
    }

    // 0x44e780..0x44e787: EAX = &surface_plane, EDX = surface->plane, stack = the collision BSP
    structure_bsp_plane_fetch_signed(&surface_plane, global_structure_collision_bsp, (int32_t)surface->plane);
    angle = vector3d_angle_between_4cd5e0(&surface_plane.normal, (const real_vector3d *)&projection->plane_i);

    axes = &k_projection_axes[projection->major_axis * 2 + projection->normal_positive];

    if (is_first_surface == 0 ||
        angle <= k_decal_type_parameters[decal_type].maximum_edge_angle * 0.017453292f) {
        int32_t edge_index = (int32_t)surface->first_edge;
        uint16_t edge_ordinal = 0;      // Ghidra local_54 in its second reading
        int16_t vertex_count = 4;       // Ghidra local_58 in its int16 reading; see file header
        uint32_t edge_bitmask = 0;      // Ghidra local_44
        uint8_t clipped_flag = 0;       // Ghidra local_5d
        real_point2d *polygon = (real_point2d *)&projection->corners[0]; // Ghidra local_48
        real_point2d *clip_out = &decal_clip_buffers[0][0];              // Ghidra pfVar11
        real_point2d previous;          // Ghidra local_34 / local_30
        real_point2d current;           // Ghidra local_3c / local_38
        real_plane2d edge_plane;        // Ghidra local_1c

        previous.x = 0.0f;
        previous.y = 0.0f;

        do {
            ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
            // bVar15: this surface sits on the edge's right side, so the edge's "far" vertex and
            // "far" surface are the ones at the low index.
            int surface_is_right = ((int32_t)edge->right_surface == surface_index);
            uint32_t far_slot = (uint32_t)!surface_is_right; // Ghidra local_40
            const real_point3d *point =
                (const real_point3d *)&bsp_vertices[(&edge->start_vertex)[far_slot]].point;

            clip_out = &decal_clip_buffers[edge_ordinal & 1][0];

            if ((int16_t)edge_ordinal == 0) {
                const real_point3d *other =
                    (const real_point3d *)&bsp_vertices[(&edge->start_vertex)[surface_is_right]].point;
                previous.x = (&other->x)[axes->i];
                previous.y = (&other->x)[axes->j];
            }

            current.x = (&point->x)[axes->i];
            current.y = (&point->x)[axes->j];

            if (plane2d_from_points(&edge_plane, &previous, &current) == 0) {
                // Degenerate edge (the two projected points coincide): nothing survives.
                vertex_count = 0;
            } else {
                vertex_count = polygon2d_clip_to_plane(clip_out, vertex_count, polygon,
                    &edge_plane, 12, &edge_bitmask, &clipped_flag, 0.0f);

                if (is_first_surface != 0 && clipped_flag != 0 && queued_count < 0x400) {
                    const real_point3d *other =
                        (const real_point3d *)&bsp_vertices[(&edge->start_vertex)[surface_is_right]].point;
                    real_vector3d along_edge;
                    along_edge.i = other->x - point->x;
                    along_edge.j = other->y - point->y;
                    along_edge.k = other->z - point->z;

                    if (ray_intersects_sphere_test((real_point3d *)point,
                            &projection->placement.position, &along_edge,
                            radius * k_decal_type_parameters[decal_type].radius_scale) != 0) {
                        // left_surface (0x10) / right_surface (0x14) picked by the same slot.
                        int32_t neighbour = (int32_t)(&edge->left_surface)[far_slot];
                        int16_t i = 0;

                        while (neighbour != -1) {
                            if (queued_count <= i) {
                                surface_queue[queued_count] = neighbour;
                                queued_count = queued_count + 1;
                                break;
                            }
                            if (surface_queue[i] == neighbour) {
                                neighbour = -1; // already queued; ends the walk
                            }
                            i = i + 1;
                        }
                    }
                }
            }

            // forward_edge (0x08) / reverse_edge (0x0c), picked by which side we came in on.
            edge_index = (int32_t)(&edge->forward_edge)[surface_is_right];
            previous = current;
            edge_ordinal = edge_ordinal + 1;
            // The input of the next clip is the output of this one, but only when the loop
            // actually continues: the original evaluates `local_48 = pfVar11` inside the second
            // half of the && , so the assignment is skipped on the edge-loop-closed exit.
        } while (edge_index != (int32_t)surface->first_edge &&
                 (polygon = clip_out, vertex_count > 0));

        // flags bit 0 two_sided, bit 1 invisible, bit 3 breakable: no decal on any of those.
        if (vertex_count > 2 &&
            vertex_count <= 0x400 - accumulator->vertex_count &&
            (surface->flags & 0x0b) == 0) {
            int16_t i;

            accumulator->visited_surfaces[accumulator->visited_surface_count] = vertex_count;
            accumulator->visited_surface_count = accumulator->visited_surface_count + 1;

            for (i = 0; i < vertex_count; i++) {
                real_point2d *clipped = &clip_out[i];
                decal_flood_vertex_record *out_vertex =
                    &accumulator->vertices[accumulator->vertex_count];
                real du = clipped->x - projection->corners[0].u;
                real dv = clipped->y - projection->corners[0].v;

                out_vertex->u = (du * projection->dv_edge1 - dv * projection->du_edge1) *
                    projection->inverse_determinant;
                out_vertex->v = -((du * projection->dv_edge0 - dv * projection->du_edge0) *
                    projection->inverse_determinant);

                // 0x44ec16..0x44ec24: out = the vertex record (position first), AL =
                // projection->normal_positive, SI = projection->major_axis, EBX = &surface_plane,
                // EDI = the clipped 2D point
                decal_plane_solve_third_axis((real_point3d *)out_vertex, projection->normal_positive,
                    projection->major_axis, &surface_plane, clipped);

                // Vertices the clipper did not create (bit clear) sit exactly on a BSP surface,
                // so they are lifted 1/256 of a world unit along the surface normal to keep the
                // decal from z-fighting the surface it lies on.
                if ((edge_bitmask & (1u << (i & 0x1f))) == 0) {
                    out_vertex->position.x += surface_plane.normal.i * 0.00390625f;
                    out_vertex->position.y += surface_plane.normal.j * 0.00390625f;
                    out_vertex->position.z += surface_plane.normal.k * 0.00390625f;
                }

                accumulator->vertex_count = accumulator->vertex_count + 1;
            }
        }
    } else {
        // The surface is too steep to take the decal itself. Walk its edge loop anyway so the
        // flood can continue past it, then put the surface on the fallback list if it is within
        // the looser second angle.
        int32_t edge_index = (int32_t)surface->first_edge;

        do {
            ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
            int surface_is_right = ((int32_t)edge->right_surface == surface_index);
            const real_point3d *point =
                (const real_point3d *)&bsp_vertices[(&edge->start_vertex)[(uint32_t)!surface_is_right]].point;

            if (queued_count < 0x400) {
                const real_point3d *other =
                    (const real_point3d *)&bsp_vertices[(&edge->start_vertex)[surface_is_right]].point;
                real_vector3d along_edge;
                along_edge.i = other->x - point->x;
                along_edge.j = other->y - point->y;
                along_edge.k = other->z - point->z;

                if (ray_intersects_sphere_test((real_point3d *)point,
                        &projection->placement.position, &along_edge,
                        radius * k_decal_type_parameters[decal_type].radius_scale) != 0) {
                    int32_t neighbour =
                        (int32_t)(&edge->left_surface)[(uint32_t)!surface_is_right];
                    int16_t i = 0;

                    while (neighbour != -1) {
                        if (queued_count <= i) {
                            surface_queue[queued_count] = neighbour;
                            queued_count = queued_count + 1;
                            break;
                        }
                        if (surface_queue[i] == neighbour) {
                            neighbour = -1;
                        }
                        i = i + 1;
                    }
                }
            }

            edge_index = (int32_t)(&edge->forward_edge)[surface_is_right];
        } while (edge_index != (int32_t)surface->first_edge);

        if (angle <= k_decal_type_parameters[decal_type].unknown_04 * 0.017453292f &&
            fallback_count < 0x400) {
            fallback_queue[fallback_count] = surface_index;
            fallback_count = fallback_count + 1;
        }
    }

    if (is_first_surface != 0) {
        *surface_queue_count = (uint16_t)queued_count;
        *fallback_queue_count = (uint16_t)fallback_count;
    }
}

#if 0
Original Ghidra decompilation (0x44e730):

void FUN_0044e730(int param_1,int param_2,int param_3,char param_4,float param_5,short param_6,
                 int param_7,undefined2 *param_8,int param_9,undefined2 *param_10)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  char cVar5;
  short sVar6;
  float *pfVar7;
  int iVar8;
  int iVar9;
  float *pfVar10;
  float *pfVar11;
  int iVar12;
  byte bVar13;
  short sVar14;
  bool bVar15;
  float10 fVar16;
  char local_5d;
  int local_5c;
  float local_58;
  uint local_54;
  int local_50;
  int local_4c;
  float *local_48;
  uint local_44;
  uint local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  undefined1 local_1c [12];
  float local_10;
  float local_c;
  float local_8;

  if (param_3 != -1) {
    iVar8 = *(int *)(DAT_00746f98 + 0x40) + param_3 * 0xc;
    if (param_4 != '\0') {
      local_5c = CONCAT22(local_5c._2_2_,*param_8);
      local_4c = CONCAT22(local_4c._2_2_,*param_10);
    }
    local_50 = iVar8;
    FUN_0044dad0(DAT_00746f98);
    fVar16 = (float10)vector3d_angle_between_4cd5e0();
    local_58 = (float)fVar16;
    if ((param_4 == '\0') ||
       (local_54 = param_6 * 0x10, local_58 <= *(float *)(&DAT_006573f8 + local_54) * 0.017453292))
    {
      iVar8 = *(int *)(iVar8 + 4);
      local_54 = 0;
      local_58 = 5.60519e-45;
      local_44 = 0;
      local_48 = (float *)(param_1 + 0x58);
      do {
        iVar8 = *(int *)(DAT_00746f98 + 0x4c) + iVar8 * 0x18;
        bVar15 = *(int *)(iVar8 + 0x14) == param_3;
        local_40 = (uint)!bVar15;
        pfVar7 = (float *)(*(int *)(iVar8 + local_40 * 4) * 0x10 + *(int *)(DAT_00746f98 + 0x58));
        pfVar11 = (float *)(&DAT_006b0a18 + (local_54 & 1) * 0x60);
        if ((short)local_54 == 0) {
          iVar9 = *(int *)(iVar8 + (uint)bVar15 * 4) * 0x10 + *(int *)(DAT_00746f98 + 0x58);
          iVar12 = ((uint)*(byte *)(param_1 + 0x56) + *(short *)(param_1 + 0x54) * 2) * 4;
          local_34 = *(float *)(iVar9 + *(short *)(&DAT_0065c29c + iVar12) * 4);
          local_30 = *(float *)(iVar9 + *(short *)(&DAT_0065c29e + iVar12) * 4);
        }
        iVar9 = ((uint)*(byte *)(param_1 + 0x56) + *(short *)(param_1 + 0x54) * 2) * 4;
        local_3c = pfVar7[*(short *)(&DAT_0065c29c + iVar9)];
        local_38 = pfVar7[*(short *)(&DAT_0065c29e + iVar9)];
        iVar9 = FUN_0044d950();
        if (iVar9 == 0) {
          local_58 = 0.0;
        }
        else {
          local_58 = (float)polygon2d_clip_to_plane
                                      (local_58,local_48,local_1c,0xc,&local_44,&local_5d,0);
          if (((param_4 != '\0') && (local_5d != '\0')) && ((short)local_5c < 0x400)) {
            pfVar10 = (float *)(*(int *)(iVar8 + (uint)bVar15 * 4) * 0x10 +
                               *(int *)(DAT_00746f98 + 0x58));
            local_28 = *pfVar10 - *pfVar7;
            local_24 = pfVar10[1] - pfVar7[1];
            local_20 = pfVar10[2] - pfVar7[2];
            cVar5 = ray_intersects_sphere_test(param_5 * *(float *)(&DAT_00657400 + param_6 * 0x10))
            ;
            if (cVar5 != '\0') {
              iVar9 = *(int *)(iVar8 + 0x10 + local_40 * 4);
              sVar6 = 0;
              while (iVar9 != -1) {
                if ((short)local_5c <= sVar6) {
                  if (iVar9 != -1) {
                    iVar12 = (int)(short)local_5c;
                    local_5c = local_5c + 1;
                    *(int *)(param_7 + iVar12 * 4) = iVar9;
                  }
                  break;
                }
                if (*(int *)(param_7 + sVar6 * 4) == iVar9) {
                  iVar9 = -1;
                }
                sVar6 = sVar6 + 1;
              }
            }
          }
        }
        iVar8 = *(int *)(iVar8 + 8 + (uint)bVar15 * 4);
        local_34 = local_3c;
        local_54 = local_54 + 1;
        local_30 = local_38;
      } while ((iVar8 != *(int *)(local_50 + 4)) && (local_48 = pfVar11, 0 < local_58._0_2_));
      if ((2 < local_58._0_2_) &&
         (((int)local_58._0_2_ <= 0x400 - *(short *)(param_2 + 0x5000) &&
          ((*(byte *)(local_50 + 8) & 0xb) == 0)))) {
        *(short *)(param_2 + 0x5002 + *(short *)(param_2 + 0x5802) * 2) = local_58._0_2_;
        *(short *)(param_2 + 0x5802) = *(short *)(param_2 + 0x5802) + 1;
        if (0 < local_58._0_2_) {
          local_48 = (float *)((uint)local_58 & 0xffff);
          bVar13 = 0;
          do {
            fVar4 = *pfVar11 - *(float *)(param_1 + 0x58);
            local_30 = pfVar11[1] - *(float *)(param_1 + 0x5c);
            fVar1 = *(float *)(param_1 + 0x7c);
            fVar2 = *(float *)(param_1 + 0x78);
            fVar3 = *(float *)(param_1 + 0x88);
            *(float *)(param_2 + 0xc + *(short *)(param_2 + 0x5000) * 0x14) =
                 (fVar4 * *(float *)(param_1 + 0x84) - local_30 * *(float *)(param_1 + 0x80)) *
                 *(float *)(param_1 + 0x88);
            *(float *)(param_2 + 0x10 + *(short *)(param_2 + 0x5000) * 0x14) =
                 -((fVar4 * fVar1 - local_30 * fVar2) * fVar3);
            FUN_0044d860(param_2 + *(short *)(param_2 + 0x5000) * 0x14);
            if ((local_44 & 1 << (bVar13 & 0x1f)) == 0) {
              pfVar7 = (float *)(param_2 + *(short *)(param_2 + 0x5000) * 0x14);
              *pfVar7 = local_10 * 0.00390625 + *pfVar7;
              pfVar7[1] = local_c * 0.00390625 + pfVar7[1];
              pfVar7[2] = local_8 * 0.00390625 + pfVar7[2];
            }
            *(short *)(param_2 + 0x5000) = *(short *)(param_2 + 0x5000) + 1;
            bVar13 = bVar13 + 1;
            pfVar11 = pfVar11 + 2;
            local_48 = (float *)((int)local_48 - 1);
          } while (local_48 != (float *)0x0);
        }
      }
    }
    else {
      iVar8 = *(int *)(iVar8 + 4);
      do {
        iVar9 = *(int *)(DAT_00746f98 + 0x4c) + iVar8 * 0x18;
        bVar15 = *(int *)(*(int *)(DAT_00746f98 + 0x4c) + 0x14 + iVar8 * 0x18) == param_3;
        pfVar11 = (float *)(*(int *)(iVar9 + (uint)!bVar15 * 4) * 0x10 +
                           *(int *)(DAT_00746f98 + 0x58));
        if ((short)local_5c < 0x400) {
          pfVar7 = (float *)(*(int *)(iVar9 + (uint)bVar15 * 4) * 0x10 +
                            *(int *)(DAT_00746f98 + 0x58));
          local_34 = *pfVar7 - *pfVar11;
          local_30 = pfVar7[1] - pfVar11[1];
          local_2c = pfVar7[2] - pfVar11[2];
          cVar5 = ray_intersects_sphere_test(param_5 * *(float *)(&DAT_00657400 + local_54));
          if (cVar5 != '\0') {
            iVar8 = *(int *)(iVar9 + 0x10 + (uint)!bVar15 * 4);
            sVar6 = 0;
            while (iVar8 != -1) {
              sVar14 = (short)local_5c;
              if (sVar14 <= sVar6) {
                if (iVar8 != -1) {
                  local_5c = local_5c + 1;
                  *(int *)(param_7 + sVar14 * 4) = iVar8;
                }
                break;
              }
              if (*(int *)(param_7 + sVar6 * 4) == iVar8) {
                iVar8 = -1;
              }
              sVar6 = sVar6 + 1;
            }
          }
        }
        iVar8 = *(int *)(iVar9 + 8 + (uint)bVar15 * 4);
      } while (iVar8 != *(int *)(local_50 + 4));
      if ((local_58 <= *(float *)(&DAT_006573fc + local_54) * 0.017453292) &&
         (sVar6 = (short)local_4c, sVar6 < 0x400)) {
        local_4c = local_4c + 1;
        *(int *)(param_9 + sVar6 * 4) = param_3;
      }
    }
    if (param_4 != '\0') {
      *param_8 = (short)local_5c;
      *param_10 = (undefined2)local_4c;
    }
  }
  return;
}
#endif
