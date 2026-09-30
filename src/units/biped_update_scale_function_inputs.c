// biped_update_scale_function_inputs  (Ghidra: no function created; the phase-4 types agent
//   carved a stub "missed_559e40" from the object_type_definition vtable evidence)
// address 0x559e40, size 193 bytes
// name confidence 0.45, rewrite confidence 0.6
// evidence: out/phase4/units_types_notes.md: "The biped row's +0x38 ... columns are 0x559e40,
//   0x559f10 and 0x559f70, and out/functions.json has no function at any of those three
//   addresses -- Ghidra never split them out of biped_update's tail." This is the object_type_
//   definition "biped" row's +0x38 vtable column. It reads Biped.biped_a_in/b_in/c_in/d_in
//   (types/tags.h, BipedFunctionIn_t = int16_t, four consecutive halfwords at +0x30c) and, for
//   each one equal to 1, writes object.function_in_values[i] (objects.h, +0x124: "a_in..d_in,
//   written by the owning object type") with the object's ground speed as a 0..1 fraction of
//   Biped.max_velocity (+0x334) per tick (* 0.033333335 = 1/30s); any other selector value writes
//   0.0 (only selector 1 is implemented in this build -- see UNSURE). Selector 0 leaves the slot
//   untouched, matching every other per-tick "refresh this object's tag-driven scale inputs" style
//   function in this module.
// register convention: object index in the low word of a single register argument (objdump
//   confirms EAX at every call site pattern this module uses for `object_index`, matching
//   biped_apply_idle_fidget's `unaff_EDI`/`param_1` sibling functions in this same address
//   range); blam-cc: object_index only.
// Selectors other than 0 and 1 store 0.0 (disassembly: 0x559e89 loads 0.0, only selector 1 replaces it).
// VERIFIED against disassembly 0x559e40..0x559f00 (2026-09-30); the sum of squares is accumulated i, j, k as in the x87 code.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern double sqrt(double x); // x87 FSQRT

// object_type_definition "biped" row, +0x38 column. Refreshes object.function_in_values[0..3]
// from the biped tag's four scale-function selectors; the only implemented selector (1) drives
// the slot with the object's current ground speed as a fraction of the tag's max_velocity.
void biped_update_scale_function_inputs(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    const int16_t *selector = &tag->biped_a_in;
    float *function_in = obj->function_in_values;
    int32_t i;

    for (i = 4; i != 0; i--) {
        if (*selector != 0) {
            float value = 0.0f;
            if (*selector == 1) {
                value = (float)sqrt((double)(obj->velocity.i * obj->velocity.i +
                                              obj->velocity.j * obj->velocity.j +
                                              obj->velocity.k * obj->velocity.k)) /
                        (tag->max_velocity * 0.033333335f);
                if (value < 0.0f) {
                    value = 0.0f;
                } else if (value > 1.0f) {
                    value = 1.0f;
                }
            }
            *function_in = value;
        }
        selector++;
        function_in++;
    }
}

#if 0
Original Ghidra decompilation (0x559e40):

void missed_559e40(uint param_1)

{
  uint *puVar1;
  int iVar2;
  float fVar3;
  short *psVar4;
  float *pfVar5;
  int iVar6;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  pfVar5 = (float *)(puVar1 + 0x49);
  psVar4 = (short *)(iVar2 + 0x30c);
  iVar6 = 4;
  do {
    if (*psVar4 != 0) {
      fVar3 = 0.0;
      if (*psVar4 == 1) {
        fVar3 = SQRT((float)puVar1[0x1c] * (float)puVar1[0x1c] +
                     (float)puVar1[0x1b] * (float)puVar1[0x1b] +
                     (float)puVar1[0x1a] * (float)puVar1[0x1a]) /
                (*(float *)(iVar2 + 0x334) * 0.033333335);
        if (0.0 <= fVar3) {
          if (1.0 < fVar3) {
            fVar3 = 1.0;
          }
        }
        else {
          fVar3 = 0.0;
        }
      }
      *pfVar5 = fVar3;
    }
    psVar4 = psVar4 + 1;
    pfVar5 = pfVar5 + 1;
    iVar6 = iVar6 + -1;
  } while (iVar6 != 0);
  return;
}
#endif
