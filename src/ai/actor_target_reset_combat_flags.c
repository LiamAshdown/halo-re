// actor_target_reset_combat_flags  (Ghidra: actor_target_reset_combat_flags, already named)
// address 0x41baf0, size 55 bytes
// name confidence: 0.5   rewrite confidence: 1.0
// evidence: out/phase2/results/ai_02.json -- clears target-data flags at +0xb9/+0xba/+0xbb and
//   sets +0x64=1, then calls actor_queue_sighted_target_dialogue; identical tail sequence appears inside
//   actor_target_data_release (0x41b980). +0xb9/+0xba/+0xbb are prop.noticed_a/b/c and +0x64
//   is prop.combat_dirty in types/ai.h.
// register convention: the prop record pointer is only visible as in_ECX (an already-scaled
//   prop index, i.e. the low 16 bits of a datum_index). Mapped to the first parameter (ECX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *prop_data; // 0x008802c0

extern void actor_queue_sighted_target_dialogue(datum_index actor_index, datum_index target_prop_index,
    uint8_t already_noticed); // 0x421c20, stack

// REWRITTEN from objdump 0x41baf0..0x41bb26. ECX: prop; stack: (actor, a slot the function overwrites with the prop,
//   already_noticed). Clears the prop's three notice flags (+0xb9..+0xbb), marks +0x64, then tail-calls
//   actor_queue_sighted_target_dialogue(actor, prop, already_noticed). The draft had no actor and called the
//   dialogue queue without arguments.
// blam-cc: ECX -> target_prop_index, stack -> actor_index, unused, already_noticed
void actor_target_reset_combat_flags(datum_index target_prop_index, datum_index actor_index, uint32_t unused,
    uint8_t already_noticed)
{
    uint8_t *p = (uint8_t *)prop_data->data + (target_prop_index & 0xffff) * 0x138;

    (void)unused;
    p[0xba] = 0;
    p[0xb9] = 0;
    p[0xbb] = 0;
    p[0x64] = 1;
    actor_queue_sighted_target_dialogue(actor_index, target_prop_index, already_noticed);
}

#if 0
Original Ghidra decompilation (0x41baf0):

void actor_target_reset_combat_flags(void)

{
  int iVar1;
  uint in_ECX;

  iVar1 = (in_ECX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  *(undefined1 *)(iVar1 + 0xba) = 0;
  *(undefined1 *)(iVar1 + 0xb9) = 0;
  *(undefined1 *)(iVar1 + 0xbb) = 0;
  *(undefined1 *)(iVar1 + 100) = 1;
  FUN_00421c20();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
