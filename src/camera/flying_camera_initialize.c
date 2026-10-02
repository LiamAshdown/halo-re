// flying_camera_initialize  (Ghidra: FUN_00446350; renamed)
// address 0x446350, size 280 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/camera_types_notes.md "0x446350: flying camera initialize (not a device
//   camera)". The first call fills flying_camera_home (.bss 0x006f1800) from the first
//   ScenarioPlayerStartingLocation (Scenario +0x354 count / +0x358 pointer: position +0x00,
//   facing +0x0c) or zeroes it; every call then seeds the editor_camera_data record from that
//   home (position, yaw = atan2 of the facing, pitch recovered through the unit forward, roll
//   0, fov 70 degrees), remembers the player 0 record in flying_camera_data, and, when the
//   flying sub-mode is not 0, tail-jumps into flying_camera_transition_procs[mode][1] with the
//   record as its only argument (0x44645c overwrites the stack argument slot with EAX).
// register convention (objdump 0x4463cb mov ecx,eax / stores through eax; 0x4463be cmp word
//   [esp+0x8]; caller 0x445f5d..0x445f61): record in EAX, local player index on the stack.
//   // blam-cc: EAX -> data, stack -> local_player_index
// Faithful oddity: the scenario branch does not write the home pitch (0x006f1810), so it keeps
//   its .bss zero; only the no-scenario branch zeroes it explicitly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Scenario *global_scenario;                           // 0x00746f8c, cache module
extern uint8_t flying_camera_home_initialized;              // 0x006f17ff
extern flying_camera_home flying_camera_home_location;      // 0x006f1800
extern editor_camera_data *flying_camera_data;              // 0x006f1814
extern int16_t flying_camera_current_mode;                  // 0x006f1828
extern flying_camera_transition_proc flying_camera_transition_procs[2][2]; // 0x00686ab0

// cos/sin/atan2/sqrt are single x87 instructions in the original code; declared locally instead
// of via <math.h> because -I types shadows that header name with types/math.h.
extern double cos(double x);
extern double sin(double x);
extern double atan2(double y, double x);
extern double sqrt(double x);

// blam-cc: EAX -> data, stack -> local_player_index
void flying_camera_initialize(editor_camera_data *data, int16_t local_player_index)
{
    float cos_pitch;
    float forward_x;
    float forward_y;
    float forward_z;

    if (!flying_camera_home_initialized) {
        ScenarioPlayerStartingLocation *start = 0;

        if (global_scenario->player_starting_locations.count != 0) {
            start = (ScenarioPlayerStartingLocation *)global_scenario->player_starting_locations.pointer;
        }
        if (start != 0) {
            flying_camera_home_location.position = start->position;
            flying_camera_home_location.yaw = start->facing;
        } else {
            flying_camera_home_location.position.x = 0.0f;
            flying_camera_home_location.position.y = 0.0f;
            flying_camera_home_location.position.z = 0.0f;
            flying_camera_home_location.yaw = 0.0f;
            flying_camera_home_location.pitch = 0.0f;
        }
    }

    cos_pitch = (float)cos(flying_camera_home_location.pitch);
    flying_camera_home_initialized = 1;
    forward_x = (float)cos(flying_camera_home_location.yaw) * cos_pitch;
    forward_y = (float)sin(flying_camera_home_location.yaw) * cos_pitch;
    forward_z = (float)sin(flying_camera_home_location.pitch);

    data->position.y = 0.0f;
    data->position.x = 0.0f;
    data->yaw = 0.0f;
    data->pitch = 0.0f;
    data->roll = 0.0f;
    data->field_of_view = 1.2217305f; // 70 degrees (0x3f9c61aa)
    data->position = flying_camera_home_location.position;
    data->yaw = (float)atan2(forward_y, forward_x);
    data->pitch = (float)atan2(forward_z, sqrt(forward_x * forward_x + forward_y * forward_y));

    if (local_player_index == 0) {
        flying_camera_data = data;
    }
    if (flying_camera_current_mode != 0) {
        flying_camera_transition_procs[flying_camera_current_mode][1](data);
    }
}

#if 0
Original Ghidra decompilation (0x446350):

void FUN_00446350(short param_1)

{
  undefined4 *puVar1;
  undefined4 *in_EAX;
  float10 fVar2;
  float10 fVar3;
  float10 fVar4;
  unkbyte10 Var5;

  if (DAT_006f17ff == '\0') {
    if ((*(int *)(global_scenario + 0x354) == 0) ||
       (puVar1 = *(undefined4 **)(global_scenario + 0x358), puVar1 == (undefined4 *)0x0)) {
      DAT_006f1800 = 0;
      DAT_006f1804 = 0;
      DAT_006f1808 = 0;
      _DAT_006f180c = 0.0;
      _DAT_006f1810 = 0.0;
    }
    else {
      DAT_006f1800 = *puVar1;
      DAT_006f1804 = puVar1[1];
      DAT_006f1808 = puVar1[2];
      _DAT_006f180c = (float)puVar1[3];
    }
  }
  fVar2 = (float10)fcos((float10)_DAT_006f1810);
  DAT_006f17ff = 1;
  fVar3 = (float10)fcos((float10)_DAT_006f180c);
  fVar3 = fVar3 * fVar2;
  fVar4 = (float10)fsin((float10)_DAT_006f180c);
  fVar4 = fVar4 * fVar2;
  Var5 = fsin((float10)_DAT_006f1810);
  in_EAX[1] = 0;
  *in_EAX = 0;
  in_EAX[3] = 0;
  in_EAX[4] = 0;
  in_EAX[5] = 0;
  in_EAX[6] = 0x3f9c61aa;
  *in_EAX = DAT_006f1800;
  in_EAX[1] = DAT_006f1804;
  in_EAX[2] = DAT_006f1808;
  fVar2 = (float10)fpatan(fVar4,fVar3);
  in_EAX[3] = (float)fVar2;
  fVar2 = (float10)fpatan(Var5,SQRT(fVar4 * fVar4 + fVar3 * fVar3));
  in_EAX[4] = (float)fVar2;
  if (param_1 == 0) {
    DAT_006f1814 = in_EAX;
  }
  if (DAT_006f1828 != 0) {
                    /* WARNING: Could not recover jumptable at 0x00446460. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (**(code **)(&DAT_00686ab4 + DAT_006f1828 * 8))();
    return;
  }
  return;
}

objdump of the tail call Ghidra could not recover:
  44645c: mov DWORD PTR [esp+0x4],eax        ; the stack argument becomes the record
  446460: jmp DWORD PTR [edx*8+0x686ab4]     ; flying_camera_transition_procs[mode][1]
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
