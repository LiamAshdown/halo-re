// structure_decals_update_switch_transitions  (Ghidra: FUN_005530d0; named here)
// address 0x5530d0, size 672 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/tags.h ScenarioStructureBSPCluster.first_decal_index/decal_count (+0x0c/+0x0e);
//   types/structures.h's confirmation that ScenarioStructureBSPRuntimeDecal.decal_type/yaw/pitch
//   (+0x0c/+0x0e/+0x0f) and their 2*pi/254, pi/254 scales come from exactly this function;
//   types/tags.h Scenario.decal_palette (ScenarioDecalPalette, a plain `decal` TagDependency) and
//   Decal.layer (+0x04, confirmed field name via types/effects.h's cross-reference) resolve what
//   the two decompiled magic numbers (the tag-id lookup and the "!= 3" compare) actually are.
// register convention: stack -> switch_group_a, switch_group_b (both one-bit-per-cluster arrays),
//   cluster_count.
// UNSURE: the exact transition semantics of `entering` (calls decal_evict_object_decals) versus `leaving`
//   (runs the decal spawn loop) are inferred from the boolean algebra alone -- see the two
//   labelled comments below for the derivation. decal_place/decal_evict_object_decals (both foreign) keep
//   their literal argument lists; they are not decompiled by this pass. 0x505880 IS resolved: it
//   is collision_test_movement_segment (src/physics/collision_test_movement_segment.c), and the
//   0x50-byte "placement" local an earlier rewrite declared as int16_t[40] is a collision_result
//   (types/projectiles.h) -- which is what makes the `placement[0] == 2` test readable as
//   `result.type == _collision_result_type_structure`.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"       // bsp_leaf_reference, needed by collision_result
#include "projectiles.h"   // collision_result (the 0x50-byte record 0x505880 fills)
#include "cache.h"
#include "structures.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, physics.h/objects.h (read, not owned)
extern uint8_t *runtime_decals_suppressed;  // 0x0072278c, this module; UNSURE, see types/structures.h
extern Scenario *global_scenario;           // 0x00746f8c, game.h/hs.h (read, not owned)
extern uint8_t decals_for_all_responses; // 0x006893f5, foreign (decal system toggle)
extern uint8_t decals_enabled;                  // 0x00687004, foreign (decal system toggle)
extern tag_instance *tag_instances;         // 0x0087bc14, cache.h
extern uint32_t effect_random_seed;        // 0x00719cd4, foreign (cseries)
extern double cos(double x); // MSVC CRT
extern double sin(double x); // MSVC CRT

extern void decal_evict_object_decals(int32_t cluster_slot); // 0x44e310, foreign; UNSURE
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index, collision_result *result); // 0x505880, foreign; UNSURE
extern void decal_place(datum_index decal_tag_index, collision_result *placement, real_vector3d *direction,
    real radius_scale, uint8_t object_attached, int16_t sequence_index); // 0x44edc0

// Detects, for each of `cluster_count` clusters, a bit transition between `switch_group_a` and
// `switch_group_b` (two one-bit-per-cluster arrays), and either notifies of an object entering
// the group (decal_evict_object_decals) or -- when leaving, and the cluster's decals are not currently
// suppressed -- computes each of the cluster's runtime decals' orientation and spawns it through
// collision_test_movement_segment/decal_place subject to the decal system's enable toggles and
// per-layer overrides.
void structure_decals_update_switch_transitions(uint32_t *switch_group_a, uint32_t *switch_group_b,
    int16_t cluster_count)
{
    int32_t cluster_offset = 0;
    int32_t bit_index = 0;
    int16_t slot;

    if (global_structure_bsp->runtime_decals.count == 0) {
        *runtime_decals_suppressed = 0;
        return;
    }
    if (cluster_count < 1) {
        *runtime_decals_suppressed = 0;
        return;
    }

    for (slot = 0; ; slot = slot + 1) {
        uint32_t saved_seed = effect_random_seed;
        ScenarioStructureBSPCluster *cluster =
            (ScenarioStructureBSPCluster *)((uint8_t *)global_structure_bsp->clusters.pointer + cluster_offset);
        int cluster_has_decals = cluster->first_decal_index != (uint16_t)-1 && cluster->decal_count != 0;
        int entering; // calls decal_evict_object_decals when true
        int leaving;  // runs the decal spawn loop when true

        if (!cluster_has_decals) {
            entering = 0;
            leaving = 0;
        } else {
            uint32_t bit = 1u << (bit_index & 0x1f);
            uint32_t word = (uint32_t)((bit_index >> 5) * 4);
            int suppressed = *runtime_decals_suppressed != 0;

            // "entering": not suppressed, and this cluster's bit is set in group A but clear in B.
            entering = !suppressed &&
                (*(uint32_t *)((uint8_t *)switch_group_a + word) & bit) != 0 &&
                (*(uint32_t *)((uint8_t *)switch_group_b + word) & bit) == 0;

            // "leaving": (group A's bit is clear, or decals are suppressed) and group B's bit is
            // set. Computed unconditionally whenever the cluster has decals, even along the
            // `entering` path (matches the original's shared fall-through), and always wins over
            // a stale `entering` value from the suppressed-check short-circuit.
            leaving = ((*(uint32_t *)((uint8_t *)switch_group_a + word) & bit) == 0 || suppressed) &&
                (*(uint32_t *)((uint8_t *)switch_group_b + word) & bit) != 0;
        }

        if (entering) {
            decal_evict_object_decals(slot);
        } else {
            effect_random_seed = saved_seed;
            if (leaving && cluster->decal_count != 0) {
                int32_t i;
                for (i = 0; i < cluster->decal_count; i = i + 1) {
                    ScenarioStructureBSPRuntimeDecal *decal =
                        (ScenarioStructureBSPRuntimeDecal *)global_structure_bsp->runtime_decals.pointer +
                        cluster->first_decal_index + i;
                    ScenarioDecalPalette *decal_palette =
                        (ScenarioDecalPalette *)global_scenario->decal_palette.pointer;
                    TagID shader_tag_id = decal_palette[decal->decal_type].reference.tag_id;
                    int spawn_ok = 1;
                    real_vector3d orientation;
                    float yaw = (float)decal->yaw * 0.02473695f;
                    float pitch = (float)decal->pitch * 0.012368475f;
                    float cos_pitch = (float)cos(pitch);
                    float cos_yaw = (float)cos(yaw);

                    orientation.i = cos_yaw * cos_pitch;
                    orientation.j = (float)sin(yaw) * cos_pitch;
                    orientation.k = (float)sin(pitch);

                    if (decals_for_all_responses == 0) {
                        Decal *shader_decal = (Decal *)tag_instances[shader_tag_id.index].data;
                        if (shader_decal->layer != decallayer_alpha_tested) {
                            spawn_ok = 0;
                        }
                    }

                    if (decals_enabled != 0 && spawn_ok) {
                        collision_result placement;

                        effect_random_seed = *(uint32_t *)&decal->position.z ^
                            *(uint32_t *)&decal->position.y ^ *(uint32_t *)&decal->position.x ^ 0xdeadc0de;
                        if (collision_test_movement_segment(0x100061,
                                (real_point3d *)&decal->position, &orientation, 0xffffffff,
                                &placement) != 0 &&
                            placement.type == _collision_result_type_structure &&
                            (*(uint8_t *)tag_instances[shader_tag_id.index].data & 0x10) == 0) {
                            decal_place(*(datum_index *)&shader_tag_id, &placement, &orientation, 1.0f, 1, -1);
                        }
                    }
                    effect_random_seed = saved_seed;
                }
            }
        }

        cluster_offset = cluster_offset + sizeof(ScenarioStructureBSPCluster);
        bit_index = bit_index + 1;
        if (cluster_count <= (int16_t)(slot + 1)) {
            *runtime_decals_suppressed = 0;
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x5530d0):

void FUN_005530d0(int param_1,int param_2,short param_3)

{
  bool bVar1;
  bool bVar2;
  undefined4 uVar3;
  int iVar4;
  char cVar5;
  int iVar6;
  uint *puVar7;
  uint uVar8;
  int iVar9;
  float10 fVar10;
  float10 fVar11;
  float10 fVar12;
  float10 fVar13;
  int local_70;
  int local_6c;
  int local_64;
  float local_5c;
  float local_58;
  float local_54;
  short local_50 [40];

  iVar4 = DAT_00746f9c;
  if (*(int *)(DAT_00746f9c + 600) == 0) {
    *DAT_0072278c = '\0';
    return;
  }
  local_70 = 0;
  if (param_3 < 1) {
    *DAT_0072278c = '\0';
    return;
  }
  local_64 = 0;
  local_6c = 0;
  do {
    uVar3 = DAT_00719cd4;
    iVar9 = *(int *)(iVar4 + 0x138) + local_6c;
    if ((*(short *)(iVar9 + 0xc) == -1) || (*(short *)(iVar9 + 0xe) == 0)) {
      bVar1 = false;
LAB_00553168:
      bVar2 = false;
      if (bVar1) goto LAB_00553171;
LAB_005531ac:
      bVar1 = false;
    }
    else {
      bVar1 = true;
      if (*DAT_0072278c != '\0') goto LAB_00553168;
      uVar8 = 1 << ((byte)local_64 & 0x1f);
      iVar6 = (local_64 >> 5) * 4;
      if (((*(uint *)(iVar6 + param_1) & uVar8) == 0) || ((*(uint *)(iVar6 + param_2) & uVar8) != 0)
         ) goto LAB_00553168;
      bVar2 = true;
LAB_00553171:
      uVar8 = 1 << ((byte)local_64 & 0x1f);
      iVar6 = (local_64 >> 5) * 4;
      if ((((*(uint *)(iVar6 + param_1) & uVar8) != 0) && (*DAT_0072278c == '\0')) ||
         ((*(uint *)(iVar6 + param_2) & uVar8) == 0)) goto LAB_005531ac;
      bVar1 = true;
    }
    if (bVar2) {
      FUN_0044e310(local_70);
    }
    else {
      DAT_00719cd4 = uVar3;
      if ((bVar1) && (iVar6 = 0, *(short *)(iVar9 + 0xe) != 0)) {
        do {
          puVar7 = (uint *)((*(short *)(iVar9 + 0xc) + iVar6) * 0x10 + *(int *)(iVar4 + 0x25c));
          uVar8 = *(uint *)((uint)(byte)puVar7[3] * 0x10 + 0xc + *(int *)(global_scenario + 0x3b8));
          bVar1 = true;
          fVar10 = (float10)(int)*(char *)((int)puVar7 + 0xe) * (float10)0.02473695;
          fVar11 = (float10)(int)*(char *)((int)puVar7 + 0xf) * (float10)0.012368475;
          fVar12 = (float10)fcos(fVar11);
          fVar13 = (float10)fcos(fVar10);
          local_5c = (float)(fVar13 * fVar12);
          fVar10 = (float10)fsin(fVar10);
          local_58 = (float)(fVar10 * fVar12);
          fVar10 = (float10)fsin(fVar11);
          local_54 = (float)fVar10;
          if ((DAT_006893f5 == '\0') &&
             (*(short *)(*(int *)((uVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 4) != 3)) {
            bVar1 = false;
          }
          if ((DAT_00687004 != '\0') && (bVar1)) {
            DAT_00719cd4 = puVar7[2] ^ puVar7[1] ^ *puVar7 ^ 0xdeadc0de;
            cVar5 = FUN_00505880(0x100061,puVar7,&local_5c,0xffffffff,local_50);
            if ((cVar5 != '\0') &&
               ((local_50[0] == 2 &&
                ((**(byte **)((uVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) & 0x10) == 0)))) {
              FUN_0044edc0(uVar8,local_50,&local_5c,0x3f800000,1,0xffffffff);
            }
          }
          iVar6 = iVar6 + 1;
          DAT_00719cd4 = uVar3;
        } while (iVar6 < (int)(uint)*(ushort *)(iVar9 + 0xe));
      }
    }
    local_70 = local_70 + 1;
    local_64 = local_64 + 1;
    local_6c = local_6c + 0x68;
    if (param_3 <= (short)local_70) {
      *DAT_0072278c = '\0';
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
