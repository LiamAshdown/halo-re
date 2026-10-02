// object_update_functions  (Ghidra: object_update_functions, already named)
// address 0x4f92f0, size 929 bytes (0x4f92f0..0x4f9690; 0x4f93b0 / 0x4f94e0 / 0x4f9540 are its fragments)
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Evaluates all of an object's model-defined periodic/scalar
//   functions each tick and caches their outputs for later use by shaders/effects"; this is
//   also the function the object struct comment cites for writing function_out_values and the
//   function_valid_flags bit)
// rewrite confidence: 0.85
// evidence: types/objects.h object (function_out_values 0x134/obj[0x4d+i],
//   function_valid_flags 0x123, the 0x120+4*selector function_in/out lookup); types/tags.h
//   Object.functions (TagReflexive), ObjectFunction (flags, scale_period_by 0x08,
//   scale_function_by 0x0c, wobble_period 0x10, wobble_magnitude 0x14,
//   square_wave_threshold 0x18, step_count 0x1c, bounds_mode 0x26, bounds 0x28/0x2c,
//   turn_off_with 0x36, scale_by 0x38, inverse_bounds 0x138, inverse_sawtooth 0x13c,
//   inverse_period 0x144); global 0x008603b0 object_data, 0x0087bc14 tag_instances,
//   0x006f1d6c game_time (tick at +0xc).
// register convention: object index in EAX. Consistent with every other single-register
//   accessor in this module and with this function's own Ghidra signature ("in_EAX" only).
//   // blam-cc: EAX -> object_index
// REWRITTEN (from objdump 0x4f92f0..0x4f9690; size is 0x3a1 bytes, the header's 185 was Ghidra's). Per function i
//   (Object +0x158 count, +0x15c block, 0x168 each), starting from inverse_period (+0x144):
//   - divided by the scale_period_by input (object +0x120 + 4*selector) when that is > 0, times the phase
//     (object_index*0x39 + game_time) * 1/30, through periodic_function_evaluate(AX = function);
//   - times the scale_function_by input; 1 - value when flags bit 0;
//   - with a wobble magnitude: + 2 * (periodic(wobble_function, phase * wobble_period) - 0.5) * magnitude;
//   - with a square wave threshold: 1 above it, else 0;
//   - step_count > 1: floor(step_count * value) * inverse_step (+0x140);
//   - inverse_sawtooth (+0x13c) > 0: fmod(value, inverse_sawtooth);
//   - + the add input, capped at 1; times the scale_result_by input;
//   - transition_function_evaluate(CX = map_to, value); times scale_by when > 0;
//   - bounds mode 2 maps 0..1 onto the bounds; otherwise clamps to them and mode 1 renormalizes by inverse_bounds;
//     at or under bounds[0] + 0.0001 the function is valid only if flags bit 2;
//   - turned off when the turn_off_with function (+0x36, not -1) is not valid;
//   - flags bit 1: fmod(value + previous output, 1.0);
//   and the output (object +0x134 + 4*i) and valid bit (object +0x123) are written. The draft called
//   transition_function_evaluate with no arguments (the crash), dropped floor's result and misused fmod.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern real periodic_function_evaluate(periodic_function_t type, double time);
    // 0x4cc9b0, blam-cc: AX -> type, stack -> time
extern real transition_function_evaluate(transition_function_t type, real phase);
    // 0x4ccac0, blam-cc: CX -> type, stack -> phase
extern double floor(double x);          // 0x623e40, MSVC CRT
extern double fmod(double x, double y); // 0x628cca, MSVC CRT _CIfmod: x in ST(1), y in ST(0)

static float function_scale_input(uint8_t *obj, int16_t selector)
{
    return *(float *)(obj + 0x120 + selector * 4);
}

void object_update_functions(uint32_t object_index) // blam-cc: EAX -> object_index
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[*(datum_index *)obj & 0xffff].data;
    float phase = (float)(int32_t)((object_index & 0xffff) * 0x39 + game_time->game_time) * 0.033333335f;
    int16_t i;

    for (i = 0; i < (int32_t)definition->functions.count; i++) {
        ObjectFunction *fn = (ObjectFunction *)((uint8_t *)definition->functions.pointer + i * 0x168);
        float period = fn->inverse_period;
        float value;
        uint8_t valid = 1;

        if (fn->scale_period_by != 0) {
            float scale = function_scale_input(obj, fn->scale_period_by);
            if (scale > 0.0f) {
                period = period / scale;
            }
        }
        value = periodic_function_evaluate(fn->function, (double)(period * phase));
        if (fn->scale_function_by != 0) {
            value = function_scale_input(obj, fn->scale_function_by) * value;
        }
        if (fn->flags & 1) {
            value = 1.0f - value;
        }
        if (fn->wobble_magnitude != 0.0f) {
            float wobble = periodic_function_evaluate(fn->wobble_function, (double)(phase * fn->wobble_period));
            wobble = (wobble - 0.5f) * fn->wobble_magnitude;
            value = wobble + wobble + value;
        }
        if (fn->square_wave_threshold != 0.0f) {
            value = value > fn->square_wave_threshold ? 1.0f : 0.0f;
        }
        if (fn->step_count > 1) {
            value = (float)(floor((double)((float)fn->step_count * value)) * fn->inverse_step);
        }
        if (fn->inverse_sawtooth > 0.0f) {
            value = (float)fmod((double)value, (double)fn->inverse_sawtooth);
        }
        if (fn->add != 0) {
            value = function_scale_input(obj, fn->add) + value;
            if (value > 1.0f) {
                value = 1.0f;
            }
        }
        if (fn->scale_result_by != 0) {
            value = function_scale_input(obj, fn->scale_result_by) * value;
        }
        value = transition_function_evaluate(fn->map_to, value);
        if (fn->scale_by > 0.0f) {
            value = value * fn->scale_by;
        }
        if (fn->bounds_mode == 2) {
            value = (fn->bounds[1] - fn->bounds[0]) * value + fn->bounds[0];
            if (fn->bounds[0] + 0.0001f >= value) { // fcomp/test ah,1: NaN leaves it alone
                valid = (uint8_t)((fn->flags >> 2) & 1);
            }
        } else {
            if (fn->bounds[0] + 0.0001f >= value) { // fcomp/test ah,1: NaN leaves it alone
                valid = (uint8_t)((fn->flags >> 2) & 1);
                value = fn->bounds[0];
            }
            if (value > fn->bounds[1]) { // fcomp/test ah,0x41
                value = fn->bounds[1];
            }
            if (fn->bounds_mode == 1) {
                value = (value - fn->bounds[0]) * fn->inverse_bounds;
            }
        }
        if (fn->turn_off_with != -1 &&
            (obj[0x123] & (uint8_t)(1u << (fn->turn_off_with & 0x1f))) == 0) {
            valid = 0;
        }
        if (fn->flags & 2) {
            value = (float)fmod((double)(value + *(float *)(obj + 0x134 + i * 4)), 1.0);
        }
        *(float *)(obj + 0x134 + i * 4) = value;
        if (valid) {
            obj[0x123] = (uint8_t)(obj[0x123] | (1u << i));
        } else {
            obj[0x123] = (uint8_t)(obj[0x123] & ~(1u << i));
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f92f0):

void object_update_functions(void)

{
  float fVar1;
  float fVar2;
  short sVar3;
  uint *puVar4;
  int iVar5;
  int iVar6;
  float fVar7;
  short sVar8;
  uint in_EAX;
  byte bVar9;
  int iVar10;
  int iVar11;
  uint *puVar12;
  float10 fVar13;
  float fStack_14;

  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar5 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar10 = 0;
  sVar8 = 0;
  fVar7 = (float)(int)((in_EAX & 0xffff) * 0x39 + *(int *)(DAT_006f1d6c + 0xc)) * 0.033333335;
  if (0 < *(int *)(iVar5 + 0x158)) {
    do {
      iVar6 = *(int *)(iVar5 + 0x15c);
      iVar11 = iVar10 * 0x168;
      fVar1 = *(float *)(iVar11 + 0x144 + iVar6);
      sVar3 = *(short *)(iVar11 + 8 + iVar6);
      puVar12 = (uint *)(iVar11 + iVar6);
      bVar9 = 1;
      if (sVar3 != 0) {
        if (sVar3 < 5) {
          fVar2 = (float)puVar4[sVar3 + 0x48];
        }
        else {
          fVar2 = (float)puVar4[sVar3 + 0x48];
        }
        if (0.0 < fVar2) {
          fVar1 = fVar1 / fVar2;
        }
      }
      fVar13 = (float10)periodic_function_evaluate((double)(fVar1 * fVar7));
      sVar3 = (short)puVar12[3];
      fStack_14 = (float)fVar13;
      if (sVar3 != 0) {
        if (sVar3 < 5) {
          fVar1 = (float)puVar4[sVar3 + 0x48];
        }
        else {
          fVar1 = (float)puVar4[sVar3 + 0x48];
        }
        fStack_14 = fVar1 * fStack_14;
      }
      if ((*puVar12 & 1) != 0) {
        fStack_14 = 1.0 - fStack_14;
      }
      if ((float)puVar12[5] != 0.0) {
        fVar13 = (float10)periodic_function_evaluate((double)(fVar7 * (float)puVar12[4]));
        fVar13 = (fVar13 - (float10)0.5) * (float10)(float)puVar12[5];
        fStack_14 = (float)(fVar13 + fVar13 + (float10)fStack_14);
      }
      fVar1 = fStack_14;
      if (((float)puVar12[6] != 0.0) && (fStack_14 = 1.0, fVar1 <= (float)puVar12[6])) {
        fStack_14 = 0.0;
      }
      if (1 < (short)puVar12[7]) {
        FUN_00623e40((double)((float)(int)(short)puVar12[7] * fStack_14));
      }
      if (0.0 < (float)puVar12[0x4f]) {
        FUN_00628cca();
      }
      fVar13 = (float10)FUN_004ccac0();
      fStack_14 = (float)fVar13;
      if (0.0 < (float)puVar12[0xe]) {
        fStack_14 = fStack_14 * (float)puVar12[0xe];
      }
      if (*(short *)((int)puVar12 + 0x26) == 2) {
        fStack_14 = ((float)puVar12[0xb] - (float)puVar12[10]) * fStack_14 + (float)puVar12[10];
        if (fStack_14 <= (float)puVar12[10] + 0.0001) {
          bVar9 = (byte)(*puVar12 >> 2) & 1;
        }
      }
      else {
        if (fStack_14 <= (float)puVar12[10] + 0.0001) {
          fStack_14 = (float)puVar12[10];
          bVar9 = (byte)(*puVar12 >> 2) & 1;
        }
        if ((float)puVar12[0xb] < fStack_14) {
          fStack_14 = (float)puVar12[0xb];
        }
        if (*(short *)((int)puVar12 + 0x26) == 1) {
          fStack_14 = (fStack_14 - (float)puVar12[10]) * (float)puVar12[0x4e];
        }
      }
      if ((*(short *)((int)puVar12 + 0x36) != -1) &&
         ((*(byte *)((int)puVar4 + 0x123) &
          (byte)(1 << ((byte)*(short *)((int)puVar12 + 0x36) & 0x1f))) == 0)) {
        bVar9 = 0;
      }
      if ((*puVar12 & 2) != 0) {
        fVar13 = (float10)FUN_00628cca();
        fStack_14 = (float)fVar13;
      }
      puVar4[iVar10 + 0x4d] = (uint)fStack_14;
      if (bVar9 == 0) {
        *(byte *)((int)puVar4 + 0x123) =
             *(byte *)((int)puVar4 + 0x123) & ~('\x01' << ((byte)iVar10 & 0x1f));
      }
      else {
        *(byte *)((int)puVar4 + 0x123) =
             *(byte *)((int)puVar4 + 0x123) | '\x01' << ((byte)iVar10 & 0x1f);
      }
      sVar8 = sVar8 + 1;
      iVar10 = (int)sVar8;
    } while (iVar10 < *(int *)(iVar5 + 0x158));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
