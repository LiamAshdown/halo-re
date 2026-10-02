// particle_next_sequence  (Ghidra: FUN_00455e60, still unnamed there; named directly by
//   types/effects.h: "particle_next_sequence 0x455e60 walks. It reads Particle
//   first_sequence_index at 0x98, initial_sequence_count at 0x9a, looping_sequence_count at
//   0x9c and final_sequence_count at 0x9e")
// address 0x455e60, size 406 bytes
// name confidence: 0.6   rewrite confidence: 0.9 (VERIFIED against objdump 0x455e60..0x455ff5; initial->looping fallthrough FIXED)
// evidence: types/effects.h particle.sequence_state (+0x0e), sequence_index (+0x24),
//   particle_sequence_state enum (_new/_initial/_looping/_final/_finished); types/tags.h
//   Particle.first_sequence_index/initial_sequence_count/looping_sequence_count/
//   final_sequence_count, Particle.bitmap (TagDependency), Bitmap.bitmap_group_sequence
//   (TagReflexive of BitmapGroupSequence), BitmapGroupSequence.sprites (TagReflexive, its
//   count is the per-sequence frame count read at +0x34 in the raw disassembly).
// register convention: particle handle in EAX (in_EAX).
//   // blam-cc: EAX -> particle_handle
// UNSURE: the fallback when the rolled sequence_index cannot be resolved into a live frame
//   (bitmap_group_sequence empty) calls particle_impact 0x456550 with no visible argument, kept
//   here as the same particle_handle by register convention.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *particle_data;   // 0x0087abd0
extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed effect_random_seed; // 0x00719cd4

extern void particle_impact(datum_index particle_handle); // 0x456550, this module

// Walks a particle's sequence state machine (new -> initial -> looping -> final -> finished),
// rolling a random frame out of the Particle tag's sequence bounds each time a state is
// (re)entered, and clamps the result against the number of sequences the live bitmap actually
// has. Returns nonzero while the particle still has a valid frame to render; on failure it
// defers to particle_impact and reports false.
uint8_t particle_next_sequence(datum_index particle_handle)
{
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)tag_instances[(uint16_t)self->definition_index].data;
    Bitmap *bitmap = (Bitmap *)tag_instances[tag->bitmap.tag_id.index].data;

    self->sequence_index = -1;

    if (self->sequence_state == _particle_sequence_state_new) {
        if (tag->initial_sequence_count > 0) {
            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            self->sequence_index = (int16_t)(((effect_random_seed >> k_random_value_shift) *
                (uint32_t)(int32_t)tag->initial_sequence_count) >> 16) + tag->first_sequence_index;
        }
        self->sequence_state = _particle_sequence_state_initial;
    }

    // FIXED (0x455eeb..0x455f08): leaving the initial state falls straight into the looping pick in the same
    //   call; the draft's `else if` skipped it, so a particle without initial sequences got -1 and was impacted.
    if (self->sequence_index == -1 && self->sequence_state == _particle_sequence_state_initial) {
        self->sequence_state = _particle_sequence_state_looping;
    }
    if (self->sequence_state == _particle_sequence_state_looping) {
        if (!(self->age < self->lifespan) || tag->looping_sequence_count < 1) {
            self->sequence_state = self->sequence_state + 1;
        } else {
            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            self->sequence_index = (int16_t)(((effect_random_seed >> k_random_value_shift) *
                (uint32_t)(int32_t)tag->looping_sequence_count) >> 16) +
                tag->initial_sequence_count + tag->first_sequence_index;
        }
    }

    if (self->sequence_index == -1 && self->sequence_state == _particle_sequence_state_final) {
        if (tag->final_sequence_count > 0) {
            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            self->sequence_index = (int16_t)(((effect_random_seed >> k_random_value_shift) *
                (uint32_t)(int32_t)tag->final_sequence_count) >> 16) +
                tag->looping_sequence_count + tag->initial_sequence_count + tag->first_sequence_index;
        }
        self->sequence_state = self->sequence_state + 1;
    }

    if (self->sequence_index != -1 && bitmap->bitmap_group_sequence.count != 0) {
        int32_t clamped = self->sequence_index;
        int32_t max_index = (int32_t)bitmap->bitmap_group_sequence.count - 1;

        if (clamped < 0) {
            self->sequence_index = 0;
            return 1;
        }
        if (max_index < clamped) {
            clamped = max_index;
        }
        self->sequence_index = (int16_t)clamped;
        return 1;
    }

    particle_impact(particle_handle);
    return 0;
}

#if 0
Original Ghidra decompilation (0x455e60):

uint FUN_00455e60(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;

  iVar4 = (in_EAX & 0xffff) * 0x70;
  iVar5 = iVar4 + *(int *)(DAT_0087abd0 + 0x34);
  iVar4 = *(int *)((*(uint *)(iVar4 + 4 + *(int *)(DAT_0087abd0 + 0x34)) & 0xffff) * 0x20 + 0x14 +
                  DAT_0087bc14);
  iVar2 = *(int *)((*(uint *)(iVar4 + 0x10) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  *(undefined2 *)(iVar5 + 0x24) = 0xffff;
  if (*(char *)(iVar5 + 0xe) == '\0') {
    if (0 < *(short *)(iVar4 + 0x9a)) {
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      *(short *)(iVar5 + 0x24) =
           (short)((DAT_00719cd4 >> 0x10) * (int)*(short *)(iVar4 + 0x9a) >> 0x10) +
           *(short *)(iVar4 + 0x98);
    }
    *(char *)(iVar5 + 0xe) = *(char *)(iVar5 + 0xe) + '\x01';
  }
  if ((*(short *)(iVar5 + 0x24) == -1) && (*(char *)(iVar5 + 0xe) == '\x01')) {
    *(undefined1 *)(iVar5 + 0xe) = 2;
  }
  else if (*(char *)(iVar5 + 0xe) != '\x02') goto LAB_00455f5c;
  if ((*(float *)(iVar5 + 0x18) <= *(float *)(iVar5 + 0x14)) || (*(short *)(iVar4 + 0x9c) < 1)) {
    *(char *)(iVar5 + 0xe) = *(char *)(iVar5 + 0xe) + '\x01';
  }
  else {
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    *(short *)(iVar5 + 0x24) =
         (short)((DAT_00719cd4 >> 0x10) * (int)*(short *)(iVar4 + 0x9c) >> 0x10) +
         *(short *)(iVar4 + 0x9a) + *(short *)(iVar4 + 0x98);
  }
LAB_00455f5c:
  if ((*(short *)(iVar5 + 0x24) == -1) && (*(char *)(iVar5 + 0xe) == '\x03')) {
    if (0 < *(short *)(iVar4 + 0x9e)) {
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      *(short *)(iVar5 + 0x24) =
           (short)((DAT_00719cd4 >> 0x10) * (int)*(short *)(iVar4 + 0x9e) >> 0x10) +
           *(short *)(iVar4 + 0x9c) + *(short *)(iVar4 + 0x9a) + *(short *)(iVar4 + 0x98);
    }
    *(char *)(iVar5 + 0xe) = *(char *)(iVar5 + 0xe) + '\x01';
  }
  sVar1 = *(short *)(iVar5 + 0x24);
  if ((sVar1 != -1) && (iVar4 = *(int *)(iVar2 + 0x54), iVar4 != 0)) {
    if (sVar1 < 0) {
      *(undefined2 *)(iVar5 + 0x24) = 0;
      return 1;
    }
    iVar4 = iVar4 + -1;
    iVar2 = (int)sVar1;
    if (iVar4 < sVar1) {
      iVar2 = iVar4;
    }
    *(short *)(iVar5 + 0x24) = (short)iVar2;
    return CONCAT31((int3)((uint)iVar2 >> 8),1);
  }
  uVar3 = FUN_00456550();
  return uVar3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
