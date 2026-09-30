// director_build_camera_input  (Ghidra: FUN_00445f90; renamed)
// address 0x445f90, size 477 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: camera_update calls it first every tick with ESI = its stack camera_input and pushes
//   the local player index; the pov procedure then receives that same record. It zeroes the
//   0x24 byte camera_input, stores the local player index and director_globals.dt, and, when
//   a mouse device exists and director_camera_switching is set and the current pov is not the
//   first or third person camera and the mouse button at mouse_state +0x0d is held, builds
//   the debug key bits, runs the axis smoothing (camera_input_axes_update, 0x446170) and fills
//   the look deltas, zoom and the four smoothed axis deltas. Field mapping of the axis deltas
//   (absolute addresses, director +0xd0 / +0xdc / +0xe8 / +0xf4) is in types/camera.h.
// register convention (objdump 0x445f98 `mov ebx,esi` then stores through ebx/esi, and the
//   caller 0x445676..0x445687 `push ebx(0); lea esi,[esp+0x7c]; call`): camera_input in ESI,
//   local player index on the stack. The result is AL.
//   // blam-cc: stack -> local_player_index, ESI -> input; result in AL
// Return value: 1 when input_get_key_state(0x1d) == 1, 0 otherwise, or 0 without a mouse
//   device or without director_camera_switching. Key 0x1d is NOT a gate on the look input;
//   the result is only returned. camera_update stores it into a dead stack slot.
//   UNSURE: meaning of key 0x1d and of mouse_state button_frames[1] (+0x0d) in this build.

#include <wchar.h>
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "camera.h"
#include "fn_camera.h"

extern void *mouse_device;                          // 0x006b1804, input module
extern uint8_t input_suppressed;                    // 0x006b15f9, input module
extern mouse_state live_mouse_state;                // 0x006b180c, input module
extern mouse_state mouse_neutral_state;             // 0x006b1828, input module
extern uint8_t director_camera_switching;           // 0x00686a98
extern director_globals camera_director_globals;    // 0x006ac558
extern director directors[1];                       // 0x006ac560

// blam-cc: ECX -> key_index; result in AL
extern uint8_t input_get_key_state(int16_t key_index); // 0x490b50, input module
// blam-cc: AX -> local_player_index, stack -> (key_bits, zoom)


// blam-cc: stack -> local_player_index, ESI -> input; result in AL
uint8_t director_build_camera_input(int16_t local_player_index, camera_input *input)
{
    director *director = &directors[local_player_index];
    mouse_state *mouse;
    uint32_t key_bits;
    uint8_t result;
    uint32_t *zero = (uint32_t *)input;
    int32_t i;

    for (i = 0; i < (int32_t)(sizeof(camera_input) / sizeof(uint32_t)); i++) {
        zero[i] = 0;
    }
    input->local_player_index = local_player_index;
    input->dt = camera_director_globals.dt;

    if (mouse_device == 0) {
        return 0;
    }
    if (!director_camera_switching) {
        return 0;
    }
    mouse = input_suppressed ? &mouse_neutral_state : &live_mouse_state;
    result = (input_get_key_state(0x1d) == 1);

    if (director->pov_proc == camera_first_person_compute_pov ||
        director->pov_proc == camera_third_person_compute_pov ||
        mouse->button_frames[1] == 0) {
        return result;
    }

    key_bits = (input_get_key_state(0x20) != 0);
    if (input_get_key_state(0x2e)) key_bits |= 0x02; else key_bits &= ~0x02u;
    if (input_get_key_state(0x2d)) key_bits |= 0x04; else key_bits &= ~0x04u;
    if (input_get_key_state(0x2f)) key_bits |= 0x08; else key_bits &= ~0x08u;
    if (input_get_key_state(0x22)) key_bits |= 0x10; else key_bits &= ~0x10u;
    if (input_get_key_state(0x30)) key_bits |= 0x20; else key_bits &= ~0x20u;
    if (input_get_key_state(0x23)) key_bits |= 0x40; else key_bits &= ~0x40u;
    if (input_get_key_state(0x31)) key_bits |= 0x80; else key_bits &= ~0x80u;

    camera_input_axes_update(local_player_index, key_bits, (float)mouse->wheel);

    input->yaw_delta = (float)mouse->x * -0.0031415927f;   // 0x00673048
    input->pitch_delta = (float)mouse->y * 0.0031415927f;  // 0x00672dd8
    input->roll_delta += director->axes[1].delta;
    input->zoom_delta = (float)mouse->wheel;
    input->move_forward += director->axes[2].delta;
    input->move_left += director->axes[3].delta;
    input->has_look_input = 1;
    input->move_up += director->axes[0].delta;
    director->look_input_consumed = 1;
    director->suppress_look_update = 1;
    return result;
}

#if 0
Original Ghidra decompilation (0x445f90):

char FUN_00445f90(short param_1)

{
  float fVar1;
  char cVar2;
  char cVar3;
  int *piVar4;
  short *unaff_ESI;
  int iVar5;
  int iVar6;
  byte bVar7;

  unaff_ESI[0] = 0;
  unaff_ESI[1] = 0;
  unaff_ESI[2] = 0;
  unaff_ESI[3] = 0;
  unaff_ESI[4] = 0;
  unaff_ESI[5] = 0;
  unaff_ESI[6] = 0;
  unaff_ESI[7] = 0;
  unaff_ESI[8] = 0;
  unaff_ESI[9] = 0;
  unaff_ESI[10] = 0;
  unaff_ESI[0xb] = 0;
  unaff_ESI[0xc] = 0;
  unaff_ESI[0xd] = 0;
  unaff_ESI[0xe] = 0;
  unaff_ESI[0xf] = 0;
  unaff_ESI[0x10] = 0;
  unaff_ESI[0x11] = 0;
  iVar5 = (int)param_1;
  *unaff_ESI = param_1;
  iVar6 = iVar5 * 0xf8;
  *(undefined4 *)(unaff_ESI + 2) = DAT_006ac558;
  cVar2 = '\0';
  if (DAT_006b1804 != 0) {
    if (DAT_00686a98 == '\0') {
      cVar2 = '\0';
    }
    else {
      piVar4 = &DAT_006b1828;
      if (DAT_006b15f9 == '\0') {
        piVar4 = &DAT_006b180c;
      }
      cVar2 = FUN_00490b50();
      cVar2 = '\x01' - (cVar2 != '\x01');
      if ((((code *)(&DAT_006ac568)[iVar5 * 0x3e] != camera_first_person_compute_pov) &&
          ((code *)(&DAT_006ac568)[iVar5 * 0x3e] != camera_third_person_compute_pov)) &&
         (*(char *)((int)piVar4 + 0xd) != '\0')) {
        cVar3 = FUN_00490b50();
        bVar7 = cVar3 != '\0';
        cVar3 = FUN_00490b50();
        if (cVar3 != '\0') {
          bVar7 = bVar7 | 2;
        }
        cVar3 = FUN_00490b50();
        if (cVar3 != '\0') {
          bVar7 = bVar7 | 4;
        }
        cVar3 = FUN_00490b50();
        if (cVar3 != '\0') {
          bVar7 = bVar7 | 8;
        }
        cVar3 = FUN_00490b50();
        if (cVar3 != '\0') {
          bVar7 = bVar7 | 0x10;
        }
        cVar3 = FUN_00490b50();
        if (cVar3 != '\0') {
          bVar7 = bVar7 | 0x20;
        }
        cVar3 = FUN_00490b50();
        if (cVar3 != '\0') {
          bVar7 = bVar7 | 0x40;
        }
        cVar3 = FUN_00490b50();
        if (cVar3 != '\0') {
          bVar7 = bVar7 | 0x80;
        }
        FUN_00446170(bVar7,(float)piVar4[2]);
        *(float *)(unaff_ESI + 4) = (float)*piVar4 * -0.0031415927;
        *(float *)(unaff_ESI + 6) = (float)piVar4[1] * 0.0031415927;
        *(float *)(unaff_ESI + 8) = (float)(&DAT_006ac63c)[iVar5 * 0x3e] + *(float *)(unaff_ESI + 8)
        ;
        *(float *)(unaff_ESI + 0x10) = (float)piVar4[2];
        *(float *)(unaff_ESI + 10) = *(float *)(&DAT_006ac648 + iVar6) + *(float *)(unaff_ESI + 10);
        *(float *)(unaff_ESI + 0xc) =
             *(float *)(&DAT_006ac654 + iVar6) + *(float *)(unaff_ESI + 0xc);
        fVar1 = (float)(&DAT_006ac630)[iVar5 * 0x3e];
        *(undefined1 *)(unaff_ESI + 1) = 1;
        *(float *)(unaff_ESI + 0xe) = fVar1 + *(float *)(unaff_ESI + 0xe);
        (&DAT_006ac5b2)[iVar6] = 1;
        (&DAT_006ac5b1)[iVar6] = 1;
        return cVar2;
      }
    }
  }
  return cVar2;
}

objdump for the arguments Ghidra dropped:
  445ffa: mov ecx,0x1d / call 0x490b50       ; key indices travel in ECX
  4460ef: mov eax,[esp+0x14]                  ; AX = local_player_index for 0x446170
  4460f3: push ecx / fstp [esp] / push ebp    ; (key_bits, (float)mouse->wheel)
#endif
