// actor_targets_share_descriptor  (Ghidra: actor_targets_share_descriptor, renamed)
// address 0x40e380, size 273 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (FIXED: target props come from actor +0x270 (0x40e402/0x40e413); rest verified against 0x40e380)
// evidence: phase-4 summary "determines whether two actors are considered to share the
// same target/threat descriptor, with a random tie-break for the ambiguous case"; both
// actors must be in mode 5 or 7, and the comparison reads a 2-byte "kind" and a 2-byte
// "extra" field at mode_data+0x08/+0x0a (actor+0xa4/+0xa6).
// register convention: actor_a in EAX, actor_b in ECX (Ghidra's in_EAX/in_ECX).
// blam-cc: EAX -> actor_a, ECX -> actor_b
// UNSURE: datum_get() is called twice with zero visible arguments; guessed here as reading
// a datum_index at mode_data+0x0c (actor+0xa8) of each actor, by analogy with the kind/extra
// pair immediately before it. (The objdump shows EDX = actor + 0x270 at both calls and ESI =
// prop_data; not re-derived here.) 0x401020 is vector3d_distance_squared on the two props'
// last_known_position (EAX / ECX, 0x40e429..0x40e434): they share when within 0.7 units.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0, UNSURE guess at which array datum_get checks here
extern void * datum_get(datum_index handle, data_array *array); // 0x4d0680
extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, src/math; blam-cc: EAX a, ECX b

// blam-cc: EAX -> actor_a, ECX -> actor_b
uint8_t actor_targets_share_descriptor(datum_index actor_a, datum_index actor_b)
{
    actor *self_a, *self_b;
    int16_t *desc_a, *desc_b;

    self_a = (actor *)((uint8_t *)actor_data->data + (actor_a & 0xffff) * sizeof(actor));
    self_b = (actor *)((uint8_t *)actor_data->data + (actor_b & 0xffff) * sizeof(actor));

    desc_a = (self_a->mode == 7 || self_a->mode == 5) ? (int16_t *)&self_a->mode_data.raw[8] : (int16_t *)0;
    desc_b = (self_b->mode == 7 || self_b->mode == 5) ? (int16_t *)&self_b->mode_data.raw[8] : (int16_t *)0;

    if (desc_a == (int16_t *)0 || desc_b == (int16_t *)0) {
        return 0;
    }

    if (desc_a[0] == 0 && desc_b[0] == 0) {
        // FIXED (0x40e402 / 0x40e413): the props are each actor's current target (+0x270), not a
        //   datum inside the mode data.
        datum_index datum_a = self_a->target_unit_index;
        datum_index datum_b = self_b->target_unit_index;
        prop *prop_a = (prop *)datum_get(datum_a, prop_data);
        prop *prop_b = (prop *)datum_get(datum_b, prop_data);
        if (prop_a == (prop *)0 || prop_b == (prop *)0) return 0;
        // 0x40e429..0x40e434: EAX = second prop + 0xbc, ECX = first prop + 0xbc (orphan pass 4:
        // 0x401020 is vector3d_distance_squared, not a random roll)
        if (0.48999998f <= vector3d_distance_squared(&prop_b->last_known_position, &prop_a->last_known_position)) return 0;
    } else if (desc_a[0] == 1 && desc_b[0] == 1) {
        return desc_a[1] == desc_b[1];
    } else if (desc_a[0] != 2 || desc_b[0] != 2) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x40e380):

bool FUN_0040e380(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  uint in_ECX;
  short *psVar3;
  short *psVar4;
  int iVar5;
  float10 fVar6;
  bool local_1;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  sVar1 = *(short *)(iVar2 + 0x6c);
  iVar5 = (in_ECX & 0xffff) * 0x724;
  psVar4 = (short *)0x0;
  if ((sVar1 == 7) || (sVar1 == 5)) {
    psVar4 = (short *)(iVar2 + 0xa4);
  }
  sVar1 = *(short *)(*(int *)(DAT_00880360 + 0x34) + 0x6c + iVar5);
  psVar3 = (short *)0x0;
  if ((sVar1 == 7) || (sVar1 == 5)) {
    psVar3 = (short *)(*(int *)(DAT_00880360 + 0x34) + iVar5 + 0xa4);
  }
  local_1 = false;
  if ((psVar4 != (short *)0x0) && (psVar3 != (short *)0x0)) {
    sVar1 = *psVar4;
    if ((sVar1 == 0) && (*psVar3 == 0)) {
      iVar2 = datum_get();
      iVar5 = datum_get();
      if (iVar2 == 0) {
        return false;
      }
      if (iVar5 == 0) {
        return false;
      }
      fVar6 = (float10)FUN_00401020();
      if ((float10)0.48999998 <= fVar6) {
        return false;
      }
    }
    else {
      if ((sVar1 == 1) && (*psVar3 == 1)) {
        return psVar4[1] == psVar3[1];
      }
      if (sVar1 != 2) {
        return false;
      }
      if (*psVar3 != 2) {
        return false;
      }
    }
    local_1 = true;
  }
  return local_1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
