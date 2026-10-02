// glow_particle_new
// address 0x4fd8e0, size 572 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fd8e0 |
//   lightning_segment_new | glow_particle_new"; matches functions.md: "Allocates and randomly
//   initializes one lightning segment's amplitude, period, color and phase from the tag's
//   configured ranges" -- actually a glow particle, per that same table)
// rewrite confidence: 0.4 (random-fill logic is straightforward; the specific Glow tag offsets
//   used are kept raw rather than matched to tags.h field names, consistent with the rest of
//   this file group)
// evidence: types/objects.h glow (definition_tag 0x224, particle_count 0x228, total_length
//   0x234), glow_particle (t 0x28); this module's glow_particle_datum_new 0x4fdde0.
// register convention: Ghidra shows a clean (short param_1, short param_2) plus one unresolved
//   `unaff_EDI`; by the same reasoning as every other function in this file group, EDI is the
//   glow entry.
// blam-cc: EDI -> entry, stack -> index, count

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t effect_random_seed; // 0x00719cd4

extern glow_particle *glow_particle_datum_new(void); // this module, 0x4fdde0

static float glow_next_random_unit(void)
{
    effect_random_seed = effect_random_seed * 0x19660dU + 0x3c6ef35fU;
    return (float)(effect_random_seed >> 16) * 1.5259022e-05f; // ~1/65536, [0,1)
}

glow_particle *glow_particle_new(glow *entry /*EDI*/, int16_t index, int16_t count)
    // blam-cc: EDI -> entry, stack -> index, count
{
    uint8_t *tag = (uint8_t *)tag_instances[entry->definition_tag & 0xffff].data;
    glow_particle *p = glow_particle_datum_new();

    if (p != 0) {
        uint8_t *pb = (uint8_t *)p;

        if (*(int16_t *)(tag + 0x80) == -1) {
            *(float *)(pb + 0x1c) = (*(float *)(tag + 0x88) - *(float *)(tag + 0x84)) *
                                     glow_next_random_unit() + *(float *)(tag + 0x84);
        }
        if (*(int16_t *)(tag + 0x9c) == -1) {
            float v = *(float *)(tag + 0xa0) +
                      (*(float *)(tag + 0xa4) - *(float *)(tag + 0xa0)) * glow_next_random_unit();
            *(float *)(pb + 0x20) = v / (float)entry->particle_count;
        }
        if (*(int16_t *)(tag + 0xb0) == -1 && (tag[0x28] & 1) == 0) {
            float t = glow_next_random_unit();
            *(uint32_t *)(pb + 0xc) = 0x3f800000; // 1.0f
            *(float *)(pb + 0x10) = (*(float *)(tag + 0xc8) - *(float *)(tag + 0xb8)) * t + *(float *)(tag + 0xb8);
            *(float *)(pb + 0x14) = (*(float *)(tag + 0xcc) - *(float *)(tag + 0xbc)) * t + *(float *)(tag + 0xbc);
            *(float *)(pb + 0x18) = (*(float *)(tag + 0xd0) - *(float *)(tag + 0xc0)) * t + *(float *)(tag + 0xc0);
        }
        if (*(int16_t *)(tag + 0x24) == 0) {
            ((struct glow_particle *)pb)->t = glow_next_random_unit() * entry->total_length;
            *(float *)(pb + 8) = glow_next_random_unit() * 6.2831855f; // 2*pi
        } else if (*(int16_t *)(tag + 0x24) == 1) {
            ((struct glow_particle *)pb)->t = ((float)index / (float)count) * entry->total_length;
            *(float *)(pb + 8) = glow_next_random_unit() * 6.2831855f;
        }
    }
    return p;
}

#if 0
Original Ghidra decompilation (0x4fd8e0):

void FUN_004fd8e0(short param_1,short param_2)

{
  int iVar1;
  float fVar2;
  int iVar3;
  uint uVar4;
  int unaff_EDI;

  iVar1 = *(int *)((*(uint *)(unaff_EDI + 0x224) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar3 = FUN_004fdde0();
  if (iVar3 != 0) {
    if (*(short *)(iVar1 + 0x80) == -1) {
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      *(float *)(iVar3 + 0x1c) =
           (*(float *)(iVar1 + 0x88) - *(float *)(iVar1 + 0x84)) *
           (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 + *(float *)(iVar1 + 0x84);
    }
    if (*(short *)(iVar1 + 0x9c) == -1) {
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      fVar2 = *(float *)(iVar1 + 0xa0) +
              (*(float *)(iVar1 + 0xa4) - *(float *)(iVar1 + 0xa0)) *
              (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
      *(float *)(iVar3 + 0x20) = fVar2;
      *(float *)(iVar3 + 0x20) = fVar2 / (float)(int)*(short *)(unaff_EDI + 0x228);
    }
    if ((*(short *)(iVar1 + 0xb0) == -1) && ((*(byte *)(iVar1 + 0x28) & 1) == 0)) {
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      uVar4 = DAT_00719cd4 >> 0x10;
      *(undefined4 *)(iVar3 + 0xc) = 0x3f800000;
      fVar2 = (float)uVar4 * 1.5259022e-05;
      *(float *)(iVar3 + 0x10) =
           (*(float *)(iVar1 + 200) - *(float *)(iVar1 + 0xb8)) * fVar2 + *(float *)(iVar1 + 0xb8);
      *(float *)(iVar3 + 0x14) =
           (*(float *)(iVar1 + 0xcc) - *(float *)(iVar1 + 0xbc)) * fVar2 + *(float *)(iVar1 + 0xbc);
      *(float *)(iVar3 + 0x18) =
           (*(float *)(iVar1 + 0xd0) - *(float *)(iVar1 + 0xc0)) * fVar2 + *(float *)(iVar1 + 0xc0);
    }
    if (*(short *)(iVar1 + 0x24) == 0) {
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      *(float *)(iVar3 + 0x28) =
           (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * *(float *)(unaff_EDI + 0x234);
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      *(float *)(iVar3 + 8) = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * 6.2831855;
    }
    else if (*(short *)(iVar1 + 0x24) == 1) {
      *(float *)(iVar3 + 0x28) =
           ((float)(int)param_1 / (float)(int)param_2) * *(float *)(unaff_EDI + 0x234);
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      *(float *)(iVar3 + 8) = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * 6.2831855;
      return;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
