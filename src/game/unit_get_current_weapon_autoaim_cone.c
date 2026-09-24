// unit_get_current_weapon_autoaim_cone  (Ghidra: FUN_00459e80; renamed -- previous name
// "player_get_camera_frustum_extents" (symbols/review_queue.txt) does not match the fields
// actually read, see evidence below)
// address 0x459e80, size 240 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: types/tags.h Item (size 0x308, confirming tag_data+0x308 is the first Weapon-only
//   field, weapon_flags) and Weapon (zoom_levels/zoom_magnification_range/autoaim_angle/
//   autoaim_range/magnetism_angle/magnetism_range/deviation_angle laid out contiguously right
//   after the size-2 padding at 0x3d8, which places autoaim_angle at 0x3e4, autoaim_range at
//   0x3e8, magnetism_angle at 0x3ec, magnetism_range at 0x3f0 and deviation_angle at 0x3f4 --
//   matching the five offsets this function reads exactly); types/units.h unit_data
//   (current_weapon_index 0x2f2, weapons[4] 0x2f8); WeaponFlags bit 0x20 =
//   aim_assists_only_when_zoomed. Used by camera_observer_update/get_target_angles/get_target_id
//   to build the observer_target_cone (angle_a/distance_a/angle_b/distance_b) plus one extra
//   angle bound (max of deviation_angle and autoaim_angle), aspect-corrected.
// register convention: unit object handle in EAX (in_EAX), a caller zoom-state flag in EDX
//   (in_DX; -1 disables the zoom gate), output float buffer in EDI (unaff_EDI, 5 floats).
//   // blam-cc: EAX -> unit_index, EDX -> require_zoomed, EDI -> out (5 floats)
//
// UNSURE: the true return type is a single byte (every caller tests it as a char); the extra
// upper bits Ghidra's `uint` return carries are leftover register content on the failure path
// and are not modelled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern real FUN_004c2d70(void); // UNSURE: viewport aspect ratio getter, not in this batch

// Fills `out[0..3]` with the current weapon's autoaim/magnetism angle and range bounds
// (aspect-corrected, forming an observer_target_cone), plus `out[4]` = the larger of the
// weapon's deviation_angle and autoaim_angle. Fails if the unit has no ready weapon, or if the
// weapon only assists while zoomed and `require_zoomed` says it is not.
uint8_t unit_get_current_weapon_autoaim_cone(datum_index unit_index, int16_t require_zoomed, real *out)
    // blam-cc: EAX -> unit_index, EDX -> require_zoomed, EDI -> out
{
    object *unit_obj;
    unit_data *unit;
    datum_index weapon_index;
    object *weapon_obj;
    Weapon *weapon;
    real aspect;
    real inv_aspect;
    real deviation_or_autoaim;

    if (unit_index == k_datum_index_none) {
        return 0;
    }
    unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    if (unit->current_weapon_index == -1) {
        return 0;
    }
    weapon_index = unit->weapons[unit->current_weapon_index];
    if (weapon_index == k_datum_index_none) {
        return 0;
    }
    weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
    weapon = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
    if (require_zoomed == -1 && (weapon->weapon_flags & 0x20) != 0) { // aim_assists_only_when_zoomed
        return 0;
    }

    aspect = FUN_004c2d70();
    inv_aspect = 1.0f / aspect;
    out[0] = inv_aspect * weapon->autoaim_angle;
    out[1] = aspect * weapon->autoaim_range;
    out[2] = inv_aspect * weapon->magnetism_angle;
    out[3] = aspect * weapon->magnetism_range;
    deviation_or_autoaim = (weapon->deviation_angle > weapon->autoaim_angle) ? weapon->deviation_angle : weapon->autoaim_angle;
    out[4] = deviation_or_autoaim * inv_aspect;
    return 1;
}

#if 0
Original Ghidra decompilation (0x459e80), from tools/pack.py 0x459e80:

uint FUN_00459e80(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  uint in_EAX;
  undefined2 extraout_var;
  undefined3 uVar5;
  short in_DX;
  float *unaff_EDI;
  float10 fVar6;

  if (in_EAX != 0xffffffff) {
    in_EAX = *(uint *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (((*(short *)(in_EAX + 0x2f2) != -1) &&
        (in_EAX = *(uint *)(in_EAX + 0x2f8 + *(short *)(in_EAX + 0x2f2) * 4), in_EAX != 0xffffffff))
       && ((iVar4 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                         (in_EAX & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                            DAT_0087bc14), in_DX != -1 || ((*(byte *)(iVar4 + 0x308) & 0x20) == 0)))
       ) {
      fVar6 = (float10)FUN_004c2d70();
      fVar1 = (float)((float10)1.0 / fVar6);
      *unaff_EDI = (float)(((float10)1.0 / fVar6) * (float10)*(float *)(iVar4 + 0x3e4));
      unaff_EDI[1] = (float)(fVar6 * (float10)*(float *)(iVar4 + 1000));
      unaff_EDI[2] = fVar1 * *(float *)(iVar4 + 0x3ec);
      unaff_EDI[3] = (float)(fVar6 * (float10)*(float *)(iVar4 + 0x3f0));
      fVar2 = *(float *)(iVar4 + 0x3f4);
      fVar3 = *(float *)(iVar4 + 0x3e4);
      uVar5 = (undefined3)
              (CONCAT22(extraout_var,
                        (ushort)(fVar2 < fVar3) << 8 | (ushort)(NAN(fVar2) || NAN(fVar3)) << 10 |
                        (ushort)(fVar2 == fVar3) << 0xe) >> 8);
      if (fVar2 < fVar3 == 0 && (fVar2 == fVar3) == 0) {
        unaff_EDI[4] = *(float *)(iVar4 + 0x3f4) * fVar1;
        return CONCAT31(uVar5,1);
      }
      unaff_EDI[4] = *(float *)(iVar4 + 0x3e4) * fVar1;
      return CONCAT31(uVar5,1);
    }
  }
  return in_EAX & 0xffffff00;
}
#endif
