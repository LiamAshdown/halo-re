// object_update_functions  (Ghidra: object_update_functions, already named)
// address 0x4f92f0, size 185 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Evaluates all of an object's model-defined periodic/scalar
//   functions each tick and caches their outputs for later use by shaders/effects"; this is
//   also the function the object struct comment cites for writing function_out_values and the
//   function_valid_flags bit)
// rewrite confidence: 0.4
// evidence: types/objects.h object (function_out_values 0x134/obj[0x4d+i],
//   function_valid_flags 0x123, the 0x120+4*selector function_in/out lookup); types/tags.h
//   Object.functions (TagReflexive), ObjectFunction (flags, scale_period_by 0x08,
//   scale_function_by 0x0c, wobble_period 0x10, wobble_magnitude 0x14,
//   square_wave_threshold 0x18, step_count 0x1c, bounds_mode 0x26, bounds 0x28/0x2c,
//   turn_off_with 0x36, scale_by 0x38, inverse_bounds 0x138, inverse_sawtooth 0x13c,
//   inverse_period 0x144); global 0x008603b0 object_data, 0x0087bc14 tag_instances,
//   0x006f1d6c game_time_globals (tick at +0xc).
// register convention: object index in EAX. Consistent with every other single-register
//   accessor in this module and with this function's own Ghidra signature ("in_EAX" only).
//   // blam-cc: EAX -> object_index
// UNSURE: periodic_function_evaluate (0x623cf0-ish, not captured by this batch's pack),
//   FUN_00623e40 and FUN_004ccac0 are foreign/unexamined callees; their signatures are guessed
//   from the single visible argument (or none) at each call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *game_time_globals; // 0x006f1d6c, tick count at +0xc

extern double periodic_function_evaluate(double phase); // UNSURE: address not captured in this batch's pack
extern void FUN_00623e40(double stepped_value); // UNSURE
extern double FUN_004ccac0(void); // UNSURE
extern float FUN_00628cca(void); // 0x628cca, this batch (object_set_position_network.c)

void object_update_functions(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    float phase_base = (float)(int32_t)((object_index & 0xffff) * 0x39 + *(int32_t *)(game_time_globals + 0xc)) * 0.033333335f;
    int32_t i;

    for (i = 0; i < definition->functions.count; i++) {
        ObjectFunction *fn = (ObjectFunction *)((uint8_t *)definition->functions.pointer + i * 0x168);
        float value = fn->inverse_period;
        float result;
        uint8_t enabled = 1;

        if (fn->scale_period_by != 0) {
            float divisor = *(float *)((uint8_t *)obj + 0x120 + fn->scale_period_by * 4);
            if (divisor > 0.0f) {
                value = value / divisor;
            }
        }

        result = (float)periodic_function_evaluate((double)(value * phase_base));

        if (fn->scale_function_by != 0) {
            float scale = *(float *)((uint8_t *)obj + 0x120 + fn->scale_function_by * 4);
            result = scale * result;
        }
        if ((fn->flags & 1) != 0) {
            result = 1.0f - result;
        }
        if (fn->wobble_magnitude != 0.0f) {
            float wobble = (float)periodic_function_evaluate((double)(phase_base * fn->wobble_period));
            wobble = (wobble - 0.5f) * fn->wobble_magnitude;
            result = wobble + wobble + result;
        }
        if ((fn->square_wave_threshold != 0.0f) && (result <= fn->square_wave_threshold)) {
            result = 0.0f;
        } else if (fn->square_wave_threshold != 0.0f) {
            result = 1.0f;
        }
        if (fn->step_count > 1) {
            FUN_00623e40((double)((float)fn->step_count * result));
        }
        if (fn->inverse_sawtooth > 0.0f) {
            FUN_00628cca();
        }

        result = (float)FUN_004ccac0();
        if (fn->scale_by > 0.0f) {
            result = result * fn->scale_by;
        }

        if (fn->bounds_mode == 2) {
            result = (fn->bounds[1] - fn->bounds[0]) * result + fn->bounds[0];
            if (result <= fn->bounds[0] + 0.0001f) {
                enabled = (fn->flags >> 2) & 1;
            }
        } else {
            if (result <= fn->bounds[0] + 0.0001f) {
                result = fn->bounds[0];
                enabled = (fn->flags >> 2) & 1;
            }
            if (fn->bounds[1] < result) {
                result = fn->bounds[1];
            }
            if (fn->bounds_mode == 1) {
                result = (result - fn->bounds[0]) * fn->inverse_bounds;
            }
        }

        if ((fn->turn_off_with != -1) &&
            ((obj->function_valid_flags & (1u << (fn->turn_off_with & 0x1f))) == 0)) {
            enabled = 0;
        }
        if ((fn->flags & 2) != 0) {
            result = FUN_00628cca();
        }

        obj->function_out_values[i] = result;
        if (enabled == 0) {
            obj->function_valid_flags &= (uint8_t)~(1u << (i & 0x1f));
        } else {
            obj->function_valid_flags |= (uint8_t)(1u << (i & 0x1f));
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
