// ai_reference_respawn_member  (Ghidra: ai_reference_respawn_member; named for this rewrite)
// address 0x432df0, size 134 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: objdump (bin/halo.exe 0x432df0..0x432e75) recovers the true parameters and call
// arguments Ghidra's own decompile dropped: EAX is a packed ai reference (fed straight into
// ai_reference_actor_iterator_new, 0x432650, this batch, exactly like every sibling
// function in this cluster), and EDI is a unit index the caller already resolved, passed
// through unchanged to actor_find_or_create_shared_prop (outside this rewrite's range). For every actor the
// reference names, marks its encounter's encounter+0xe (an ai.h unknown_0e byte pair) with
// 0x96 and reactivates the squad (encounter_activate, 0x437710, this batch) once, then calls
// actor_find_or_create_shared_prop(unit_index, actor_index, 1, 0) and, on success, actor_squad_react_to_grenade
// on the result.
// register convention: confirmed by objdump: EAX -> packed_reference, EDI -> unit_index.
//   // blam-cc: EAX -> packed_reference, EDI -> unit_index
//
// UNSURE: encounter.unknown_0e (types/ai.h has no name for it beyond "unknown_0e[2]") and
// the exact roles of actor_find_or_create_shared_prop's four arguments are not established beyond their values.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *encounter_data; // 0x008802c8

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern void encounter_activate(datum_index encounter_index); // 0x437710, this batch; blam-cc: ECX -> encounter_index, UNSURE
extern datum_index actor_find_or_create_shared_prop(datum_index unit_index, datum_index actor_index, int32_t flag_a, int32_t flag_b); // 0x43eb30, outside this rewrite's range, UNSURE signature
extern void actor_squad_react_to_grenade(datum_index actor_index, datum_index target_prop_index, int16_t grenade_type);
    // 0x42a3a0, ESI actor, stack target_prop, EAX grenade_type

// blam-cc: EAX -> packed_reference, EDI -> unit_index
void ai_reference_respawn_member(uint32_t packed_reference, datum_index unit_index)
{
    if (packed_reference != (uint32_t)k_datum_index_none && unit_index != (datum_index)k_datum_index_none) {
        ai_reference_actor_iterator iterator;
        actor *a;

        ai_reference_actor_iterator_new(packed_reference, &iterator);
        a = ai_reference_actor_iterator_next(&iterator);
        while (a != 0) {
            datum_index respawned;

            if (a->encounter_index != (datum_index)k_datum_index_none) {
                encounter *enc = &((encounter *)encounter_data->data)[a->encounter_index & 0xffff];
                *(int16_t *)&enc->activation_delay = 0x96; // UNSURE: see file header
                encounter_activate(a->encounter_index);
            }

            respawned = actor_find_or_create_shared_prop(unit_index, iterator.actor_index, 1, 0);
            if (respawned != (datum_index)k_datum_index_none) {
                // 0x432e56..0x432e5c: ESI = the actor ([esp+0x14], also the shared-prop call's actor), push the prop,
                // EAX = 3
                actor_squad_react_to_grenade(iterator.actor_index, respawned, 3);
            }

            a = ai_reference_actor_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x432df0):

void FUN_00432df0(void)

{
  int in_EAX;
  int iVar1;
  int unaff_EDI;
  undefined4 local_8;

  if ((in_EAX != -1) && (unaff_EDI != -1)) {
    FUN_00432650();
    iVar1 = FUN_004326d0();
    while (iVar1 != 0) {
      if (*(uint *)(iVar1 + 0x34) != 0xffffffff) {
        *(undefined2 *)
         ((*(uint *)(iVar1 + 0x34) & 0xffff) * 0x6c + 0xe + *(int *)(DAT_008802c8 + 0x34)) = 0x96;
        squad_activate();
      }
      iVar1 = FUN_0043eb30(local_8,1,0);
      if (iVar1 != -1) {
        actor_squad_react_to_grenade(iVar1);
      }
      iVar1 = FUN_004326d0();
    }
  }
  return;
}

Real disassembly (0x432df0-0x432e75), used to recover EDI and the FUN_0043eb30 arguments:

00432e3e: mov    esi,[esp+0x14]     ; iterator.actor_index
00432e42: push   0x0
00432e44: push   0x1
00432e46: push   esi
00432e47: mov    eax,edi            ; unit_index, unmodified since function entry
00432e49: call   0x43eb30
#endif
