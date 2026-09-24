// actor_commit_grenade_toss  (Ghidra: actor_commit_grenade_toss, renamed)
// address 0x411180, size 295 bytes
// name confidence: 0.4   rewrite confidence: 0.15
// evidence: phase-4 summary "computes and commits the final grenade toss parameters once a
// valid landing solution has been accepted"; on success writes actor.grenade_impact_point,
// actor.unknown_6b4/6b8/6bc/6c0/6c4/6c8 and clears actor.unknown_6a1.
// register convention: landing point in EBX -> actor_index (Ghidra's unaff_EBX is actually
// used as an actor index, not a pointer, despite the variable name), the point pointer,
// object handle and a third value are Ghidra-recognized parameters.
// UNSURE: this is one of the least-resolved files in this batch. actor_get_grenade_launch_velocity is called
// with a mix of visible and invisible arguments including `&param_1`, which appears to let
// it redirect the caller's own point pointer -- a pattern this pass cannot fully verify.
// local_18/local_14/local_10 are never visibly assigned before use, which (as elsewhere in
// this module) means they are almost certainly part of the same output buffer as local_c;
// modeled here as one combined `grenade_solution` struct. Needs the disassembly review pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14


extern uint8_t actor_get_grenade_launch_velocity(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_get_grenade_launch_velocity at 0x410980
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint8_t actor_grenade_parabolic_path_clear(float param_1, real_point3d *point, uint32_t param_3, uint32_t param_4);

// blam-cc: EBX -> actor_index; point/object handle/param_3 are Ghidra-recognized parameters
uint32_t actor_commit_grenade_toss(datum_index actor_index, real_point3d *point, uint32_t object_handle, uint32_t param_3)
{
    actor *self;
    ActorVariant *variant;
    real_point3d *original_point = point;
    real_vector3d aim_vector;
    float out_a, out_b;
    grenade_solution solution;
    uint8_t ok;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    variant = (ActorVariant *)tag_instances[self->actor_variant_tag & 0xffff].data;

    aim_vector.i = self->aim_origin.x;
    aim_vector.j = self->aim_origin.y;
    aim_vector.k = self->aim_origin.z;

    ok = actor_get_grenade_launch_velocity(&aim_vector, *(float *)((uint8_t *)variant + 400), point, 0,
                       &out_a, &out_b, &solution, &point);
    if (ok != 0 && actor_grenade_parabolic_path_clear(out_b, point, param_3, self->active_unit_index != (datum_index)k_datum_index_none) != 0) {
        self->grenade_impact_point = *original_point;
        self->unknown_6b4 = object_handle;
        self->grenade_unknown_6bc = solution.unknown_10;
        self->grenade_unknown_6c0 = solution.unknown_14;
        *(uint32_t *)self->unknown_6b8 = param_3; // originally `param_3` re-stored unchanged
        self->grenade_unknown_6c8 = out_a;
        self->unknown_6a1[0] = 0;
        self->grenade_unknown_6c4 = solution.unknown_0c;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x411180):

undefined4 FUN_00411180(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  char cVar3;
  uint unaff_EBX;
  int iVar4;
  int iVar5;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined1 local_c [12];

  puVar1 = param_1;
  iVar4 = (unaff_EBX & 0xffff) * 0x724;
  iVar5 = iVar4 + *(int *)(DAT_00880360 + 0x34);
  local_24 = *(undefined4 *)(iVar5 + 0x120);
  local_20 = *(undefined4 *)(iVar5 + 0x124);
  local_1c = *(undefined4 *)(iVar5 + 0x128);
  cVar3 = FUN_00410980(&local_24,
                       *(undefined4 *)
                        (*(int *)((*(uint *)(iVar4 + 0x5c + *(int *)(DAT_00880360 + 0x34)) & 0xffff)
                                  * 0x20 + 0x14 + DAT_0087bc14) + 400),param_1,0,&local_28,&local_2c
                       ,local_c,&param_1);
  uVar2 = param_3;
  if (cVar3 != '\0') {
    cVar3 = FUN_0042b5d0(local_2c,param_1,param_3,*(int *)(iVar5 + 0x158) != -1);
    if (cVar3 != '\0') {
      *(undefined4 *)(iVar5 + 0x6a8) = *puVar1;
      *(undefined4 *)(iVar5 + 0x6ac) = puVar1[1];
      *(undefined4 *)(iVar5 + 0x6b0) = puVar1[2];
      *(undefined4 *)(iVar5 + 0x6b4) = param_2;
      *(undefined4 *)(iVar5 + 0x6bc) = local_18;
      *(undefined4 *)(iVar5 + 0x6c0) = local_14;
      *(undefined4 *)(iVar5 + 0x6b8) = uVar2;
      *(undefined4 *)(iVar5 + 0x6c8) = local_28;
      *(undefined1 *)(iVar5 + 0x6a1) = 0;
      *(undefined4 *)(iVar5 + 0x6c4) = local_10;
      return 1;
    }
  }
  return 0;
}
#endif
