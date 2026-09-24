// weapon_trigger_get_aiming_vector  (Ghidra: FUN_004c2b40; was weapon_trigger_projectile_collision_test,
//   renamed in the orphan pass 4 review: it runs the projectile aiming solver, not a collision test)
// address 0x4c2b40, size 153 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: types/tags.h Weapon.triggers (0x4fc count, 0x500 pointer, 0x114 stride),
//   WeaponTrigger.projectile (tag_id at 0x94+0xc = 0xa0). objdump 0x4c2b40..0x4c2bd8: when the
//   trigger index is in range, the trigger's projectile tag goes to projectile_get_aiming_vector
//   (0x4beec0, src/ai) with ECX = stack arg 1 (target), EAX = 0 (no speed override) and the stack
//   (tag, arg 0 origin, 0, 0, 0, arg 2 use_high_arc, arg 3 out_direction, 0 out_speed, arg 4,
//   arg 5, arg 6). Its result is ignored: `mov al,1` follows the call. An out-of-range index
//   returns 0 (`xor al,al`). The earlier rewrite declared six stack arguments, but the function
//   reads seven ([esp+0x20] four times at rising depths, [esp+0x24] and [esp+0x38]), and it passed
//   them to the solver in the wrong slots.
// register convention: weapon object index in EAX, trigger index in CX; seven stack arguments.
//   // blam-cc: EAX -> weapon_index, CX -> trigger_index, stack -> (origin, target, use_high_arc,
//   //           out_direction, out_time, out_range, out_used_straight_line)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "projectiles.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t projectile_get_aiming_vector(real_point3d *target, real *speed_in, Projectile *tag,
    real_point3d *origin, void *unused_param_3, real *max_time, real *max_speed_override,
    uint8_t use_high_arc, real_vector3d *out_direction, real *out_speed,
    real *out_time_or_fraction, real *out_range_or_length, uint8_t *out_used_straight_line);
    // 0x4beec0, src/ai; blam-cc: ECX target, EAX speed_in, the rest on the stack

// Solves the aiming direction for a shot from origin to target with the projectile of the
// weapon's trigger_index-th trigger. Returns 1 when the trigger exists (whatever the solver
// says), 0 otherwise.
uint8_t weapon_trigger_get_aiming_vector(datum_index weapon_index, int16_t trigger_index,
    real_point3d *origin, real_point3d *target, uint8_t use_high_arc, real_vector3d *out_direction,
    real *out_time, real *out_range, uint8_t *out_used_straight_line)
{
    object *weapon_obj = ((object_header *)object_data->data)[(uint16_t)weapon_index].data;
    Weapon *weapon_tag = (Weapon *)tag_instances[(uint16_t)weapon_obj->definition_tag].data;

    if (trigger_index >= 0 && (int32_t)trigger_index < (int32_t)weapon_tag->triggers.count) {
        WeaponTrigger *trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
        Projectile *projectile_tag =
            (Projectile *)tag_instances[(uint16_t)(*(datum_index *)&trigger->projectile.tag_id)].data;

        projectile_get_aiming_vector(target, 0, projectile_tag, origin, 0, 0, 0, use_high_arc,
            out_direction, 0, out_time, out_range, out_used_straight_line);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4c2b40):

undefined4
FUN_004c2b40(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,undefined4 param_6,undefined4 param_7)

{
  int iVar1;
  uint in_EAX;
  undefined4 uVar2;
  short in_CX;

  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar2 = 0;
  if (-1 < in_CX) {
    if ((int)in_CX < *(int *)(iVar1 + 0x4fc)) {
      FUN_004beec0(*(undefined4 *)
                    ((*(uint *)(in_CX * 0x114 + 0xa0 + *(int *)(iVar1 + 0x500)) & 0xffff) * 0x20 +
                     0x14 + DAT_0087bc14),param_1,0,0,0,param_3,param_4,0,param_5,param_6,param_7);
      uVar2 = 1;
    }
  }
  return uVar2;
}
#endif
