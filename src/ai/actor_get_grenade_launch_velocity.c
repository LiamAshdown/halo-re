// actor_get_grenade_launch_velocity  (Ghidra: actor_get_grenade_launch_velocity, renamed)
// address 0x410980, size 221 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (verified vs objdump 0x410980..0x410a5c)
// evidence: indexes the 0x44-stride grenade type table hanging off the globals block at
//   0x00746fa0+300 by the grenade type in AX, follows entry+0x40 to a projectile tag, asks
//   the ballistics solver 0x4beec0 for a solution and then scales the caller direction
//   vector by the solved speed. Its one caller is actor_commit_grenade_toss @0x411180.
// register convention: the grenade type is in AX and the direction vector in ESI; the eight
//   stack parameters are the ones Ghidra recognized, in its order.
//
// UNSURE: the call site in actor_commit_grenade_toss passes an 8-argument list whose shape
// does not line up with how this body reads param_5, param_7 and param_8, and the caller
// then reads twelve bytes of its frame that nothing in the visible argument list writes.
// One of the two frames is being mis-split by the decompiler. Both sides are preserved as
// decompiled and the disagreement is left for the hook pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include "objects.h"
#include "projectiles.h" // Projectile
#include <stdint.h>

extern tag_instance *tag_instances; // 0x0087bc14
extern Globals *global_globals;
extern float k_physics_gravity;   // 0x0069c52c

extern uint8_t projectile_get_aiming_vector(real_point3d *target, real *speed_in, Projectile *tag,
    real_point3d *origin, void *unused_param_3, real *max_time, real *max_speed_override,
    uint8_t use_high_arc, real_vector3d *out_direction, real *out_speed,
    real *out_time_or_fraction, real *out_range_or_length, uint8_t *out_used_straight_line);
    // 0x4beec0, src/ai; blam-cc: ECX target, EAX speed_in, the rest on the stack

// blam-cc: AX -> grenade_type, ESI -> direction; the rest on the stack
// Resolves the grenade type to its projectile tag, solves the throw and reports the launch
// velocity (the caller direction scaled by the solved speed) and, optionally, the negated
// gravity term for that projectile. Returns 0 when the type has no projectile or the solver
// fails.
uint8_t actor_get_grenade_launch_velocity(int16_t grenade_type, real_vector3d *direction,
                                          void *param_1, float range, real_point3d *point,
                                          int32_t param_4, float *speed, void *param_6,
                                          real_vector3d *out_velocity, float *out_gravity)
{
    uint8_t *entry;
    uint32_t projectile_tag;
    void *projectile_definition;
    float scale;
    uint8_t used_straight_line; // [esp+0xf]

    entry = (uint8_t *)global_globals->grenades.pointer + (int32_t)grenade_type * 0x44;
    if (entry == (uint8_t *)0) {
        return 0;
    }
    projectile_tag = *(uint32_t *)(entry + 0x40);
    if (projectile_tag == 0xffffffff) {
        return 0;
    }

    projectile_definition = tag_instances[projectile_tag & 0xffff].data;
    if (projectile_definition == (void *)0) {
        return 0;
    }
    // 0x4109ca..0x4109f1 (orphan pass 4 review; the earlier call passed 6 of 13 arguments):
    // ECX = point (stack arg 2), EAX = &range (stack arg 1, a float whose address is taken as
    // the speed override), origin = param_1 (arg 0), max_time = param_4 (arg 3), out_direction
    // = direction (ESI), out_speed = speed (arg 4, EBP), out_time = param_6 (arg 5), and a local
    // byte for "used the straight-line solver". Stack args 2, 4, 5 and 9 are zero.
    used_straight_line = 0;
    if (projectile_get_aiming_vector(point, &range, (Projectile *)projectile_definition,
            (real_point3d *)param_1, 0, (real *)(uintptr_t)param_4, 0, 0, direction, speed,
            (real *)param_6, 0, &used_straight_line) == 0) {
        return 0;
    }

    if (out_velocity != (real_vector3d *)0) {
        scale = *speed;
        out_velocity->i = scale * direction->i;
        out_velocity->j = scale * direction->j;
        out_velocity->k = scale * direction->k;
    }
    if (out_gravity != (float *)0) {
        // 0x410a1e..0x410a49: zero for a straight-line solution (the constant at 0x00672ac0)
        if (used_straight_line) {
            *out_gravity = 0.0f;
        } else {
            *out_gravity = -(k_physics_gravity *
                             *(float *)((uint8_t *)projectile_definition + 0x1cc));
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x410980):

/* WARNING: Removing unreachable block (ram,0x00410a2e) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4
FUN_00410980(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            float *param_5,undefined4 param_6,float *param_7,float *param_8)

{
  float fVar1;
  uint uVar2;
  char cVar3;
  short in_AX;
  int iVar4;
  float *unaff_ESI;

  iVar4 = in_AX * 0x44 + *(int *)(DAT_00746fa0 + 300);
  if ((iVar4 == 0) || (uVar2 = *(uint *)(iVar4 + 0x40), uVar2 == 0xffffffff)) {
    return 0;
  }
  iVar4 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((iVar4 != 0) && (cVar3 = FUN_004beec0(iVar4,param_1,0,param_4,0,0), cVar3 != '\0')) {
    if (param_7 != (float *)0x0) {
      fVar1 = *param_5;
      *param_7 = fVar1 * *unaff_ESI;
      param_7[1] = fVar1 * unaff_ESI[1];
      param_7[2] = fVar1 * unaff_ESI[2];
    }
    if (param_8 != (float *)0x0) {
      *param_8 = -(_DAT_0069c52c * *(float *)(iVar4 + 0x1cc));
    }
    return 1;
  }
  return 0;
}
#endif
