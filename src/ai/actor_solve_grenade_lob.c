// actor_solve_grenade_lob  (Ghidra: actor_solve_grenade_lob, renamed)
// address 0x410780, size 499 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (FIXED from objdump 0x410906: the path check gets EAX velocity, ECX actor, EDX point)
// evidence: it reads ActorVariant.grenade_type (0x180), walks the same 0x44-stride grenade
//   type table as actor_get_grenade_launch_velocity @0x410980 to a projectile tag, asks the
//   ballistics solver 0x4beec0 for a throw, checks the throw is forward-facing and commits
//   the result into the four grenade scratch floats types/ai.h attributes to this address
//   (grenade_unknown_6bc..6c8). Its one caller is
//   actor_compute_grenade_throw_vector @0x410a60.
// register convention: actor_index and the target point are the two Ghidra-recognized stack
//   parameters; the solver takes eleven arguments, three of which are out-parameters.
//
// UNSURE: the facing test dots the normalized horizontal throw direction against
// actor+0x174 / +0x178, which types/ai.h calls actor.position. A dot product against
// cos(30 degrees) only makes sense against a unit facing vector, so either the header name
// is wrong or this is doing something stranger. The same pattern shows up in
// actor_check_grenade_facing_and_commit.c; see src/ai/README.md.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include "objects.h"
#include "projectiles.h" // Projectile
#include "fn_ai.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern Globals *global_globals;
extern float k_physics_gravity;   // 0x0069c52c

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function


    // 0x4beec0, src/ai; blam-cc: ECX target, EAX speed_in, the rest on the stack


// blam-cc: stack -> actor_index, point
// Solves a grenade lob at the given point and commits it only when the resulting throw is
// inside a 30 degree cone of the actor facing. The committed values are the throw direction
// and the solved speed, which actor_compute_grenade_throw_vector then turns into the final
// aim vector.
uint32_t actor_solve_grenade_lob(datum_index actor_index, real_point3d *point)
{
    actor *self;
    ActorVariant *variant;
    uint8_t *entry;
    void *projectile_definition;
    uint32_t projectile_tag;
    real_vector3d direction;
    real_vector3d velocity;
    float speed;
    float arc;
    float gravity;
    float length;
    uint8_t flat;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    variant = (ActorVariant *)tag_instances[self->actor_variant_tag & 0xffff].data;

    entry = (uint8_t *)global_globals->grenades.pointer + (int32_t)variant->grenade_type * 0x44;
    projectile_definition = (void *)0;
    if (entry != (uint8_t *)0) {
        projectile_tag = *(uint32_t *)(entry + 0x40);
        if (projectile_tag != 0xffffffff) {
            projectile_definition = tag_instances[projectile_tag & 0xffff].data;
        }
    }

    // 0x4107e9..0x410823 (orphan pass 4 review; the earlier call dropped the register target and
    // shifted every stack slot): ECX = &grenade_impact_point, EAX = 0 (no speed override),
    // origin = point, max_time = 0, max_speed_override = &grenade_unknown_6c8, use_high_arc =
    // unknown_6a1[0], out_time = &arc, out_range = 0, out_used_straight_line = &flat.
    if (projectile_get_aiming_vector(&self->grenade_impact_point, 0, (Projectile *)projectile_definition,
                     point, 0, 0, &self->grenade_unknown_6c8, self->unknown_6a1[0], &direction,
                     &speed, &arc, 0, &flat) == 0) {
        return 0;
    }

    length = (float)sqrt((double)(direction.i * direction.i + direction.j * direction.j));
    if (length < 0.0001f && length > -0.0001f) {
        return 0;
    }
    if (length <= 0.0f) {
        return 0;
    }

    if (direction.i * (1.0f / length) * self->facing.i +
        (1.0f / length) * direction.j * self->facing.j <= 0.8660254f) {
        return 0;
    }

    // 0x4108be: the launch velocity (direction * speed) and the gravity the arc falls under
    velocity.i = direction.i * speed;
    velocity.j = direction.j * speed;
    velocity.k = direction.k * speed;
    gravity = (flat != 0) ? 0.0f
                          : -(k_physics_gravity *
                              *(float *)((uint8_t *)projectile_definition + 0x1cc));

    // 0x410906: EAX the velocity, ECX the actor, EDX the throw point
    if (actor_grenade_parabolic_path_clear(&velocity, actor_index, point, arc, gravity,
                                           *(datum_index *)self->unknown_6b8,
                                           (uint8_t)(self->active_unit_index != (datum_index)0xffffffff)) == 0) {
        return 0;
    }

    self->grenade_unknown_6bc = direction.i;
    self->grenade_unknown_6c0 = direction.j;
    self->grenade_unknown_6c4 = direction.k;
    self->grenade_unknown_6c8 = speed;
    return 1;
}

#if 0
Original Ghidra decompilation (0x410780):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00410780(uint param_1,undefined4 param_2)

{
  uint uVar1;
  float fVar2;
  char cVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  char local_25;
  float local_24;
  float local_20;
  undefined4 local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar5 = (param_1 & 0xffff) * 0x724;
  iVar6 = iVar5 + *(int *)(DAT_00880360 + 0x34);
  iVar4 = *(short *)(*(int *)((*(uint *)(iVar5 + 0x5c + *(int *)(DAT_00880360 + 0x34)) & 0xffff) *
                              0x20 + 0x14 + DAT_0087bc14) + 0x180) * 0x44 +
          *(int *)(DAT_00746fa0 + 300);
  iVar5 = 0;
  if ((iVar4 != 0) && (uVar1 = *(uint *)(iVar4 + 0x40), uVar1 != 0xffffffff)) {
    iVar5 = *(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  }
  cVar3 = FUN_004beec0(iVar5,param_2,0,0,(float *)(iVar6 + 0x6c8),*(undefined1 *)(iVar6 + 0x6a1),
                       &local_c,&local_24,&local_1c,0,&local_25);
  if (cVar3 != '\0') {
    fVar2 = SQRT(local_c * local_c + local_8 * local_8);
    if (0.0001 <= ABS(fVar2)) {
      if (0.0 < fVar2) {
        if (0.8660254 <
            local_c * (1.0 / fVar2) * *(float *)(iVar6 + 0x174) +
            (1.0 / fVar2) * local_8 * *(float *)(iVar6 + 0x178)) {
          local_18 = local_c * local_24;
          local_14 = local_8 * local_24;
          local_10 = local_4 * local_24;
          if (local_25 == '\0') {
            local_20 = -(_DAT_0069c52c * *(float *)(iVar5 + 0x1cc));
          }
          else {
            local_20 = 0.0;
          }
          cVar3 = FUN_0042b5d0(local_1c,local_20,*(undefined4 *)(iVar6 + 0x6b8),
                               *(int *)(iVar6 + 0x158) != -1);
          if (cVar3 != '\0') {
            *(float *)(iVar6 + 0x6bc) = local_c;
            *(float *)(iVar6 + 0x6c0) = local_8;
            *(float *)(iVar6 + 0x6c4) = local_4;
            *(float *)(iVar6 + 0x6c8) = local_24;
            return 1;
          }
        }
      }
    }
  }
  return 0;
}
#endif
