// object_sample_ambient_lighting
// address 0x4f20b0, size 892 bytes
// name confidence: 0.9 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Builds an averaged ambient-lighting sample for an object by
//   probing its center and several offset points around its bounding volume")
// rewrite confidence: 0.75
// evidence: types/objects.h object (bounding_center at 0x0a0, bounding_radius at 0x0ac);
//   types/tags.h Object.flags bit 2 ("brighter_than_it_should_be"); global 0x0087bc14
//   tag_instances; callee object_lighting_sample_point (0x4f2550, outside this batch's address
//   range but already named). The 29-float output block param_1 fills has no established
//   struct anywhere in types/objects.h (it belongs to whatever rendering-side "ambient sample"
//   type consumes it), so it is kept as a flat float array indexed exactly the way the original
//   indexes it, rather than inventing field names.
// IMPORTANT: the accumulate and average passes do NOT cover all 29 floats. The original
//   touches indices 0..2, 4..0x0f and 0x13..0x1c only; index 3 holds the int16 status word
//   written as *(int16_t *)(sample + 3) = 2, and 0x10..0x12 are left alone. An earlier draft
//   of this file looped 0..28 over both passes, which corrupted those four slots.
// resolved from disassembly (previously UNSURE): the three interleaved
//   vector3d_normalize_with_length calls take their vector in ECX, and the disassembly loads
//   it immediately before each one --
//     0x4f235d..0x4f2375  ecx = sample + 0x1c  -> normalizes sample[7..9]
//     0x4f237c..0x4f23b9  lea ecx,[ebp+0x34]   -> normalizes sample[0x0d..0x0f]
//     0x4f23c0..0x4f23fd  lea ecx,[ebp+0x5c]   -> normalizes sample[0x17..0x19]
//   (EBP is the sample pointer here; this function keeps no frame pointer.) So the 29-float
//   block carries three unit direction vectors at 7, 0x0d and 0x17.
// UNSURE: the two object.flags bits tested here (0x8000 via a signed-byte-of-the-high-byte
//   test, and 0x4000) are not in types/objects.h's object_flags enum; kept as raw bit tests.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "rasterizer.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t object_lighting_sample_point(uint8_t flags, real_point3d *point, render_lighting *lighting); // 0x4f2550, cdecl;
    // `sample` below is that 0x74-byte render_lighting (types/rasterizer.h) walked as 29 floats
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX

// True for the float slots the accumulate/average passes touch. Index 3 is the int16 status
// word and 0x10..0x12 are never written by this function.
static int object_ambient_sample_slot_is_averaged(int index)
{
    return index != 3 && (index < 0x10 || index > 0x12);
}

// FIXED (register inputs, objdump + difftest): the original never reads ECX; sample arrive(s) on the stack (1 stack argument(s)).
// blam-cc: EAX -> object_index, stack -> sample
void object_sample_ambient_lighting(uint32_t object_index, float *sample) // blam-cc: EAX -> object_index, ECX -> sample
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t flags = (int8_t)(obj->flags >> 8) < 0 ? 1 : 0; // UNSURE: see file header (bit 0x8000)
    char center_ok;
    int16_t successes;
    uint16_t offset_index;
    int i;

    if ((*((uint8_t *)object_tag + 2) & 4) != 0) {
        flags |= 4;
    }

    center_ok = object_lighting_sample_point(flags, &obj->bounding_center, (render_lighting *)sample);

    if ((obj->flags & 0x4000) == 0) { // UNSURE: see file header
        float probe[29];

        if (center_ok == 0) {
            for (i = 0; i < 29; i++) {
                sample[i] = 0.0f;
            }
            *(int16_t *)(sample + 3) = 2;
            successes = 0;
        } else {
            successes = 1;
        }

        for (offset_index = 0; (int16_t)offset_index < 4; offset_index++) {
            real_point3d corner;
            char ok;

            corner.x = ((offset_index & 1) == 0 ? -0.70710677f : 0.70710677f) * obj->bounding_radius +
                       obj->bounding_center.x;
            corner.z = obj->bounding_center.z;
            corner.y = ((offset_index & 2) == 0 ? -0.70710677f : 0.70710677f) * obj->bounding_radius +
                       obj->bounding_center.y;

            ok = object_lighting_sample_point(flags, &corner, (render_lighting *)probe);
            if (ok != 0) {
                successes = successes + 1;
                for (i = 0; i < 29; i++) {
                    if (object_ambient_sample_slot_is_averaged(i)) {
                        sample[i] += probe[i];
                    }
                }
            }
        }

        if (successes > 1) {
            float scale = 1.0f / (float)(int)successes;

            // The original interleaves the scaling with the three renormalizations in exactly
            // this order; the groups are disjoint, so the order is preserved for fidelity.
            sample[0] *= scale; sample[1] *= scale; sample[2] *= scale;
            for (i = 0x13; i <= 0x16; i++) sample[i] *= scale;
            for (i = 4; i <= 9; i++) sample[i] *= scale;
            vector3d_normalize_with_length((real_vector3d *)(sample + 7));

            for (i = 0x0a; i <= 0x0f; i++) sample[i] *= scale;
            vector3d_normalize_with_length((real_vector3d *)(sample + 0x0d));

            for (i = 0x1a; i <= 0x1c; i++) sample[i] *= scale;
            for (i = 0x17; i <= 0x19; i++) sample[i] *= scale;
            vector3d_normalize_with_length((real_vector3d *)(sample + 0x17));
            return;
        }

        if (successes == 0) {
            // Copies the last probe buffer wholesale, contents undefined when no probe hit.
            for (i = 0; i < 29; i++) {
                sample[i] = probe[i];
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f20b0):

void object_sample_ambient_lighting(float *param_1)

{
  uint *puVar1;
  float fVar2;
  char cVar3;
  uint in_EAX;
  int iVar4;
  ushort uVar5;
  float *pfVar6;
  short sVar7;
  byte bVar8;
  float local_80;
  float local_7c;
  uint local_78;
  float local_74 [4];
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar5 = 0;
  bVar8 = (char)(puVar1[4] >> 8) < '\0';
  if ((*(byte *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 2) & 4) != 0) {
    bVar8 = bVar8 | 4;
  }
  cVar3 = object_lighting_sample_point(bVar8,puVar1 + 0x28,param_1);
  if ((puVar1[4] & 0x4000) == 0) {
    if (cVar3 == '\0') {
      pfVar6 = param_1;
      for (iVar4 = 0x1d; iVar4 != 0; iVar4 = iVar4 + -1) {
        *pfVar6 = 0.0;
        pfVar6 = pfVar6 + 1;
      }
      *(undefined2 *)(param_1 + 3) = 2;
      sVar7 = 0;
    }
    else {
      sVar7 = 1;
    }
    do {
      if ((uVar5 & 1) == 0) {
        fVar2 = -0.70710677;
      }
      else {
        fVar2 = 0.70710677;
      }
      local_80 = fVar2 * (float)puVar1[0x2b] + (float)puVar1[0x28];
      if ((uVar5 & 2) == 0) {
        fVar2 = -0.70710677;
      }
      else {
        fVar2 = 0.70710677;
      }
      local_78 = puVar1[0x2a];
      local_7c = fVar2 * (float)puVar1[0x2b] + (float)puVar1[0x29];
      cVar3 = object_lighting_sample_point(bVar8,&local_80,local_74);
      if (cVar3 != '\0') {
        sVar7 = sVar7 + 1;
        *param_1 = local_74[0] + *param_1;
        param_1[1] = local_74[1] + param_1[1];
        param_1[2] = local_74[2] + param_1[2];
        param_1[0x13] = local_28 + param_1[0x13];
        param_1[0x14] = local_24 + param_1[0x14];
        param_1[0x15] = local_20 + param_1[0x15];
        param_1[0x16] = local_1c + param_1[0x16];
        param_1[4] = local_64 + param_1[4];
        param_1[5] = local_60 + param_1[5];
        param_1[6] = local_5c + param_1[6];
        param_1[7] = local_58 + param_1[7];
        param_1[8] = local_54 + param_1[8];
        param_1[9] = local_50 + param_1[9];
        param_1[10] = local_4c + param_1[10];
        param_1[0xb] = local_48 + param_1[0xb];
        param_1[0xc] = local_44 + param_1[0xc];
        param_1[0xd] = local_40 + param_1[0xd];
        param_1[0xe] = local_3c + param_1[0xe];
        param_1[0xf] = local_38 + param_1[0xf];
        param_1[0x1a] = local_c + param_1[0x1a];
        param_1[0x1b] = local_8 + param_1[0x1b];
        param_1[0x1c] = local_4 + param_1[0x1c];
        param_1[0x17] = local_18 + param_1[0x17];
        param_1[0x18] = local_14 + param_1[0x18];
        param_1[0x19] = local_10 + param_1[0x19];
      }
      uVar5 = uVar5 + 1;
    } while ((short)uVar5 < 4);
    if (1 < sVar7) {
      fVar2 = 1.0 / (float)(int)sVar7;
      *param_1 = fVar2 * *param_1;
      param_1[1] = fVar2 * param_1[1];
      param_1[2] = fVar2 * param_1[2];
      param_1[0x13] = fVar2 * param_1[0x13];
      param_1[0x14] = fVar2 * param_1[0x14];
      param_1[0x15] = fVar2 * param_1[0x15];
      param_1[0x16] = fVar2 * param_1[0x16];
      param_1[4] = fVar2 * param_1[4];
      param_1[5] = fVar2 * param_1[5];
      param_1[6] = fVar2 * param_1[6];
      param_1[7] = fVar2 * param_1[7];
      param_1[8] = fVar2 * param_1[8];
      param_1[9] = fVar2 * param_1[9];
      vector3d_normalize_with_length();
      param_1[10] = fVar2 * param_1[10];
      param_1[0xb] = fVar2 * param_1[0xb];
      param_1[0xc] = fVar2 * param_1[0xc];
      param_1[0xd] = fVar2 * param_1[0xd];
      param_1[0xe] = fVar2 * param_1[0xe];
      param_1[0xf] = fVar2 * param_1[0xf];
      vector3d_normalize_with_length();
      param_1[0x1a] = fVar2 * param_1[0x1a];
      param_1[0x1b] = fVar2 * param_1[0x1b];
      param_1[0x1c] = fVar2 * param_1[0x1c];
      param_1[0x17] = fVar2 * param_1[0x17];
      param_1[0x18] = fVar2 * param_1[0x18];
      param_1[0x19] = fVar2 * param_1[0x19];
      vector3d_normalize_with_length();
      return;
    }
    if (sVar7 == 0) {
      pfVar6 = local_74;
      for (iVar4 = 0x1d; iVar4 != 0; iVar4 = iVar4 + -1) {
        *param_1 = *pfVar6;
        pfVar6 = pfVar6 + 1;
        param_1 = param_1 + 1;
      }
    }
  }
  return;
}
#endif
