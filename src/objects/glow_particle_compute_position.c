// glow_particle_compute_position
// address 0x4fd4a0, size 431 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fd4a0 |
//   lightning_segment_compute_position | glow_particle_compute_position")
// rewrite confidence: 0.25 (every field this touches past glow_particle+0x08 falls inside the
//   struct's documented "unknown_08[0x20]" blob or the later gap before `t` at 0x28; kept as
//   raw offsets throughout rather than guessed field names)
// evidence: types/objects.h glow (definition_tag 0x224, total_length 0x234), object
//   (function_out_values 0x134, function_valid_flags 0x123); global 0x008603b0 object_data.
// register convention: Ghidra shows `uint in_ECX`, `int unaff_EBX`, `int unaff_ESI` with no
//   local definitions; by the same reasoning as glow_particle_compute_fade.c / _color.c (same
//   +0x224 glow-entry read), unaff_EBX is the glow entry; unaff_ESI matches the particle
//   pointer shape used throughout glow_update.c's second particle-walk loop
//   (+0x50/+0x52/+0x58 age/lifetime/fade); in_ECX is a plain object index (tested against
//   object_data and object.function_out_values/function_valid_flags).
// blam-cc: ECX -> object_index, EBX -> entry, ESI -> particle (register roles inferred by
//   analogy with the sibling functions in this file group; not independently confirmed by
//   disassembling a call site, since glow_update.c's own calls to this function were already
//   opaque in Ghidra's decompile)
// UNSURE: the whole function is preserved as raw offsets rather than named fields, per the
//   confidence note above; the two blended-colour-bound sections (glow_flags bit 0 unset vs
//   bit 0 set) and the trailing-fade computation at the end are transliterated mechanically.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *object_data;     // 0x008603b0

void glow_particle_compute_position(uint32_t object_index /*ECX*/, glow *entry /*EBX*/,
                                     glow_particle *particle /*ESI*/)
    // blam-cc: ECX -> object_index, EBX -> entry, ESI -> particle
{
    uint8_t *tag = (uint8_t *)tag_instances[entry->definition_tag & 0xffff].data;
    int16_t attachment = *(int16_t *)(tag + 0xb0);
    uint8_t *p = (uint8_t *)particle;

    if (attachment != -1) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        float driver = *(float *)((uint8_t *)obj + 0x134 + attachment * 4);

        if (((1 << (attachment & 0x1f)) & *((uint8_t *)obj + 0x123)) == 0) {
            driver = 0.0f;
        }

        *(float *)(p + 0x10) = (*(float *)(tag + 0xc8) - *(float *)(tag + 0xb8)) * driver + *(float *)(tag + 0xb8);
        *(float *)(p + 0x14) = (*(float *)(tag + 0xcc) - *(float *)(tag + 0xbc)) * driver + *(float *)(tag + 0xbc);
        *(uint32_t *)(p + 0xc) = 0x3f800000; // 1.0f
        *(float *)(p + 0x18) = (*(float *)(tag + 0xd0) - *(float *)(tag + 0xc0)) * driver + *(float *)(tag + 0xc0);
    }

    if ((tag[0x28] & 1) != 0) {
        float t = *(float *)(p + 0x28);
        float rate = *(float *)(tag + 0xf4);

        *(float *)(p + 0x10) = (*(float *)(tag + 0xc8) - *(float *)(tag + 0xb8)) * rate * t + *(float *)(tag + 0xb8);
        *(float *)(p + 0x14) = (*(float *)(tag + 0xcc) - *(float *)(tag + 0xbc)) * rate * t + *(float *)(tag + 0xbc);
        *(uint32_t *)(p + 0xc) = 0x3f800000; // 1.0f
        *(float *)(p + 0x18) = (*(float *)(tag + 0xd0) - *(float *)(tag + 0xc0)) * rate * t + *(float *)(tag + 0xc0);
    }

    {
        float t = *(float *)(p + 0x28) / entry->total_length;
        float half_span = *(float *)(tag + 0xf8) * 0.5f;
        float fade;

        if (t >= half_span) {
            if (t <= 1.0f - half_span) {
                fade = 1.0f;
            } else {
                fade = (1.0f - t) / half_span;
            }
        } else {
            fade = t / half_span;
        }

        if (fade < 0.0f) {
            fade = 0.0f;
        } else if (fade > 1.0f) {
            fade = 1.0f;
        }
        *(float *)(p + 0x58) = fade;
    }
}

#if 0
Original Ghidra decompilation (0x4fd4a0):

void FUN_004fd4a0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  short sVar5;
  int iVar6;
  int iVar7;
  uint in_ECX;
  int unaff_EBX;
  int unaff_ESI;

  iVar6 = *(int *)((*(uint *)(unaff_EBX + 0x224) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar5 = *(short *)(iVar6 + 0xb0);
  if (sVar5 != -1) {
    iVar7 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    fVar1 = *(float *)(iVar7 + 0x134 + sVar5 * 4);
    if (((byte)(1 << ((byte)sVar5 & 0x1f)) & *(byte *)(iVar7 + 0x123)) == 0) {
      fVar1 = 0.0;
    }
    *(float *)(unaff_ESI + 0x10) =
         (*(float *)(iVar6 + 200) - *(float *)(iVar6 + 0xb8)) * fVar1 + *(float *)(iVar6 + 0xb8);
    *(float *)(unaff_ESI + 0x14) =
         (*(float *)(iVar6 + 0xcc) - *(float *)(iVar6 + 0xbc)) * fVar1 + *(float *)(iVar6 + 0xbc);
    fVar2 = *(float *)(iVar6 + 0xd0);
    fVar3 = *(float *)(iVar6 + 0xc0);
    fVar4 = *(float *)(iVar6 + 0xc0);
    *(undefined4 *)(unaff_ESI + 0xc) = 0x3f800000;
    *(float *)(unaff_ESI + 0x18) = (fVar2 - fVar3) * fVar1 + fVar4;
  }
  if ((*(byte *)(iVar6 + 0x28) & 1) != 0) {
    *(float *)(unaff_ESI + 0x10) =
         (*(float *)(iVar6 + 200) - *(float *)(iVar6 + 0xb8)) * *(float *)(iVar6 + 0xf4) *
         *(float *)(unaff_ESI + 0x28) + *(float *)(iVar6 + 0xb8);
    *(float *)(unaff_ESI + 0x14) =
         (*(float *)(iVar6 + 0xcc) - *(float *)(iVar6 + 0xbc)) * *(float *)(iVar6 + 0xf4) *
         *(float *)(unaff_ESI + 0x28) + *(float *)(iVar6 + 0xbc);
    fVar1 = *(float *)(iVar6 + 0xd0);
    fVar2 = *(float *)(iVar6 + 0xc0);
    fVar3 = *(float *)(iVar6 + 0xf4);
    fVar4 = *(float *)(iVar6 + 0xc0);
    *(undefined4 *)(unaff_ESI + 0xc) = 0x3f800000;
    *(float *)(unaff_ESI + 0x18) = (fVar1 - fVar2) * fVar3 * *(float *)(unaff_ESI + 0x28) + fVar4;
  }
  fVar1 = *(float *)(unaff_ESI + 0x28) / *(float *)(unaff_EBX + 0x234);
  fVar2 = *(float *)(iVar6 + 0xf8) * 0.5;
  if (fVar2 <= fVar1) {
    if (fVar1 <= 1.0 - fVar2) {
      *(undefined4 *)(unaff_ESI + 0x58) = 0x3f800000;
    }
    else {
      *(float *)(unaff_ESI + 0x58) = (1.0 - fVar1) / fVar2;
    }
  }
  else {
    *(float *)(unaff_ESI + 0x58) = fVar1 / fVar2;
  }
  if (0.0 <= *(float *)(unaff_ESI + 0x58)) {
    if (*(float *)(unaff_ESI + 0x58) <= 1.0) {
      *(undefined4 *)(unaff_ESI + 0x58) = *(undefined4 *)(unaff_ESI + 0x58);
      return;
    }
    *(undefined4 *)(unaff_ESI + 0x58) = 0x3f800000;
    return;
  }
  *(undefined4 *)(unaff_ESI + 0x58) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
