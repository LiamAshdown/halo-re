// camera_initialize  (Ghidra: camera_initialize, already named)
// address 0x445580, size 104 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// reviewed (phase 4 gate): objdump 0x445580..0x4455e7; matches (data dword zero modelled as 4 byte stores).
// evidence: every global this function zeroes or seeds matches a types/camera.h director /
//   director_globals / camera_input_axis_state field exactly (mode, mode_changed, pov_proc,
//   transition_time, look_scale, unknown_c0, unknown_4c, unknown_50, the director_camera_data
//   union's first dword, and the four camera_input_axis_state entries seeded from
//   camera_input_axes[].reset_value).
// register convention: __cdecl, no arguments (confirmed: no stack or register reads before the
//   first global store).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"

extern director_globals camera_director_globals;         // 0x006ac558
extern director directors[1];                     // 0x006ac560
extern camera_input_axis_definition camera_input_axes[4]; // 0x00686a28

extern void camera_first_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command); // 0x446d60, this module

// Resets the camera subsystem to its default state: following mode, first person as the active
// pov procedure, a fresh transition, unit look scale, and every debug-look axis reset to its
// definition's neutral value.
void camera_initialize(void)
{
    int i;

    camera_director_globals.mode = _director_camera_mode_following;
    camera_director_globals.mode_changed = 0;

    directors[0].unknown_50 = 0;
    directors[0].unknown_4c = 0;
    directors[0].transition_time = 0.0f;
    directors[0].data.raw[0] = 0;
    directors[0].data.raw[1] = 0;
    directors[0].data.raw[2] = 0;
    directors[0].data.raw[3] = 0;
    directors[0].pov_proc = camera_first_person_compute_pov;
    directors[0].look_scale = 1.0f;
    directors[0].unknown_c0 = 0;

    for (i = 0; i < k_camera_input_axis_count; i++) {
        directors[0].axes[i].value = camera_input_axes[i].reset_value;
        directors[0].axes[i].velocity = 0.0f;
        directors[0].axes[i].delta = 0.0f;
    }
}

#if 0
Original Ghidra decompilation (0x445580):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void camera_initialize(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  int iVar3;

  DAT_006ac55c = 0;
  DAT_006ac55e = 0;
  DAT_006ac5b0 = 0;
  _DAT_006ac5ac = 0;
  DAT_006ac564 = 0;
  _DAT_006ac56c = 0;
  DAT_006ac568 = camera_first_person_compute_pov;
  _DAT_006ac624 = 0x3f800000;
  DAT_006ac620 = 0;
  puVar2 = &DAT_00686a34;
  puVar1 = &DAT_006ac630;
  iVar3 = 4;
  do {
    puVar1[-2] = *puVar2;
    *puVar1 = 0;
    puVar1[-1] = 0;
    puVar2 = puVar2 + 7;
    puVar1 = puVar1 + 3;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  return;
}
#endif
