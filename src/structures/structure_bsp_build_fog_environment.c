// structure_bsp_build_fog_environment  (Ghidra: FUN_00555330, still unnamed there)
// address 0x555330, size 387 bytes
// name confidence: 0.6 -- matches types/structures.h's own detailed documentation of this exact
//   address ("structure_fog_environment (the out-block 0x00555330 fills through ESI)").
// rewrite confidence: 0.55 -- clean decompile, every field resolved via
//   types/structures.h's structure_fog_environment layout and the Fog tag's own fields.
// evidence: types/structures.h structure_fog_environment and its accompanying prose; types/tags.h
//   Fog (color, maximum_density, opaque_distance, opaque_depth, flags, flags_1 -- offsets 0x78,
//   0x58, 0x60, 0x68, 0x00, 0x84, all confirmed by direct field-offset arithmetic).
// register convention: in_AX -> cluster_index, unaff_ESI -> out (structure_fog_environment *).
//   No stack parameters.
//   // blam-cc: AX -> cluster_index, ESI -> out
// UNSURE: the `_pad_2c[8]` (structure_fog_environment.unknown_04) span this function never
//   touches, per structures.h's own note; also the `*(float*)(puVar2+2)*0.0` fog-plane-vector
//   z-seed is reproduced exactly even though it is always zero (see inline comment) -- preserved
//   rather than simplified, in case it is a compiler artifact of a since-optimized-out multiply.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "structures.h"

extern ScenarioStructureBSP *structure_bsp; // 0x00746f9c
extern tag_instance *tag_instances;         // 0x0087bc14
extern Scenario *global_scenario;           // 0x00746f8c
extern uint8_t fog_plane_vector_valid;  // 0x006e3ae0: accessed as BYTE      // 0x006e3ae0
extern real_vector3d fog_plane_vector;      // 0x006e3ae4

extern uint32_t structure_bsp_resolve_fog_tag(int16_t cluster_index,
                                               ScenarioStructureBSP *structure_bsp,
                                               uint8_t use_sky); // this batch

// blam-cc: AX -> cluster_index, ESI -> out
void structure_bsp_build_fog_environment(int16_t cluster_index, structure_fog_environment *out)
{
    out->plane_mode = _structure_fog_plane_none;
    out->fog_flags = 0;
    out->screen_parameters = 0;

    uint32_t fog_tag_id = structure_bsp_resolve_fog_tag(cluster_index, structure_bsp, 0);
    uint8_t from_sky;
    if (fog_tag_id == 0xffffffff) {
        fog_tag_id = structure_bsp_resolve_fog_tag(cluster_index, structure_bsp, 1);
        from_sky = 1;
        if (fog_tag_id == 0xffffffff) {
            return;
        }
    } else {
        from_sky = 0;
    }

    Fog *fog = (Fog *)tag_instances[fog_tag_id & 0xffff].data;
    ScenarioStructureBSPCluster *cluster =
        &((ScenarioStructureBSPCluster *)structure_bsp->clusters.pointer)[cluster_index];

    if (from_sky) {
        out->flags |= 1;
    } else {
        if ((cluster->fog & 0x8000) == 0) {
            out->plane_mode = _structure_fog_plane_unbounded;
        } else {
            out->plane_mode = _structure_fog_plane_bounded;
            ScenarioStructureBSPFogPlane *fog_plane =
                &((ScenarioStructureBSPFogPlane *)structure_bsp->fog_planes.pointer)
                    [cluster->fog & 0x7fff];
            out->plane.normal.i = fog_plane->plane.vector.i;
            out->plane.normal.j = fog_plane->plane.vector.j;
            out->plane.normal.k = fog_plane->plane.vector.k;
            out->plane.d = fog_plane->plane.w;
        }
        out->color_red = fog->color.red;
        out->color_green = fog->color.green;
        out->color_blue = fog->color.blue;
        out->maximum_density = fog->maximum_density;
        out->opaque_distance = fog->opaque_distance;
        out->opaque_depth = fog->opaque_depth;

        if ((cluster->fog & 0x8000) != 0) {
            // UNSURE: multiplying by 0.0 makes this always zero in the original; preserved
            // verbatim rather than folded to a constant.
            float seed = *(float *)((uint8_t *)fog + 4) * 0.0f;
            out->plane.d = seed + out->plane.d;
            fog_plane_vector.i = seed * out->plane.normal.i;
            fog_plane_vector.j = seed * out->plane.normal.j;
            fog_plane_vector.k = seed * out->plane.normal.k;
            fog_plane_vector_valid = 1;
        }
    }

    out->fog_flags = fog->flags;
    out->screen_parameters = &fog->flags_1;
}

#if 0
Original Ghidra decompilation (0x555330):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00555330(void)

{
  undefined4 *puVar1;
  undefined2 *puVar2;
  bool bVar3;
  int iVar4;
  short in_AX;
  uint uVar5;
  uint uVar6;
  int iVar7;
  undefined2 *unaff_ESI;

  iVar4 = DAT_00746f9c;
  unaff_ESI[0xe] = 0;
  *unaff_ESI = 0;
  *(undefined4 *)(unaff_ESI + 0x24) = 0;
  uVar5 = FUN_00555270();
  if (uVar5 == 0xffffffff) {
    uVar5 = 0xffffffff;
    if (in_AX != -1) {
      uVar6 = 0xffffffff;
      if (0 < *(int *)(global_scenario + 0x30)) {
        uVar6 = *(uint *)(*(int *)(global_scenario + 0x34) + 0xc);
      }
      if ((uVar6 != 0xffffffff) &&
         (iVar7 = *(int *)((uVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), iVar7 != 0)) {
        uVar5 = *(uint *)(iVar7 + 0xa4);
      }
    }
    bVar3 = true;
    if (uVar5 == 0xffffffff) {
      return;
    }
  }
  else {
    bVar3 = false;
  }
  puVar2 = *(undefined2 **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar7 = in_AX * 0x68 + *(int *)(iVar4 + 0x138);
  if (bVar3) {
    *(byte *)(unaff_ESI + 1) = *(byte *)(unaff_ESI + 1) | 1;
  }
  else {
    if ((*(ushort *)(iVar7 + 2) & 0x8000) == 0) {
      unaff_ESI[0xe] = 2;
    }
    else {
      unaff_ESI[0xe] = 1;
      puVar1 = (undefined4 *)
               ((*(ushort *)(iVar7 + 2) & 0x7fff) * 0x20 + 4 + *(int *)(iVar4 + 0x17c));
      *(undefined4 *)(unaff_ESI + 0x10) = *puVar1;
      *(undefined4 *)(unaff_ESI + 0x12) = puVar1[1];
      *(undefined4 *)(unaff_ESI + 0x14) = puVar1[2];
      *(undefined4 *)(unaff_ESI + 0x16) = puVar1[3];
    }
    *(undefined4 *)(unaff_ESI + 0x18) = *(undefined4 *)(puVar2 + 0x3c);
    *(undefined4 *)(unaff_ESI + 0x1a) = *(undefined4 *)(puVar2 + 0x3e);
    *(undefined4 *)(unaff_ESI + 0x1c) = *(undefined4 *)(puVar2 + 0x40);
    *(undefined4 *)(unaff_ESI + 0x1e) = *(undefined4 *)(puVar2 + 0x2c);
    *(undefined4 *)(unaff_ESI + 0x22) = *(undefined4 *)(puVar2 + 0x34);
    *(undefined4 *)(unaff_ESI + 0x20) = *(undefined4 *)(puVar2 + 0x30);
    if ((*(ushort *)(iVar7 + 2) & 0x8000) != 0) {
      _DAT_006e3aec = *(float *)(puVar2 + 2) * 0.0;
      *(float *)(unaff_ESI + 0x16) = _DAT_006e3aec + *(float *)(unaff_ESI + 0x16);
      _DAT_006e3ae4 = _DAT_006e3aec * *(float *)(unaff_ESI + 0x10);
      _DAT_006e3ae8 = _DAT_006e3aec * *(float *)(unaff_ESI + 0x12);
      _DAT_006e3aec = _DAT_006e3aec * *(float *)(unaff_ESI + 0x14);
      DAT_006e3ae0 = 1;
    }
  }
  *unaff_ESI = *puVar2;
  *(undefined2 **)(unaff_ESI + 0x24) = puVar2 + 0x42;
  return;
}
#endif
