// hs_damage_apply_at_location  (Ghidra: FUN_00488960)
// address 0x488960, size 220 bytes
// name confidence: 0.3 (the auto-generated summary in out/phase4/hs_functions.md for this address
//   describes a sound impulse, but the actual callee is damage_apply_area_effect and the built
//   structure is a damage request -- the summary appears to be mismatched)
// rewrite confidence: 0.4
// evidence: the 0x54-byte, all-zero-initialized request structure matches out/phase4/
//   hs_types_notes.md's "the 0x54-byte damage request the effects/damage module owns" (also used
//   by hs_damage_apply_with_sound.c, 0x488a40, which shares its shape); the resolved lookup at
//   DAT_00746f9c+0xe4 matches types/hs.h's documented "global_matg_multiplayer ... sound/damage
//   lookup at +0xe4".
// register convention: location index in DX (in_DX). Damage/effect reference as the recognized
//   stack parameter (param_1).
//   // blam-cc: DX -> location_index, stack -> damage_effect
// UNSURE: field names inside hs_damage_request are inferred purely from which offsets are
// written and by whom (position appears twice, at +0x1c and +0x28); bsp3d_node_find_leaf's own meaning
// (an index into some table read at global_matg_multiplayer+0xe4, stride 0x10) is not recovered.
// reconciled: R06 global_matg_multiplayer (0x00746f9c) -> ScenarioStructureBSP *global_structure_bsp; the +0xe4 read is the leaves block POINTER (0x488a0a/0x488ad0: mov ecx,[bsp+0xe4]; and eax,0x7fffffff; cluster = WORD [ptr + leaf*0x10 + 8]), the old code indexed the struct itself
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *memset(void *dst, int32_t value, uint32_t size); // CRT
extern int32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX, ECX, EDX
extern void damage_apply_area_effect(void *request, uint32_t param_2); // effects module, 0x4edd30

extern Scenario *global_scenario;      // 0x00746f8c
extern ModelCollisionGeometryBSP *global_collision_bsp;           // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (formerly global_matg_multiplayer)

// hs_damage_request: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

// Builds and applies a damage request at Scenario::cutscene_flags[location_index]'s position
// (used for both position and direction). Resolves a sound-impulse table entry via bsp3d_node_find_leaf
// and global_matg_multiplayer+0xe4 (stride 0x10, uint16 at +8) when available.
void hs_damage_apply_at_location(int16_t location_index, uint32_t damage_effect)
{
    ScenarioCutsceneFlag *location;
    hs_damage_request request;
    int32_t impulse;

    location = (ScenarioCutsceneFlag *)((uint8_t *)global_scenario->cutscene_flags.pointer +
        location_index * 0x5c);

    memset(&request, 0, sizeof(request));
    request.damage_effect = damage_effect;
    request.team_index = 0xffff;
    request.causer = 0xffffffff;
    request.attacker = 0xffffffff;
    request.sound_index = 0xffff;
    request.scale_a = 1.0f;
    request.scale_b = 1.0f;
    request.material_type = 0xffff; // FIXED: material type -1 (0x488993)
    *(Point3D *)&request.position = location->position;
    *(Point3D *)&request.direction = location->position;

    // FIXED (objdump 0x4889d4..0x4889eb): EAX = 0, ECX = the collision BSP, EDX = &location->position; the leaf
    //   is stored in the record (+0x14). The draft passed nothing and dropped the leaf.
    impulse = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)&location->position);
    request.sound_impulse = impulse;
    if (impulse == -1) {
        request.sound_index = 0xffff;
        damage_apply_area_effect(&request, 0xffffffff);
        return;
    }
    request.sound_index = ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[impulse & 0x7fffffff].cluster;
    damage_apply_area_effect(&request, 0xffffffff);
}

#if 0
Original Ghidra decompilation (0x488960):

void FUN_00488960(undefined4 param_1)

{
  int iVar1;
  short in_DX;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_54 [4];
  undefined2 local_44;
  int local_40;
  undefined2 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_14;
  undefined4 local_10;
  undefined2 local_8;

  iVar2 = in_DX * 0x5c + *(int *)(DAT_00746f8c + 0x4e8);
  puVar3 = local_54;
  for (iVar1 = 0x15; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  local_54[0] = param_1;
  local_8 = 0xffff;
  local_54[2] = 0xffffffff;
  local_54[3] = 0xffffffff;
  local_44 = 0xffff;
  local_3c = 0xffff;
  local_14 = 0x3f800000;
  local_10 = 0x3f800000;
  local_38 = *(undefined4 *)(iVar2 + 0x24);
  local_34 = *(undefined4 *)(iVar2 + 0x28);
  local_30 = *(undefined4 *)(iVar2 + 0x2c);
  local_2c = local_38;
  local_28 = local_34;
  local_24 = local_30;
  local_40 = FUN_005013a0();
  if (local_40 == -1) {
    local_3c = 0xffff;
    damage_apply_area_effect(local_54,0xffffffff);
    return;
  }
  local_3c = *(undefined2 *)(local_40 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  damage_apply_area_effect(local_54,0xffffffff);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
