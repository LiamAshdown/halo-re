// actor_commit_grenade_toss  (Ghidra: actor_commit_grenade_toss, renamed)
// address 0x411180, size 295 bytes
// name confidence: 0.4   rewrite confidence: 0.9
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
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14


extern uint8_t actor_get_grenade_launch_velocity(int16_t grenade_type, real_vector3d *direction, void *origin,
    float range, real_point3d *point, int32_t max_time, float *speed, void *out_time_or_fraction, real_vector3d *out_velocity,
    float *out_gravity); // 0x410980, AX, ESI, stack
extern uint8_t actor_grenade_parabolic_path_clear(real_vector3d *initial_velocity, datum_index source_actor_index,
    real_point3d *start_position, real total_time, real vertical_acceleration, datum_index exclude_object_index,
    uint8_t wide_mask); // 0x42b5d0, EAX, ECX, EDX, stack

// REWRITTEN from objdump 0x411180..0x4112a6: solve a throw from the actor's position at the point (variant grenade
//   type +0x180, range +0x190), check its arc is clear of everything but `exclude`, then commit it: impact point
//   +0x6a8, target prop +0x6b4, excluded object +0x6b8, direction +0x6bc, speed +0x6c8, +0x6a1 cleared. The draft
//   called both helpers without their register operands.
// blam-cc: EBX -> actor_index, stack -> point, target_prop, exclude_object
uint32_t actor_commit_grenade_toss(datum_index actor_index, real_point3d *point, uint32_t object_handle, uint32_t exclude_object_index)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *variant = (uint8_t *)tag_instances[((actor *)a)->actor_variant_tag & 0xffff].data;
    real_point3d origin = *(real_point3d *)&((actor *)a)->aim_origin.x;    // [esp+0x14]
    real_vector3d direction;                               // [esp+0x20]
    real_vector3d velocity;                                // [esp+0x2c]
    float speed;                                           // [esp+0x10]
    float flight_time;                                     // [esp+0xc]
    float gravity;                                         // [esp+0x3c], the argument slot reused

    if (!actor_get_grenade_launch_velocity(*(int16_t *)&((ActorVariant *)variant)->grenade_type, &direction, &origin,
                                           ((ActorVariant *)variant)->grenade_velocity, point, 0, &speed, &flight_time,
                                           &velocity, &gravity)) {
        return 0;
    }
    if (!actor_grenade_parabolic_path_clear(&velocity, actor_index, &origin, flight_time, gravity, exclude_object_index,
                                            (uint8_t)(((actor *)a)->active_unit_index != k_datum_index_none))) {
        return 0;
    }
    *(real_point3d *)&((actor *)a)->grenade_impact_point.x = *point;
    ((struct actor *)a)->grenade_target_prop_index = object_handle;
    *(real_vector3d *)&((actor *)a)->grenade_unknown_6bc = direction;
    *(uint32_t *)(a + 0x6b8) = exclude_object_index;
    ((actor *)a)->grenade_unknown_6c8 = speed;
    a[0x6a1] = 0;
    return 1;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
