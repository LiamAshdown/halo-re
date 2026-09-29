// unit_update_scale_function_inputs  (Ghidra: no function created; the phase-4 types agent
//   carved a placeholder "missed_563860" from the object_type_definition vtable evidence)
// address 0x563860, size 362 bytes
// name confidence 0.45, rewrite confidence 0.55 (see the UNSURE note on case 7)
// evidence: out/phase4/units_types_notes.md: "The unit row's other columns are ... 0x563860
//   (+0x38)". Same shape as biped_update_scale_function_inputs.c (0x559e40, the biped row's own
//   +0x38 column): reads Unit.unit_a_in/b_in/c_in/d_in (types/tags.h, UnitFunctionIn_t =
//   int16_t, four consecutive halfwords at tag+0x198) and, for each nonzero selector, writes
//   object.function_in_values[i] (objects.h +0x124). Every field access below was checked
//   against objdump 0x563860..0x5639d4: cases 1/2/4/5 are plain `fld` float loads (Ghidra's
//   Ghidra's `(float)puVarN[k]` display for these is a decompiler artifact of the pointer's
//   inferred `uint *` type, not a real int-to-float conversion); case 3 is a genuine `fild`
//   int-to-float conversion of an 8-bit field times 0x672ad4 (0.003921569 = 1/255); the four
//   `.rdata` constants (0x672ac0 = 0.0f, 0x672ac4 = 1.0f, 0x672ad4 = 1/255, 0x672e08 =
//   0.011111111 = 1/90) were read directly out of the image.
// register convention: object index in a single register argument (matches every other biped_*/
//   unit_* per-object helper in this address range); blam-cc: object_index only.
// UNSURE: case 7 walks the unit's Object.animation_graph tag (objects.h +0xcc, a tag reference
//   reused as a tag_instances index the same way case 0x559e40's tag lookup works) into what
//   objdump proves is that ModelAnimations tag's `animations` TagReflexive (types/tags.h
//   ModelAnimations, struct size 0x80: the reflexive occupying the last 0xc bytes, so its
//   `.pointer` sub-field lands exactly on the `[ebx+0x78]` read here), indexes it by
//   Object.animation_index (+0xd0) at a 0xb4-byte stride, and reads a `+0x2e` int16 field from
//   the indexed ModelAnimationsAnimation record -- almost certainly that animation's frame count,
//   compared against the current animation_index to compute a playback-progress ratio (or, once
//   playback has run past the frame count, `1.0 - unit_data.unknown_20e / 90`). Neither
//   ModelAnimationsAnimation nor its +0x2e field is defined in types/tags.h, so this is written
//   with raw offsets and left for a models/cache-module types pass to name properly.
// Cleanup-pass review (objdump 0x563860..0x5639cf, jump table 0x5639d4): case 3 reads
//   unit+0x323 zero-extended (movzx at 0x5638f0); the draft sign-extended the int8_t field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// object_type_definition "unit" row, +0x38 column. Refreshes object.function_in_values[0..3]
// from the unit tag's four scale-function selectors.
void unit_update_scale_function_inputs(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    const int16_t *selector = &tag->unit_a_in;
    float *function_in = obj->function_in_values;
    int32_t i;

    for (i = 4; i != 0; i--) {
        if (*selector != 0) {
            float value = 0.0f;
            switch (*selector) {
            case 1:
                value = unit->driver_seat_power;
                break;
            case 2:
                value = unit->gunner_seat_power;
                break;
            case 3:
                value = (float)(int32_t)(uint8_t)unit->aiming_change * 0.003921569f; // movzx: unsigned byte / 255
                break;
            case 4:
                value = unit->animation_blend_weight;
                break;
            case 5:
                value = unit->integrated_light_power;
                break;
            case 6:
                if ((obj->vitality_flags & 4) == 0 && (unit->flags & 0x400000) == 0) { // UNSURE: 0x400000 not in unit_flags
                    value = 1.0f;
                } else {
                    value = 0.0f;
                }
                break;
            case 7:
            {
                // UNSURE: raw ModelAnimations.animations[] walk; see the header note above
                tag_instance *graph = &tag_instances[obj->animation_graph & 0xffff];
                uint8_t *animations_pointer = *(uint8_t **)((uint8_t *)graph->data + 0x78);
                int16_t frame_count = *(int16_t *)(animations_pointer + (int32_t)obj->animation_index * 0xb4 + 0x2e);
                if (obj->animation_index < frame_count) {
                    value = (float)(int32_t)obj->animation_index / (float)(int32_t)frame_count;
                } else {
                    value = 1.0f - (float)(int32_t)unit->unknown_20e * 0.011111111f;
                }
                break;
            }
            }
            *function_in = value;
        }
        selector++;
        function_in++;
    }
}

#if 0
Original Ghidra decompilation (0x563860):

void missed_563860(uint param_1)

{
  float fVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  short *psVar5;
  float *pfVar6;

  iVar4 = DAT_0087bc14;
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  pfVar6 = (float *)(puVar3 + 0x49);
  psVar5 = (short *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x198);
  param_1 = 4;
  do {
    if (*psVar5 != 0) {
      fVar1 = 0.0;
      switch(*psVar5) {
      case 1:
        fVar1 = (float)puVar3[0xce];
        break;
      case 2:
        fVar1 = (float)puVar3[0xcf];
        break;
      case 3:
        fVar1 = (float)*(byte *)((int)puVar3 + 0x323) * 0.003921569;
        break;
      case 4:
        fVar1 = (float)puVar3[0xba];
        break;
      case 5:
        fVar1 = (float)puVar3[0xd0];
        break;
      case 6:
        if (((*(byte *)((int)puVar3 + 0x106) & 4) == 0) && ((puVar3[0x81] & 0x400000) == 0)) {
          fVar1 = 1.0;
        }
        else {
          fVar1 = 0.0;
        }
        break;
      case 7:
        sVar2 = *(short *)((short)puVar3[0x34] * 0xb4 +
                           *(int *)(*(int *)((puVar3[0x33] & 0xffff) * 0x20 + 0x14 + iVar4) + 0x78)
                          + 0x2e);
        if ((short)puVar3[0x34] < sVar2) {
          fVar1 = (float)(int)(short)puVar3[0x34] / (float)(int)sVar2;
        }
        else {
          fVar1 = 1.0 - (float)(int)*(char *)((int)puVar3 + 0x20e) * 0.011111111;
        }
      }
      *pfVar6 = fVar1;
    }
    psVar5 = psVar5 + 1;
    pfVar6 = pfVar6 + 1;
    param_1 = param_1 + -1;
  } while (param_1 != 0);
  return;
}
#endif
