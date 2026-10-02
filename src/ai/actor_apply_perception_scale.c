// actor_apply_perception_scale  (Ghidra: actor_apply_perception_scale, renamed)
// address 0x42aa90, size 99 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: types/ai.h actor.perception_scale (0x69c, "0x42aa90 scales hearing and awareness
//   ranges by this"), actor.unknown_1ca. Phase-4 summary: "Applies a per-actor
//   perception-scaling factor to a caller-provided float (such as a hearing or awareness
//   range), based on a zone flag and an actor state byte."
//   UNSURE: `zone` is a caller-owned record whose byte at +4 bit 0x8 selects whether
//   perception_scale applies; no struct is established for it here.
// register convention: EAX -> actor_index, stack -> zone (a caller record, +4 tested for bit
//   0x8), EDX -> in_out_value (float*).
//   // blam-cc: EAX -> actor_index, stack -> zone, EDX -> in_out_value

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index, stack -> zone, EDX -> in_out_value
// Scales *in_out_value by the actor's perception_scale when the actor is valid and the zone
// record allows it (flag bit 0x8), then unconditionally applies a further 0.3x scale when
// actor.unknown_1ca is set. Returns whether either scale was applied.
uint8_t actor_apply_perception_scale(datum_index actor_index, const uint8_t *zone, float *in_out_value)
{
    uint8_t scaled = 0;

    if (actor_index == (datum_index)k_datum_index_none) {
        return 0;
    }

    {
        actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

        if ((zone[4] & 8) != 0) {
            if (self->perception_scale > 0.0f) {
                scaled = 1;
                *in_out_value = *in_out_value * self->perception_scale;
            }
        }
        if (self->playfight != 0) {
            *in_out_value = *in_out_value * 0.3f;
            return 1;
        }
    }
    return scaled;
}

#if 0
Original Ghidra decompilation (0x42aa90):

undefined1 FUN_0042aa90(int param_1)

{
  uint in_EAX;
  int iVar1;
  float *in_EDX;
  undefined1 uVar2;

  uVar2 = 0;
  if (in_EAX != 0xffffffff) {
    iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    uVar2 = 0;
    if ((*(byte *)(param_1 + 4) & 8) != 0) {
      if (0.0 < *(float *)(iVar1 + 0x69c)) {
        uVar2 = 1;
        *in_EDX = *in_EDX * *(float *)(iVar1 + 0x69c);
      }
    }
    if (*(char *)(iVar1 + 0x1ca) != '\0') {
      *in_EDX = *in_EDX * 0.3;
      return 1;
    }
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
