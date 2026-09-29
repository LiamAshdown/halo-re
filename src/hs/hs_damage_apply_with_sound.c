// hs_damage_apply_with_sound  (Ghidra: FUN_00488a40)
// address 0x488a40, size 198 bytes
// name confidence: 0.4 (out/phase4/hs_functions.md: "Applies a damage effect (with an embedded
//   sound) to an object using the same request structure shape as the flag sound-impulse helper")
// rewrite confidence: 0.4
// evidence: identical 0x54-byte request layout to hs_damage_apply_at_location.c (0x488960).
// register convention: object index in EBX (unaff_EBX). Damage/effect reference as the recognized
//   stack parameter (param_1).
//   // blam-cc: EBX -> object_index, stack -> damage_effect
// UNSURE: object_get_position() is called with zero visible arguments; `object_index` is
// threaded through here on the assumption it is the position source (matching the function's own
// gate check), which is not directly observable in the decompile.
// reconciled: R06 global_matg_multiplayer (0x00746f9c) -> ScenarioStructureBSP *global_structure_bsp; the +0xe4 read is the leaves block POINTER (0x488a0a/0x488ad0: mov ecx,[bsp+0xe4]; and eax,0x7fffffff; cluster = WORD [ptr + leaf*0x10 + 8]), the old code indexed the struct itself

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern void *memset(void *dst, int32_t value, uint32_t size); // CRT
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX out, ECX object
    // objects module, 0x4f6900, UNSURE args
extern int32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX, ECX, EDX
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern void object_apply_damage(void *dd, uint32_t object_index, int16_t hit_node_index, int16_t hit_region_index,
    int16_t hit_material_index, uint32_t hit_plane); // 0x4ee5e0

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (formerly global_matg_multiplayer)

// hs_damage_request: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

// Applies `damage_effect` to `object_index`, at the object's current position, resolving a
// sound-impulse table entry the same way hs_damage_apply_at_location does.
void hs_damage_apply_with_sound(datum_index object_index, uint32_t damage_effect)
{
    hs_damage_request request;
    int32_t impulse;

    if (object_index != k_datum_index_none) {
        memset(&request, 0, sizeof(request));
        request.damage_effect = damage_effect;
        request.team_index = 0xffff;
        request.causer = 0xffffffff;
        request.attacker = 0xffffffff;
        request.sound_index = 0xffff;
        request.scale_a = 1.0f;
        request.scale_b = 1.0f;
        request.material_type = 0xffff; // FIXED: material type -1 (0x488a6b)

        object_get_position((real_point3d *)&request.position, object_index);
        *(Point3D *)&request.direction = *(Point3D *)&request.position;

        // FIXED (objdump 0x488aa7..0x488ac2): EAX = 0, ECX = the collision BSP, EDX = &request.position; the leaf
        //   is stored at +0x14.
        impulse = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)&request.position);
        request.sound_impulse = impulse;
        if (impulse == -1) {
            request.sound_index = 0xffff;
        } else {
            request.sound_index = ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[impulse & 0x7fffffff].cluster;
        }
        // FIXED (objdump 0x488aee..0x488af9): (&dd, object, -1, -1, -1, 0); the draft passed only the record, so
        //   the damage went to whatever object index was on the stack.
        object_apply_damage(&request, object_index, -1, -1, -1, 0);
    }
}

#if 0
Original Ghidra decompilation (0x488a40):

void FUN_00488a40(undefined4 param_1)

{
  int iVar1;
  int unaff_EBX;
  undefined4 *puVar2;
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

  if (unaff_EBX != -1) {
    puVar2 = local_54;
    for (iVar1 = 0x15; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    local_54[0] = param_1;
    local_8 = 0xffff;
    local_54[2] = 0xffffffff;
    local_54[3] = 0xffffffff;
    local_44 = 0xffff;
    local_3c = 0xffff;
    local_14 = 0x3f800000;
    local_10 = 0x3f800000;
    object_get_position();
    local_2c = local_38;
    local_28 = local_34;
    local_24 = local_30;
    local_40 = FUN_005013a0();
    if (local_40 == -1) {
      local_3c = 0xffff;
    }
    else {
      local_3c = *(undefined2 *)(local_40 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
    }
    object_apply_damage(local_54);
  }
  return;
}
#endif
