// hs_effect_spawn_at_location  (Ghidra: FUN_00488870)
// address 0x488870, size 113 bytes
// name confidence: 0.4 (out/phase4/hs_functions.md: "Spawns a visual effect positioned and
//   oriented at a scenario location (flag/camera point) table entry")
// rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: the location_index * 0x5c + Scenario+0x4e8 addressing matches Scenario::cutscene_flags
//   exactly, as in hs_object_detach_and_place_at_location.c.
// register convention: location index in AX (in_AX). Effect reference as the recognized stack
//   parameter (param_1).
//   // blam-cc: AX -> location_index, stack -> effect
// UNSURE: effect_new_with_color's parameter list (12 arguments) is preserved exactly by position and
//   literal value from the decompile; only `location_index`/`location` and the derived forward
//   vector are understood. PTR_DAT_00696714 and the trailing literal constants (0xffffffff, 1, 0,
//   1.0f, 1.0f, 0, 0, 1) are passed through unchanged, uninterpreted.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern double fcos(double x); // CRT
extern double fsin(double x);

extern void effect_new_with_color(uint32_t effect, uint32_t param_2, void *param_3, int32_t param_4,
    int32_t param_5, real_point3d *position, real_vector3d *forward, float param_8, float param_9,
    int32_t param_10, int32_t param_11, int32_t param_12); // effects module, 0x450980

extern Scenario *global_scenario;      // 0x00746f8c
extern void *unknown_00696714;         // 0x00696714, PTR_DAT_00696714, UNSURE

// Spawns `effect` at Scenario::cutscene_flags[location_index]'s position, oriented along the
// location's yaw/pitch converted to a forward vector.
void hs_effect_spawn_at_location(int16_t location_index, uint32_t effect)
{
    ScenarioCutsceneFlag *location;
    real_vector3d forward;

    location = (ScenarioCutsceneFlag *)((uint8_t *)global_scenario->cutscene_flags.pointer +
        location_index * 0x5c);
    forward.i = (float)(fcos((double)location->facing.yaw) *
        fcos((double)location->facing.pitch));
    forward.j = (float)(fsin((double)location->facing.yaw) *
        fcos((double)location->facing.pitch));
    forward.k = (float)fsin((double)location->facing.pitch);
    // UNSURE: `location->facing.pitch`/`.j` stand in for the source floats at
    // cutscene_flag+0x30/+0x34 (pitch, yaw); see hs_object_detach_and_place_at_location.c.

    effect_new_with_color(effect, 0xffffffff, unknown_00696714, 1, 0, (real_point3d *)&location->position, &forward,
        1.0f, 1.0f, 0, 0, 1);
    // UNSURE: `&location->position` stands in for `location+0x24`; ScenarioCutsceneFlag's
    // actual position field name may differ.
}

#if 0
Original Ghidra decompilation (0x488870):

void FUN_00488870(undefined4 param_1)

{
  short in_AX;
  int iVar1;
  float10 fVar2;
  float10 fVar3;
  float local_c;
  float local_8;
  float local_4;

  fVar2 = (float10)fcos((float10)*(float *)(in_AX * 0x5c + 0x34 + *(int *)(DAT_00746f8c + 0x4e8)));
  iVar1 = in_AX * 0x5c + *(int *)(DAT_00746f8c + 0x4e8);
  fVar3 = (float10)fcos((float10)*(float *)(iVar1 + 0x30));
  local_c = (float)(fVar3 * fVar2);
  fVar3 = (float10)fsin((float10)*(float *)(iVar1 + 0x30));
  local_8 = (float)(fVar3 * fVar2);
  fVar2 = (float10)fsin((float10)*(float *)(iVar1 + 0x34));
  local_4 = (float)fVar2;
  FUN_00450980(param_1,0xffffffff,PTR_DAT_00696714,1,0,iVar1 + 0x24,&local_c,0x3f800000,0x3f800000,0
               ,0,1);
  return;
}
#endif
