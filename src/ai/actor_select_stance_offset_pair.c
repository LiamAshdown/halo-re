// actor_select_stance_offset_pair  (Ghidra: actor_select_stance_offset_pair, renamed)
// address 0x4106b0, size 95 bytes
// name confidence: 0.3   rewrite confidence: 0.95 (VERIFIED against objdump 0x4106b0..0x41070e)
// evidence: phase-4 summary "selects which stance-specific offset pair (crouching,
// in-cover, leaning) to use for subsequent position math"; types/ai.h actor+0x378 is
// documented as "stance selector read by 0x4106b0" already.
// register convention: actor_index in EAX, a base pointer in EDX, two output int*
// pointers in ESI and EDI (all Ghidra unaff_/in_ registers).
// blam-cc: EAX -> actor_index, EDX -> base, ESI -> out_b, EDI -> out_a

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index, EDX -> base, ESI -> out_b, EDI -> out_a
void actor_select_stance_offset_pair(datum_index actor_index, uint8_t *base, uint8_t **out_a, uint8_t **out_b)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    *out_a = base + 0xcc;
    if (self->berserking != 0) {
        *out_b = base + 0x130;
        return;
    }
    if (self->new_target_firing_pattern != 0) {
        *out_b = base + 0x100;
        return;
    }
    if (self->moving_firing_pattern != 0) {
        *out_b = base + 0x118;
        return;
    }
    *out_b = (uint8_t *)0;
}

#if 0
Original Ghidra decompilation (0x4106b0):

void FUN_004106b0(void)

{
  int iVar1;
  char cVar2;
  uint in_EAX;
  int iVar3;
  int in_EDX;
  int *unaff_ESI;
  int *unaff_EDI;

  iVar3 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar1 = in_EDX + 0xcc;
  if (*(char *)(iVar3 + 0x378) != '\0') {
    *unaff_EDI = iVar1;
    *unaff_ESI = in_EDX + 0x130;
    return;
  }
  if (*(char *)(iVar3 + 0x600) != '\0') {
    *unaff_EDI = iVar1;
    *unaff_ESI = in_EDX + 0x100;
    return;
  }
  cVar2 = *(char *)(iVar3 + 0x601);
  *unaff_EDI = iVar1;
  if (cVar2 != '\0') {
    *unaff_ESI = in_EDX + 0x118;
    return;
  }
  *unaff_ESI = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
