// actor_target_get_backup_priority  (Ghidra: actor_target_get_backup_priority; described as
//   unit_get_combat_priority in out/phase2/results/ai_02.json but the record read is prop,
//   addressed through prop_data with the module's 0x138 stride)
// address 0x420e50, size 106 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase2/results/ai_02.json (offsets reinterpreted against types/ai.h's prop):
//   for a prop in active combat (kind 2-3) with the engaged flag set, returns tiered priority
//   values: 4 if fleeing (+0x74), 2 or 3 if crouched (+0x12f, based on +0x122<2), 1 if
//   +0x32>1, else 0; used by actor_scan_allies_for_backup_request (0x420ec0) to rank which
//   allies most need help.
// register convention: EAX -> target_prop_index; no other register operands are read.
//
// UNSURE: prop+0x74 is named `seen` in types/ai.h on stronger evidence
// (actor_target_reset_seen_flags); phase2's "fleeing" reading for this call site is kept only
// as a behavioral comment, the header field is what is actually accessed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0

// blam-cc: EAX -> target_prop_index
// Returns a small integer priority ranking how urgently a prop (target-data record) needs
// backup/assistance based on its current combat sub-state.
uint8_t actor_target_get_backup_priority(datum_index target_prop_index)
{
    prop *target;

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

    if (1 < target->kind && target->kind < 4 && target->engaged != 0) {
        if (target->seen != 0) {
            return 4;
        }
        if (target->unknown_12f != 0) {
            return (uint8_t)((target->unknown_122 < 2) + 2);
        }
        if (1 < target->unknown_32) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x420e50):

char FUN_00420e50(void)

{
  char cVar1;
  uint in_EAX;
  int iVar2;

  iVar2 = (in_EAX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  cVar1 = '\0';
  if (((1 < *(short *)(iVar2 + 0x24)) && (*(short *)(iVar2 + 0x24) < 4)) &&
     (*(char *)(iVar2 + 0xa4) != '\0')) {
    if (*(char *)(iVar2 + 0x74) != '\0') {
      return '\x04';
    }
    if (*(char *)(iVar2 + 0x12f) != '\0') {
      return (*(char *)(iVar2 + 0x122) < '\x02') + '\x02';
    }
    if (1 < *(short *)(iVar2 + 0x32)) {
      cVar1 = '\x01';
    }
  }
  return cVar1;
}
#endif
