// motion_sensor_object_is_detected  (Ghidra: already named)
// address 0x4b36a0, size 244 bytes
// name confidence: 0.55   rewrite confidence: 0.8 (checked against objdump 0x4b36a0..0x4b3793 in the s2 part 2 review)
// evidence: out/phase4/interface_functions.md "Determines whether a given object should
// currently register as a blip on the local player's motion sensor."; types/units.h
// unit_data::control_flags (+0x208, primary_trigger bit 0x800), throwing_grenade_state
// (+0x28d).
// register convention: object datum_index in the one recovered parameter (param_1, stack
// per Ghidra, but every other function in this pair of files takes it in EAX/ECX -- not
// independently confirmed here).
// UNSURE: DAT_006f1d20 (a "motion sensor override active" flag) and DAT_006f1cc0 (the
// override's own detected/visible byte) are named speculatively; when the override is
// active but its low bit is clear the function returns false immediately, but when the low
// bit is set the code does NOT return early -- it falls through and the later movement
// check can still override it back to false. This is preserved exactly as decompiled rather
// than "fixed" into a clean early return. DAT_00710300 (a fallback "moving" byte, read only
// when the unit's velocity is below the motion-sensor detection threshold) is also unnamed.
// unit+0x204 bit 0x10 and the threshold at hud_globals_tag_data+0x2d4 (a HudGlobals field
// with no header type) are raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "units.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern game_engine_definition *current_game_engine; // 0x006f1d20, UNSURE name
extern uint8_t motion_sensor_override_value;   // 0x006f1cc0, UNSURE name (bit 0 read here)
extern uint8_t motion_sensor_force_moving;     // 0x00710300, UNSURE name

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
                                              real_vector3d *out_angular_velocity); // 0x4f6aa0

uint8_t motion_sensor_object_is_detected(datum_index unit_index)
{
    unit_data *unit;
    real_vector3d velocity, angular;
    uint8_t visible;
    float speed_sq, threshold;

    if (object_try_and_get(unit_index, 3) == 0) {
        return 0;
    }
    if (current_game_engine != 0) {
        if ((motion_sensor_override_value & 1) == 0) {
            return 0;
        }
        // override says detected, but (per the original) execution still falls through to
        // the movement check below, which can still turn this back into "not detected"
    }

    unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data
                          + k_unit_data_offset);
    if ((unit->control_flags & 0x800) != 0) { // primary_trigger
        return 1;
    }
    if (unit->throwing_grenade_state != 0 && unit->throwing_grenade_state != 3) {
        return 1;
    }

    // objdump 0x4b370f: EDI (the angular velocity out) is 0, only the linear velocity is asked for
    object_get_root_object_velocities((uint32_t)unit_index, &velocity, (real_vector3d *)0);

    visible = (current_game_engine == 0 && (unit->flags & _unit_flag_unknown_10) != 0)
              ? 0 : 1;

    speed_sq = velocity.i * velocity.i + velocity.j * velocity.j + velocity.k * velocity.k;
    threshold = *(float *)((uint8_t *)hud_globals_tag_data + 0x2d4);

    if (!(speed_sq < threshold)) {
        return visible ? 1 : 0;
    }
    if (motion_sensor_force_moving != 0) {
        return visible ? 1 : 0;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4b36a0):

uint motion_sensor_object_is_detected(uint param_1)

{
  float fVar1;
  char cVar2;
  float fVar3;
  bool bVar4;
  int iVar5;
  uint uVar6;
  undefined4 uVar7;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  uint3 uVar8;

  iVar5 = object_try_and_get(3);
  uVar6 = 0;
  if (iVar5 == 0) {
LAB_004b378c:
    return uVar6 & 0xffffff00;
  }
  if (DAT_006f1d20 != 0) {
    uVar6 = CONCAT31((int3)((uint)DAT_006f1d20 >> 8),(byte)DAT_006f1cc0) & 0xffffff01;
    if (((byte)DAT_006f1cc0 & 1) == 0) goto LAB_004b378c;
  }
  iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  uVar6 = *(uint *)(iVar5 + 0x208);
  if ((uVar6 & 0x800) != 0) goto LAB_004b3784;
  cVar2 = *(char *)(iVar5 + 0x28d);
  uVar6 = CONCAT31((int3)(uVar6 >> 8),cVar2);
  if ((cVar2 != '\0') && (cVar2 != '\x03')) goto LAB_004b3784;
  FUN_004f6aa0();
  if ((DAT_006f1d20 == 0) && ((*(byte *)(iVar5 + 0x204) & 0x10) != 0)) {
    bVar4 = false;
  }
  else {
    bVar4 = true;
  }
  fVar3 = local_c * local_c + local_8 * local_8 + local_4 * local_4;
  fVar1 = *(float *)(DAT_0071941c + 0x2d4);
  uVar7 = CONCAT22((short)((uint)DAT_006f1d20 >> 0x10),
                   (ushort)(fVar3 < fVar1) << 8 | (ushort)(NAN(fVar3) || NAN(fVar1)) << 10 |
                   (ushort)(fVar3 == fVar1) << 0xe);
  if (fVar3 < fVar1 == 0) {
LAB_004b3772:
    uVar6 = CONCAT31((int3)((uint)uVar7 >> 8),1);
  }
  else {
    uVar8 = (uint3)((uint)uVar7 >> 8);
    uVar7 = CONCAT31(uVar8,DAT_00710300);
    if (DAT_00710300 != '\0') goto LAB_004b3772;
    uVar6 = (uint)uVar8 << 8;
  }
  if ((!bVar4) || ((char)uVar6 == '\0')) {
    return uVar6 & 0xffffff00;
  }
LAB_004b3784:
  return CONCAT31((int3)(uVar6 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
