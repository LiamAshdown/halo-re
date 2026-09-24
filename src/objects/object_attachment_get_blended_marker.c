// object_attachment_get_blended_marker
// address 0x4fe740, size 443 bytes, zero recorded callers
// name confidence: 0.75 (types/objects.h's globals list names the function's own return value
//   directly: "global 0x006b8cc0: object_marker object_marker_scratch //
//   object_attachment_get_blended_marker result")
// rewrite confidence: 0.3 (the blend arithmetic Ghidra shows -- `fVar1 * *puVar2 + (1-fVar1) *
//   *puVar2`, i.e. the SAME source pointer on both sides of every blend -- is a mathematical
//   identity (always yields the unmodified source value) and functions.md's summary says this
//   interpolates BETWEEN TWO STATES, so a second source pointer almost certainly exists and was
//   lost by the decompiler. Preserved literally rather than guessed at, per "no invented
//   behaviour"; every blended output field is simply a copy of the corresponding input field.)
// evidence: types/objects.h object (function_out_values 0x134, function_valid_flags 0x123);
//   global 0x008603b0 object_data; global 0x006b8cc0 object_marker_scratch.
// register convention: Ghidra shows unresolved `in_EAX` and `in_ECX`; by the object_data lookup
//   shape (matching every other function in this file that resolves an object from an index),
//   EAX is the object index; ECX is some per-widget-instance record with a (count, pointer) pair
//   at +0x120/+0x124 and a function-selector byte at +0xb8 -- not otherwise identified.
// blam-cc: EAX -> object_index, ECX -> instance
// UNSURE: `instance` (in_ECX)'s type is not identified; kept as a raw byte pointer.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t object_marker_scratch[0x6c]; // 0x006b8cc0

uint8_t *object_attachment_get_blended_marker(uint32_t object_index /*EAX*/, uint8_t *instance /*ECX*/)
    // blam-cc: EAX -> object_index, ECX -> instance
{
    uint8_t *source = *(uint8_t **)(instance + 0x124);

    if (*(int32_t *)(instance + 0x120) < 2) {
        return source;
    }

    {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        int16_t selector = *(int16_t *)(instance + 0xb8) - 1;
        float weight;

        if (selector == -1) {
            weight = 1.0f;
        } else {
            weight = *(float *)((uint8_t *)obj + 0x134 + selector * 4);
            if (((1 << (selector & 0x1f)) & *((uint8_t *)obj + 0x123)) == 0) {
                return source;
            }
        }

        {
            float inv = 1.0f - weight;
            static const int offsets[16] = {
                0x10, 0x14, 0x18, 0x3c, 0x40, 0x44, 0x68, 0x6c,
                0x70, 0x74, 0x78, 0x7c, 0x80, 0x84, 0x88, 0x8c
            };
            int i;
            for (i = 0; i < 16; i++) {
                float v = *(float *)(source + offsets[i]);
                *(float *)(object_marker_scratch + 0x10 + i * 4) = weight * v + inv * v; // == v
            }
        }
    }
    return object_marker_scratch;
}

#if 0
Original Ghidra decompilation (0x4fe740):

undefined * FUN_004fe740(void)

{
  float fVar1;
  undefined *puVar2;
  int iVar3;
  float fVar4;
  uint in_EAX;
  short sVar5;
  int in_ECX;

  puVar2 = *(undefined **)(in_ECX + 0x124);
  if (*(int *)(in_ECX + 0x120) < 2) {
    return puVar2;
  }
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  sVar5 = *(short *)(in_ECX + 0xb8) + -1;
  if (sVar5 == -1) {
    fVar1 = 1.0;
  }
  else {
    fVar1 = *(float *)(iVar3 + 0x134 + sVar5 * 4);
    if (((byte)(1 << ((byte)sVar5 & 0x1f)) & *(byte *)(iVar3 + 0x123)) == 0) {
      return puVar2;
    }
  }
  fVar4 = 1.0 - fVar1;
  _DAT_006b8cd0 = fVar1 * *(float *)(puVar2 + 0x10) + fVar4 * *(float *)(puVar2 + 0x10);
  _DAT_006b8cd4 = fVar1 * *(float *)(puVar2 + 0x14) + fVar4 * *(float *)(puVar2 + 0x14);
  _DAT_006b8cd8 = fVar1 * *(float *)(puVar2 + 0x18) + fVar4 * *(float *)(puVar2 + 0x18);
  _DAT_006b8cfc = fVar1 * *(float *)(puVar2 + 0x3c) + fVar4 * *(float *)(puVar2 + 0x3c);
  _DAT_006b8d00 = fVar1 * *(float *)(puVar2 + 0x40) + fVar4 * *(float *)(puVar2 + 0x40);
  _DAT_006b8d04 = fVar1 * *(float *)(puVar2 + 0x44) + fVar4 * *(float *)(puVar2 + 0x44);
  _DAT_006b8d28 = fVar1 * *(float *)(puVar2 + 0x68) + fVar4 * *(float *)(puVar2 + 0x68);
  _DAT_006b8d2c = fVar1 * *(float *)(puVar2 + 0x6c) + fVar4 * *(float *)(puVar2 + 0x6c);
  _DAT_006b8d30 = fVar1 * *(float *)(puVar2 + 0x70) + fVar4 * *(float *)(puVar2 + 0x70);
  _DAT_006b8d34 = fVar1 * *(float *)(puVar2 + 0x74) + fVar4 * *(float *)(puVar2 + 0x74);
  _DAT_006b8d38 = fVar1 * *(float *)(puVar2 + 0x78) + fVar4 * *(float *)(puVar2 + 0x78);
  _DAT_006b8d3c = fVar1 * *(float *)(puVar2 + 0x7c) + fVar4 * *(float *)(puVar2 + 0x7c);
  _DAT_006b8d40 = fVar1 * *(float *)(puVar2 + 0x80) + fVar4 * *(float *)(puVar2 + 0x80);
  _DAT_006b8d44 = fVar1 * *(float *)(puVar2 + 0x84) + fVar4 * *(float *)(puVar2 + 0x84);
  _DAT_006b8d48 = fVar1 * *(float *)(puVar2 + 0x88) + fVar4 * *(float *)(puVar2 + 0x88);
  _DAT_006b8d4c = fVar1 * *(float *)(puVar2 + 0x8c) + fVar4 * *(float *)(puVar2 + 0x8c);
  return &DAT_006b8cc0;
}
#endif
