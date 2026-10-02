// camera_input_axes_update  (Ghidra: FUN_00446170, plus its mis-split tail 0x4462a0
//   director_load_camera; renamed)
// address 0x446170, size 474 bytes (0x446170..0x44634a; Ghidra splits it at 0x4462a0, which is
//   entered only through the loop back-edge `jne 0x4461f0` and the fall-through)
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: multiplies director.look_scale by pow(1.3, zoom) and clamps it to 0.01..50, then
//   runs the four camera_input_axis_definition rows (.data 0x00686a28, stride 0x1c) against the
//   four director.axes states (+0xc8, stride 0xc): exponential damping of the velocity by
//   (1 - clamp(dt * 5, 0, 1)), a push of dt * acceleration * scale * 25 when exactly one of
//   the decrease / increase keys is held, delta = dt * velocity, value += delta (or reset),
//   clamp to minimum..maximum. Constants: 1.3 (double 0x00672c60), 0.01 / 50 (0x00672c5c /
//   0x00672c58), 5.0 (0x00672c40), 25.0 (0x0065727c).
// register convention (objdump 0x446185 movsx edi,ax; 0x44617f ebp = [esp+0x14]; caller
//   0x4460ef..0x4460f8): local player index in AX, then key bits and zoom on the stack.
//   // blam-cc: AX -> local_player_index, stack -> (key_bits, zoom)
// Faithful oddity: the scale_by_zoom byte is loaded from the absolute address 0x00686a40
//   (row 0) on every iteration, so row 0 decides for all four axes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern director_globals camera_director_globals;          // 0x006ac558
extern director directors[1];                             // 0x006ac560
extern camera_input_axis_definition camera_input_axes[4]; // 0x00686a28

extern double pow(double base, double exponent); // 0x6283c0, MSVC 7.1 CRT _CIpow (x87 operands)

static uint8_t camera_input_key_held(uint32_t key_bits, int16_t bit)
{
    return bit != -1 && (key_bits & (1u << (bit & 0x1f))) != 0;
}

// blam-cc: AX -> local_player_index, stack -> (key_bits, zoom)
void camera_input_axes_update(int16_t local_player_index, uint32_t key_bits, float zoom)
{
    director *director = &directors[local_player_index];
    camera_input_axis_definition *definition = camera_input_axes;
    camera_input_axis_state *state = director->axes;
    float look_scale;
    int32_t count;

    look_scale = (float)(pow((double)1.3f, (double)zoom) * director->look_scale);
    director->look_scale = look_scale;
    if (look_scale < 0.01f) {
        look_scale = 0.01f;
    } else if (look_scale > 50.0f) {
        look_scale = 50.0f;
    }
    director->look_scale = look_scale;

    for (count = 4; count != 0; count--, definition++, state++) {
        float scale = camera_input_axes[0].scale_by_zoom ? director->look_scale : 1.0f;
        float damping = camera_director_globals.dt * 5.0f;
        uint8_t decrease;
        uint8_t increase;
        uint8_t reset;
        float velocity;
        float value;

        if (damping < 0.0f) {
            damping = 0.0f;
        } else if (damping > 1.0f) {
            damping = 1.0f;
        }
        decrease = camera_input_key_held(key_bits, definition->decrease_key_bit);
        increase = camera_input_key_held(key_bits, definition->increase_key_bit);
        reset = camera_input_key_held(key_bits, definition->reset_key_bit);

        velocity = (1.0f - damping) * state->velocity;
        state->velocity = velocity;
        if (decrease) {
            if (!increase) {
                state->velocity = velocity -
                    camera_director_globals.dt * definition->acceleration * scale * 25.0f;
            }
        } else if (increase) {
            state->velocity = camera_director_globals.dt * definition->acceleration * scale * 25.0f +
                velocity;
        }

        state->delta = camera_director_globals.dt * state->velocity;
        if (reset) {
            state->value = definition->reset_value;
        } else {
            state->value = state->delta + state->value;
        }

        value = state->value;
        if (value < definition->minimum_value) {
            value = definition->minimum_value;
        } else if (value > definition->maximum_value) {
            value = definition->maximum_value;
        }
        state->value = value;
    }
}

#if 0
Original Ghidra decompilation (0x446170):

void FUN_00446170(uint param_1)

{
  float fVar1;
  float fVar2;
  bool bVar3;
  bool bVar4;
  bool bVar5;
  short in_AX;
  float *pfVar6;
  short *psVar7;
  int iVar8;
  float10 fVar9;
  int local_4;

  iVar8 = in_AX * 0xf8;
  fVar9 = (float10)FUN_006283c0();
  fVar9 = fVar9 * (float10)*(float *)(&DAT_006ac624 + iVar8);
  *(float *)(&DAT_006ac624 + iVar8) = (float)fVar9;
  if ((float10)0.01 <= fVar9) {
    if ((float10)50.0 < fVar9) {
      fVar9 = (float10)50.0;
    }
  }
  else {
    fVar9 = (float10)0.01;
  }
  *(float *)(&DAT_006ac624 + iVar8) = (float)fVar9;
  psVar7 = &DAT_00686a2c;
  pfVar6 = (float *)(&DAT_006ac628 + in_AX * 0x3e);
  local_4 = 4;
  do {
    if (DAT_00686a40 == '\0') {
      fVar1 = 1.0;
    }
    else {
      fVar1 = *(float *)(&DAT_006ac624 + iVar8);
    }
    fVar2 = DAT_006ac558 * 5.0;
    if (0.0 <= fVar2) {
      if (1.0 < fVar2) {
        fVar2 = 1.0;
      }
    }
    else {
      fVar2 = 0.0;
    }
    if ((psVar7[-2] == -1) || (bVar3 = true, (param_1 & 1 << ((byte)psVar7[-2] & 0x1f)) == 0)) {
      bVar3 = false;
    }
    if ((psVar7[-1] == -1) || ((param_1 & 1 << ((byte)psVar7[-1] & 0x1f)) == 0)) {
      bVar4 = false;
    }
    else {
      bVar4 = true;
    }
    if ((*psVar7 == -1) || ((param_1 & 1 << ((byte)*psVar7 & 0x1f)) == 0)) {
      bVar5 = false;
    }
    else {
      bVar5 = true;
    }
    fVar2 = (1.0 - fVar2) * pfVar6[1];
    pfVar6[1] = fVar2;
    if (bVar3) {
      if (!bVar4) {
        fVar2 = fVar2 - DAT_006ac558 * *(float *)(psVar7 + 2) * fVar1 * 25.0;
LAB_004462df:
        pfVar6[1] = fVar2;
      }
    }
    else if (bVar4) {
      fVar2 = DAT_006ac558 * *(float *)(psVar7 + 2) * fVar1 * 25.0 + fVar2;
      goto LAB_004462df;
    }
    fVar1 = DAT_006ac558 * pfVar6[1];
    pfVar6[2] = fVar1;
    if (bVar5) {
      *pfVar6 = *(float *)(psVar7 + 4);
    }
    else {
      *pfVar6 = fVar1 + *pfVar6;
    }
    if (*(float *)(psVar7 + 6) <= *pfVar6) {
      if (*pfVar6 <= *(float *)(psVar7 + 8)) {
        fVar1 = *pfVar6;
      }
      else {
        fVar1 = *(float *)(psVar7 + 8);
      }
    }
    else {
      fVar1 = *(float *)(psVar7 + 6);
    }
    *pfVar6 = fVar1;
    psVar7 = psVar7 + 0xe;
    pfVar6 = pfVar6 + 3;
    local_4 = local_4 + -1;
    if (local_4 == 0) {
      return;
    }
  } while( true );
}

The split tail 0x4462a0 (Ghidra director_load_camera) is the same loop body from the key tests
onwards and adds nothing; it is not reproduced. objdump for the pow operands Ghidra dropped:
  446170: fld QWORD PTR ds:0x672c60   ; 1.3
  446179: fld DWORD PTR [esp+0x10]    ; zoom
  446194: call 0x6283c0               ; _CIpow(st1, st0)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
