// actor_get_current_mode_combat_grade  (Ghidra: actor_get_current_mode_combat_grade, renamed)
// address 0x40e760, size 36 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: phase-4 summary "returns a per-mode property value from the mode definition
// table for the actor's currently active mode"; types/ai.h actor_mode_definition.combat_grade.
// register convention: actor_index in EAX (Ghidra's in_EAX).
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;                            // 0x00880360
extern actor_mode_definition actor_mode_definitions[16];  // 0x00655254

// blam-cc: EAX -> actor_index
int16_t actor_get_current_mode_combat_grade(datum_index actor_index)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    return actor_mode_definitions[self->mode].combat_grade;
}

#if 0
Original Ghidra decompilation (0x40e760):

undefined4 FUN_0040e760(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = *(short *)((in_EAX & 0xffff) * 0x724 + 0x6c + *(int *)(DAT_00880360 + 0x34)) * 0x38;
  return CONCAT22((short)((uint)iVar1 >> 0x10),*(undefined2 *)(&DAT_00655258 + iVar1));
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
