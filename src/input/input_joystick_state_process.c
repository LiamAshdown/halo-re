// input_joystick_state_process  (Ghidra: already named)
// address 0x491fd0, size 365 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/input_types_notes.md: "input_joystick_state_process 0x491fd0: buttons
// from raw +0xc0 (bit 7) into +0x00; POVs from raw +0x80 into +0x60, as octants; axes from raw
// +i*4 into +0x20 as int16. The counts come from the input_device in EBX (+0x234 / +0x238 /
// +0x23c)." The POV octant thresholds are exactly k_input_joystick_pov_octant (4500 hundredths
// of a degree) steps offset by half an octant, matching types/input.h's own note on that
// constant; k_input_joystick_pov_none (-1) is the low-word-0xffff centered sentinel.
// register convention: raw in the recognized param_1, dest in the recognized param_2,
// input_device in EBX (unaff_EBX)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

// VERIFIED against disassembly 0x491fd0..0x49213f (2026-09-30): button saturation, the pov threshold cascade (negative or
//   0xffff low word -> none, >= 0x83d6 -> north), axis copy and all three loop counts (+0x238/+0x23c/+0x234) match.
// blam-cc: input_device in EBX
// Normalizes a raw joystick sample (raw) into the engine's joystick_state (dest), for the axis
// count, button count, and POV count that device reports: button hold-frame counters (saturating
// at 255), POV hats quantized into 8 compass octants (or k_input_joystick_pov_none when
// centered), and axis values passed through as int16.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void input_joystick_state_process(joystick_raw_state *raw, joystick_state *dest, input_device *device)
{
    int32_t i;
    int32_t angle;
    int32_t octant;

    for (i = 0; i < device->button_count; i++) {
        if ((raw->buttons[i] & 0x80) == 0) {
            dest->button_frames[i] = 0;
        } else if (dest->button_frames[i] < 0xff) {
            dest->button_frames[i] = dest->button_frames[i] + 1;
        } else {
            dest->button_frames[i] = 0xff;
        }
    }

    for (i = 0; i < device->pov_count; i++) {
        angle = (int32_t)raw->povs[i];
        if ((int16_t)angle == -1) {
            angle = -1;
        }

        if (angle < 0) {
            // 0x492055 -> 0x4920a8: for any negative value every `jl` in the threshold
            // cascade is taken and the preset -1 (0x49204b) is stored, so all negatives are
            // centered, not only the 0xffff sentinel (checked in the phase-4 review)
            octant = k_input_joystick_pov_none;
        } else if (angle < 0x8ca) {
            octant = 0; // north
        } else if (angle < 0x1a5e) {
            octant = 1; // northeast
        } else if (angle < 0x2bf2) {
            octant = 2; // east
        } else if (angle < 0x3d86) {
            octant = 3; // southeast
        } else if (angle < 0x4f1a) {
            octant = 4; // south
        } else if (angle < 0x60ae) {
            octant = 5; // southwest
        } else if (angle < 0x7242) {
            octant = 6; // west
        } else if (angle < 0x83d6) {
            octant = 7; // northwest
        } else {
            octant = 0; // wraps back to north
        }
        dest->povs[i] = octant;
    }

    for (i = 0; i < device->axis_count; i++) {
        dest->axes[i] = (int16_t)raw->axes[i];
    }
}

#if 0
Original Ghidra decompilation (0x491fd0):

void input_joystick_state_process(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined4 uVar4;
  undefined2 *puVar5;
  undefined4 *puVar6;
  int unaff_EBX;
  int *piVar7;

  iVar1 = 0;
  if (0 < *(int *)(unaff_EBX + 0x238)) {
    do {
      if ((*(byte *)(param_1 + 0xc0 + iVar1) & 0x80) == 0) {
        uVar3 = 0;
      }
      else {
        uVar3 = *(byte *)(iVar1 + param_2) + 1;
        if (0xff < uVar3) {
          uVar3 = 0xff;
        }
      }
      *(char *)(iVar1 + param_2) = (char)uVar3;
      iVar1 = iVar1 + 1;
    } while (iVar1 < *(int *)(unaff_EBX + 0x238));
  }
  iVar1 = 0;
  if (0 < *(int *)(unaff_EBX + 0x23c)) {
    puVar6 = (undefined4 *)(param_2 + 0x60);
    piVar7 = (int *)(param_1 + 0x80);
    do {
      iVar2 = *piVar7;
      if ((short)iVar2 == -1) {
        iVar2 = -1;
      }
      uVar4 = 0xffffffff;
      if (iVar2 < 0x83d6) {
        if (iVar2 < 0) {
          if (0x8c9 < iVar2) goto LAB_004920af;
          if (0x1a5d < iVar2) goto LAB_004920c4;
          if (0x2bf1 < iVar2) goto LAB_004920d9;
          if (0x3d85 < iVar2) goto LAB_004920f1;
          if (0x4f19 < iVar2) goto LAB_00492109;
          if (0x60ad < iVar2) goto LAB_00492121;
          if (iVar2 < 0x7242) goto LAB_00492062;
LAB_0049213d:
          uVar4 = 7;
        }
        else {
          if (iVar2 < 0x8ca) goto LAB_00492060;
LAB_004920af:
          if (iVar2 < 0x1a5e) {
            uVar4 = 1;
          }
          else {
LAB_004920c4:
            if (iVar2 < 0x2bf2) {
              uVar4 = 2;
            }
            else {
LAB_004920d9:
              if (iVar2 < 0x3d86) {
                uVar4 = 3;
              }
              else {
LAB_004920f1:
                if (iVar2 < 0x4f1a) {
                  uVar4 = 4;
                }
                else {
LAB_00492109:
                  if (iVar2 < 0x60ae) {
                    uVar4 = 5;
                  }
                  else {
LAB_00492121:
                    if (0x7241 < iVar2) goto LAB_0049213d;
                    uVar4 = 6;
                  }
                }
              }
            }
          }
        }
      }
      else {
LAB_00492060:
        uVar4 = 0;
      }
LAB_00492062:
      *puVar6 = uVar4;
      iVar1 = iVar1 + 1;
      piVar7 = piVar7 + 1;
      puVar6 = puVar6 + 1;
    } while (iVar1 < *(int *)(unaff_EBX + 0x23c));
  }
  iVar1 = 0;
  if (0 < *(int *)(unaff_EBX + 0x234)) {
    puVar5 = (undefined2 *)(param_2 + 0x20);
    do {
      *puVar5 = *(undefined2 *)(param_1 + iVar1 * 4);
      iVar1 = iVar1 + 1;
      puVar5 = puVar5 + 1;
    } while (iVar1 < *(int *)(unaff_EBX + 0x234));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
