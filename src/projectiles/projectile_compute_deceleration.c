// projectile_compute_deceleration  (Ghidra: FUN_004c0310; renamed per
// out/phase4/projectiles_types_notes.md: "fills 0x254..0x260 from the air or water damage
// range depending on object flag 0x10")
// address 0x4c0310, size 224 bytes
// name confidence: 0.7   rewrite confidence: 0.9 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: types/projectiles.h projectile_data.deceleration_delay/_rate/deceleration/
//   deceleration_end_range (0x254/0x258/0x25c/0x260); types/tags.h Projectile.air_damage_range
//   (0x1d0), .water_damage_range (0x1dc), .initial_velocity (0x1e4). Per
//   out/phase4/projectiles_types_notes.md item 3, BOTH branches store water_damage_range[1]
//   into deceleration_end_range -- the air branch reading the water field is a retail bug, kept
//   as-is rather than "corrected" to air_damage_range[1].
// register convention: object index in EAX (in_EAX).
// blam-cc: EAX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern real projectile_deceleration_from_range(Projectile *tag, real r0, real r1); // 0x4c03f0

void projectile_compute_deceleration(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Projectile *tag = (Projectile *)tag_instances[(uint16_t)obj->definition_tag].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    real near_range;

    if ((obj->flags & _object_in_water_bit) == 0) {
        near_range = tag->air_damage_range[0];
        proj->deceleration = projectile_deceleration_from_range(tag, tag->air_damage_range[0], tag->air_damage_range[1]);
    } else {
        near_range = tag->water_damage_range[0];
        proj->deceleration = projectile_deceleration_from_range(tag, tag->water_damage_range[0], tag->water_damage_range[1]);
    }
    proj->deceleration_end_range = tag->water_damage_range[1]; // retail bug in the air branch, see file header

    if (near_range > 0.0f) {
        proj->deceleration_delay_rate = near_range / tag->initial_velocity;
        return;
    }
    proj->deceleration_delay = 1.0f;
    proj->deceleration_delay_rate = 0.0f;
}

#if 0
Original Ghidra decompilation (0x4c0310):

void FUN_004c0310(void)

{
  uint *puVar1;
  uint in_EAX;
  int extraout_ECX;
  int extraout_ECX_00;
  int extraout_EDX;
  int extraout_EDX_00;
  int iVar2;
  float10 fVar3;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((puVar1[4] & 0x10) == 0) {
    fVar3 = (float10)FUN_004c03f0(*(undefined4 *)(iVar2 + 0x1d0),*(undefined4 *)(iVar2 + 0x1d4));
    *(float *)(extraout_EDX_00 + 0x25c) = (float)fVar3;
    *(undefined4 *)(extraout_EDX_00 + 0x260) = *(undefined4 *)(extraout_ECX_00 + 0x1e0);
    iVar2 = extraout_EDX_00;
    if (0.0 < *(float *)(extraout_ECX_00 + 0x1d0)) {
      *(float *)(extraout_EDX_00 + 600) =
           *(float *)(extraout_ECX_00 + 0x1d0) / *(float *)(extraout_ECX_00 + 0x1e4);
      return;
    }
  }
  else {
    fVar3 = (float10)FUN_004c03f0(*(undefined4 *)(iVar2 + 0x1dc),*(undefined4 *)(iVar2 + 0x1e0));
    *(float *)(extraout_EDX + 0x25c) = (float)fVar3;
    *(undefined4 *)(extraout_EDX + 0x260) = *(undefined4 *)(extraout_ECX + 0x1e0);
    iVar2 = extraout_EDX;
    if (0.0 < *(float *)(extraout_ECX + 0x1dc)) {
      *(float *)(extraout_EDX + 600) =
           *(float *)(extraout_ECX + 0x1dc) / *(float *)(extraout_ECX + 0x1e4);
      return;
    }
  }
  *(undefined4 *)(iVar2 + 0x254) = 0x3f800000;
  *(undefined4 *)(iVar2 + 600) = 0;
  return;
}
#endif
