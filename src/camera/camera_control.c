// camera_control  (Ghidra: FUN_00445cc0; renamed)
// address 0x445cc0, size 243 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/camera_types_notes.md "0x445cc0: camera_control". It stores its byte
//   argument into camera_script_globals.camera_control (0x006869d0) and through the hs mirror
//   pointer at 0x0087bc0c, sets camera_script_globals.changed, and switches local player 0
//   either to the scripted pov (0x444d50) or back to the first / third person camera the seat
//   asks for (the same third person initialisation as 0x445c00 / 0x445dc0).
// register convention: cdecl, one stack byte (mov al,[esp+0x8] after push ecx). Local player 0
//   only (every director address is absolute).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"
#include "fn_camera.h"

extern uint8_t *hs_camera_control_pointer;                 // 0x0087bc0c, hs module
extern camera_script_globals camera_script;                // 0x006869d0
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern director directors[1];                              // 0x006ac560

// blam-cc: ECX -> unit, stack -> out_state; result in AX


// hs camera_control: true hands local player 0 to the scripted camera, false returns it to the
// gameplay camera its seat wants.
void camera_control(uint8_t enable)
{
    int16_t seat_camera_state;

    *hs_camera_control_pointer = enable;
    if (enable) {
        directors[0].unknown_c0 = 0;
        directors[0].pov_proc = camera_debug_compute_pov;
        directors[0].look_scale = 1.0f;
        camera_script.camera_control = enable;
        camera_script.changed = 1;
        return;
    }

    {
        int16_t third_person = camera_get_seat_camera_state(
            player_control_globals_ptr->local_players[0].unit, &seat_camera_state);

        directors[0].unknown_c0 = 0;
        directors[0].look_scale = 1.0f;
        if (third_person == 1) {
            third_person_camera_data *third = &directors[0].data.third_person;

            third->initialized = 0;
            third->unknown_01 = 0;
            third->crouch_or_jump = 0;
            third->unknown_03 = 0;
            third->unknown_04 = 0;
            third->unit = k_datum_index_none;
            third->seat_index = -1;
            third->pitch_offset = 0.0f;
            third->yaw_offset = 0.0f;
            third->distance_scale = 1.0f;
            directors[0].pov_proc = camera_third_person_compute_pov;
        } else {
            directors[0].data.first_person.field_of_view = 0.0f;
            directors[0].pov_proc = camera_first_person_compute_pov;
        }
    }
    directors[0].seat_camera_state = seat_camera_state;
    camera_script.camera_control = enable;
    camera_script.changed = 1;
}

#if 0
Original Ghidra decompilation (0x445cc0):

void FUN_00445cc0(char param_1)

{
  short sVar1;
  undefined2 local_4 [2];

  *DAT_0087bc0c = param_1;
  if (param_1 != '\0') {
    DAT_006ac620 = 0;
    DAT_006ac568 = camera_debug_compute_pov;
    _DAT_006ac624 = 0x3f800000;
    DAT_006869d0 = param_1;
    DAT_006869d1 = 1;
    return;
  }
  sVar1 = FUN_00445b20(local_4);
  DAT_006ac620 = 0;
  _DAT_006ac624 = 0x3f800000;
  if (sVar1 == 1) {
    _DAT_006ac570 = 0;
    _DAT_006ac574 = 0xffffffff;
    _DAT_006ac578 = 0xffff;
    _DAT_006ac580 = 0;
    _DAT_006ac57c = 0;
    _DAT_006ac584 = 0x3f800000;
    DAT_006ac568 = camera_third_person_compute_pov;
  }
  else {
    DAT_006ac568 = camera_first_person_compute_pov;
  }
  _DAT_006ac56c = 0;
  _DAT_006ac5b4 = local_4[0];
  DAT_006869d0 = param_1;
  DAT_006869d1 = 1;
  return;
}

Note: Ghidra folded the four byte stores to 0x006ac56c..f into the trailing "_DAT_006ac56c = 0"
on both paths. objdump shows the third person path stores the four bytes (0x445d2b..0x445d3d)
and only the first person path stores the dword (0x445d7f); the effect is the same.
#endif
