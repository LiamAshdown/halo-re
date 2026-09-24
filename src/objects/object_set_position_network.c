// object_set_position_network  (Ghidra: object_set_position_network, already named)
// address 0x4f8bd0, size 234 bytes
// name confidence: 0.8 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Applies a batch of predicted/network positions to objects,
//   snapping to tolerance and clamping derived blend weights")
// rewrite confidence: 0.3
// evidence: types/objects.h object_placement_data.network_vectors (0x58, four vectors handed
//   to this function); callee color_interpolate (0x43f6a0, established); caller FUN_004f8b70
//   (this batch), which forwards obj+0x188 as the destination array with count=4, matching the
//   four network_vectors.
// register convention: Object tag pointer in EAX, destination array in EDI; the remaining
//   inputs are stack values (loop start index, a byte-stride accumulator, the source array, and
//   the loop count) that this function's own body re-derives every iteration rather than taking
//   as clean parameters -- collapsed here into a plain loop. Confirmed against objdump
//   -d -M intel bin/halo.exe via its only caller, FUN_004f8b70 (this batch).
//   // blam-cc: EAX -> tag, EDI -> dest, stack -> start_index, source, count
// UNSURE: FUN_00628cca (a zero-argument RNG-shaped callee) and the threshold-table lookup at
//   tag+0x168 (count at +0x20, float array stride 0x1c at +0x24) are not otherwise established
//   in this module; preserved as raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern void color_interpolate(void *out_color, uint32_t color_pair, float t); // 0x43f6a0
extern float FUN_00628cca(void); // 0x628cca, UNSURE: foreign/libc-shaped RNG, see file header

void object_set_position_network(uint8_t *tag, real_vector3d *dest, int32_t start_index,
    real_vector3d *source, int32_t count) // blam-cc: EAX -> tag, EDI -> dest, stack -> start_index, source, count
{
    int32_t byte_stride = start_index * 0x2c;
    int32_t i;

    for (i = 0; i < count; i++) {
        dest[0] = source[i];

        if (start_index + i < *(int32_t *)(tag + 0x164)) {
            uint8_t *threshold_record = *(uint8_t **)(tag + 0x168) + byte_stride;
            float roll = FUN_00628cca();
            int32_t threshold_count = *(int32_t *)(threshold_record + 0x20);
            int32_t j;
            for (j = 0; j < threshold_count; j++) {
                float threshold = *(float *)(*(int32_t *)(threshold_record + 0x24) + j * 0x1c);
                if (roll > threshold) {
                    float blend = FUN_00628cca();
                    color_interpolate(dest, 1, blend);
                    break;
                }
            }
        }

        {
            float *component = (float *)dest;
            int32_t c;
            for (c = 0; c < 3; c++) {
                float v = component[c];
                if (v < 0.0f) {
                    v = 0.0f;
                } else if (v > 1.0f) {
                    v = 1.0f;
                }
                ((float *)dest)[0xc + c] = v;
            }
        }

        byte_stride += 0x2c;
        dest += 3;
    }
}

#if 0
Original Ghidra decompilation (0x4f8bd0):

void object_set_position_network(void)

{
  float fVar1;
  int iVar2;
  int in_EAX;
  int iVar3;
  short sVar4;
  int iVar5;
  float *unaff_EDI;
  float10 fVar6;
  int in_stack_00000010;
  int in_stack_00000014;
  float *in_stack_00000018;
  float fStack0000001c;
  int in_stack_00000020;
  int in_stack_00000024;

  do {
    *unaff_EDI = *in_stack_00000018;
    unaff_EDI[1] = in_stack_00000018[1];
    unaff_EDI[2] = in_stack_00000018[2];
    if (in_stack_00000010 < *(int *)(in_EAX + 0x164)) {
      fStack0000001c = (float)in_stack_00000010;
      iVar5 = *(int *)(in_EAX + 0x168) + in_stack_00000014;
      fVar6 = (float10)FUN_00628cca();
      iVar2 = *(int *)(iVar5 + 0x20);
      sVar4 = 0;
      if (0 < iVar2) {
        iVar3 = 0;
        do {
          fVar1 = *(float *)(iVar3 * 0x1c + *(int *)(iVar5 + 0x24));
          if ((float)fVar6 < fVar1 != ((float)fVar6 == fVar1)) {
            fVar6 = (float10)FUN_00628cca();
            color_interpolate(unaff_EDI,1,(float)fVar6);
            break;
          }
          sVar4 = sVar4 + 1;
          iVar3 = (int)sVar4;
        } while (iVar3 < iVar2);
      }
    }
    if (0.0 <= *unaff_EDI) {
      if (*unaff_EDI <= 1.0) {
        fVar1 = *unaff_EDI;
      }
      else {
        fVar1 = 1.0;
      }
    }
    else {
      fVar1 = 0.0;
    }
    unaff_EDI[0xc] = fVar1;
    if (0.0 <= unaff_EDI[1]) {
      if (unaff_EDI[1] <= 1.0) {
        fVar1 = unaff_EDI[1];
      }
      else {
        fVar1 = 1.0;
      }
    }
    else {
      fVar1 = 0.0;
    }
    unaff_EDI[0xd] = fVar1;
    if (0.0 <= unaff_EDI[2]) {
      if (unaff_EDI[2] <= 1.0) {
        fVar1 = unaff_EDI[2];
      }
      else {
        fVar1 = 1.0;
      }
    }
    else {
      fVar1 = 0.0;
    }
    unaff_EDI[0xe] = fVar1;
    in_stack_00000010 = in_stack_00000010 + 1;
    in_stack_00000014 = in_stack_00000014 + 0x2c;
    in_stack_00000018 = in_stack_00000018 + 3;
    unaff_EDI = unaff_EDI + 3;
    in_stack_00000020 = in_stack_00000020 + -1;
    in_EAX = in_stack_00000024;
    if (in_stack_00000020 == 0) {
      return;
    }
  } while( true );
}
#endif
