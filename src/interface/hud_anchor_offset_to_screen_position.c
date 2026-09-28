// hud_anchor_offset_to_screen_position  (Ghidra: FUN_004ab690, renamed)
// address 0x4ab690, size 315 bytes
// name confidence: 0.35 (chosen)   rewrite confidence: 0.85 (REWRITTEN: child-placement jump table (0x4ab8b8) decoded and implemented; the ECX = 0 path verified against 0x4ab690 (all callers pass 0))
// evidence: phase-4 summary "Converts a HUD element's anchor-relative offset into an absolute
// 640x480 screen position"; the four anchor-type constants (8, 0x278, 8, 0x1d8+8=0x1e0) bracket
// exactly a 640x480 canvas (0x278-8=0x270=624, leaving 8px margins on each side; 0x1e0-8=0x1d8=472
// likewise for 480), matching types/tags.h's HUDInterfaceCanvasSize/anchor conventions.
// UNSURE: for anchor values 4 and up, Ghidra could not recover the jump table used when the
// caller's ECX selector is nonzero ("Could not recover jumptable... too many branches"); it is
// transcribed as an opaque function-pointer table indexed by the anchor value, matching
// Ghidra's own fallback, and none of its targets are analyzed. DAT_007c3140 (a packed pair of
// int16 offsets used for anchor values >= 4) is not documented anywhere in this pass.
// register convention: anchor pointer in ECX->... actually the anchor+offset input is ECX (a
// selector, see below), scale in AL/param_2 (recognized as the second parameter), offset pair
// in EDX (in_EDX), output pointer in the recognized third parameter; anchor value itself is
// read through the recognized first parameter.
//   // blam-cc: anchor -> recognized param 1, scale_flag/scale -> AL + recognized param 2,
//   //          offset -> EDX, out -> recognized param 3, selector -> ECX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"


extern int32_t ROUND(float x); // MSVC round-to-nearest helper
extern uint32_t render_viewport_top; // 0x007c3140, UNSURE name: packed {int16 x, int16 y}


// blam-cc: see header
// Converts a HUD element's anchor + pixel offset into an absolute 640x480-canvas screen
// position (anchor 0..3, one per corner) or a camera-viewport-relative position (anchor >= 4,
// via an unrecovered per-anchor handler when selector is nonzero).
void hud_anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale,
                                           const int16_t *offset, int16_t *out, int32_t selector)
{
    float x, y;
    uint16_t anchor_value = *anchor;

    if (!has_scale || scale == 0.0f) {
        scale = 1.0f;
    }

    if ((int16_t)anchor_value < 4) {
        x = (float)((((anchor_value & 1) == 0) ? 1 : -1) * (int32_t)offset[0]) * scale +
            (float)((((anchor_value & 1) != 0) ? 0x270 : 0) + 8);
        y = (float)((((anchor_value & 2) == 0) ? 1 : -1) * (int32_t)offset[1]) * scale +
            (float)((((anchor_value & 2) != 0) ? 0x1d8 : 0) + 8);
    } else {
        int16_t offset_x = (int16_t)(render_viewport_top >> 16);
        int16_t offset_y = (int16_t)render_viewport_top;
        x = (float)(int32_t)offset[0] * scale + (float)(0x140 - offset_x);
        y = (float)(int32_t)offset[1] * scale + (float)(0xf0 - offset_y);
    }

    // REWRITTEN (0x4ab78e..0x4ab8b0, jump table 0x4ab8b8): a nonzero ECX is a child placement
    // record (+0x04 width, +0x06 height, +0x10 / +0x12 offset) added in the anchor's direction;
    // anchor 4 (centre) adds half of +0x04 to BOTH axes, as the binary does. Every caller in
    // this build passes ECX = 0.
    if (selector != 0) {
        const int16_t *child = (const int16_t *)(uint32_t)selector;
        int32_t dx = child[8];  // +0x10
        int32_t dy = child[9];  // +0x12

        switch ((int16_t)anchor_value) {
        case 0:
            break;
        case 1:
            dx -= child[2];     // +0x04
            break;
        case 2:
            dy -= child[3];     // +0x06
            break;
        case 3:
            dx -= child[2];
            dy -= child[3];
            break;
        case 4: {
            int32_t half = (int32_t)child[2] / 2;
            dx += half;
            dy += half;
            break;
        }
        default:
            goto store; // UNSURE: the binary's table has no bounds check
        }
        x = (float)dx * scale + x;
        y = (float)dy * scale + y;
    }
store:
    out[0] = (int16_t)(int32_t)ROUND(x);
    out[1] = (int16_t)(int32_t)ROUND(y);
}

#if 0
Original Ghidra decompilation (0x4ab690):

void FUN_004ab690(ushort *param_1,float param_2,undefined2 *param_3)

{
  ushort uVar1;
  float fVar2;
  char in_AL;
  int in_ECX;
  short *in_EDX;
  float local_8;

  if ((in_AL == '\0') || (param_2 == 0.0)) {
    param_2 = 1.0;
  }
  uVar1 = *param_1;
  if ((short)uVar1 < 4) {
    local_8 = (float)(int)(((uint)((uVar1 & 1) == 0) * 2 + -1) * (int)*in_EDX) * param_2 +
              (float)((-(uint)((uVar1 & 1) != 0) & 0x270) + 8);
    fVar2 = (float)(int)(((uint)((uVar1 & 2) == 0) * 2 + -1) * (int)in_EDX[1]) * param_2 +
            (float)((-(uint)((uVar1 & 2) != 0) & 0x1d8) + 8);
  }
  else {
    local_8 = (float)(int)*in_EDX * param_2 + (float)(0x140 - DAT_007c3140._2_2_);
    fVar2 = (float)(int)in_EDX[1] * param_2 + (float)(0xf0 - (short)DAT_007c3140);
  }
  if (in_ECX == 0) {
    param_1._0_2_ = (undefined2)(int)ROUND(local_8);
    *param_3 = param_1._0_2_;
    param_1._0_2_ = (undefined2)(int)ROUND(fVar2);
    param_3[1] = param_1._0_2_;
    return;
  }
                    /* WARNING: Could not recover jumptable at 0x004ab799. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*(code *)(&PTR_LAB_004ab8b8)[(short)uVar1])();
  return;
}
#endif
