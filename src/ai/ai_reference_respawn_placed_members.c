// ai_reference_respawn_placed_members  (Ghidra: ai_reference_respawn_placed_members; named for this rewrite)
// address 0x432d30, size 81 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: objdump (bin/halo.exe 0x432d30..0x432d80) recovers the true parameters: ESI is
// the packed ai reference (fed to ai_reference_actor_iterator_new exactly like every
// sibling, and also forwarded on unchanged as ai_reference_respawn_member's own packed-
// reference argument), EAX is a second packed ai reference forwarded to
// ai_reference_actor_iterator_new for the SAME iteration (Ghidra's own decompile calls it
// in_EAX). For every actor with either a controlled unit or a cluster unit, calls
// ai_reference_respawn_member with that unit (preferring unit_index, falling back to
// cluster_unit_index, and skipping the call entirely if both are none).
// register convention: confirmed by objdump: EAX -> packed_reference, ESI -> respawn_reference.
//   // blam-cc: EAX -> packed_reference, ESI -> respawn_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include "fn_ai.h"

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch


// blam-cc: EAX -> packed_reference, ESI -> respawn_reference
void ai_reference_respawn_placed_members(uint32_t packed_reference, uint32_t respawn_reference)
{
    if (respawn_reference != (uint32_t)k_datum_index_none && packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_actor_iterator iterator;
        actor *a;

        ai_reference_actor_iterator_new(packed_reference, &iterator);
        a = ai_reference_actor_iterator_next(&iterator);
        while (a != 0) {
            datum_index unit_index = a->unit_index;
            if (unit_index == (datum_index)k_datum_index_none) {
                unit_index = a->cluster_unit_index;
            }
            if (unit_index != (datum_index)k_datum_index_none) {
                ai_reference_respawn_member(respawn_reference, unit_index);
            }
            a = ai_reference_actor_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x432d30):

void FUN_00432d30(void)

{
  int in_EAX;
  int iVar1;
  int unaff_ESI;

  if ((unaff_ESI != -1) && (in_EAX != -1)) {
    FUN_00432650();
    iVar1 = FUN_004326d0();
    while (iVar1 != 0) {
      if ((*(int *)(iVar1 + 0x18) != -1) || (*(int *)(iVar1 + 0x24) != -1)) {
        FUN_00432df0();
      }
      iVar1 = FUN_004326d0();
    }
  }
  return;
}

Real disassembly (0x432d30-0x432d80), used to recover ESI, the unit_index || cluster_unit_index
skip-when-both-none case, and the arguments FUN_00432df0 (ai_reference_respawn_member) receives:

00432d56: mov    edi,[eax+0x18]      ; actor.unit_index
00432d5a: cmp    edi,0xffffffff
00432d5d: jne    0x432d68
00432d5f: mov    eax,[eax+0x24]      ; actor.cluster_unit_index
00432d62: cmp    eax,edi             ; both none -> skip the call (jump to 0x432d6f)
00432d64: je     0x432d6f
00432d66: mov    edi,eax
00432d68: mov    eax,esi             ; packed_reference == respawn_reference
00432d6a: call   0x432df0
#endif
