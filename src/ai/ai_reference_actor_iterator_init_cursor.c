// ai_reference_actor_iterator_init_cursor  (Ghidra: ai_reference_actor_iterator_init_cursor; named for this rewrite)
// address 0x4369f0, size 61 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: tail-called from ai_reference_actor_iterator_new (0x432650, this batch) with
// ECX advanced by 0xc (confirmed by objdump, `add ecx,0xc; jmp 0x4369f0`), so its own
// in_ECX[0..2] are types/ai.h ai_reference_actor_iterator's bytes 0xc, 0x10 (actor_index)
// and 0x14. Rewritten here taking the iterator itself plus the already-resolved encounter
// index, operating on those same absolute offsets, to avoid reproducing the raw pointer
// adjustment. types/ai.h ai_globals.unknown_08 is already documented as "the head of the
// unassigned actor list", which is exactly what this stores at +0x14 when encounter_index
// is none.
// register convention: confirmed by objdump: EAX -> encounter_index, ECX -> iterator + 0xc.
//   // blam-cc: EAX -> encounter_index, ECX -> cursor
//
// UNSURE: the field this writes at iterator+0xc (called unknown_0c here, since
// ai_reference_actor_iterator only names actor_index at +0x10) is never read back by
// ai_reference_actor_iterator_next (0x4326d0, this batch); its purpose is not established.

// FIXED (verified against the retail bytes and its callers): ECX is a 12-byte actor cursor
//   {encounter_index, actor_index, next_actor}, not the iterator base. Five original callers pass a
//   bare 12-byte local (`lea ecx,[esp+0x24]; call 0x4369f0`), and ai_reference_actor_iterator_new
//   passes iterator+0xc. The draft added the +0xc itself, so every original caller had 12 bytes
//   written past its cursor, over its saved registers and return address (in game: a crash inside
//   path_find_run with EBP and EIP overwritten).
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *encounter_data; // 0x008802c8
extern ai_globals *ai_globals_ptr; // 0x00880354

// blam-cc: EAX -> encounter_index, ECX -> cursor
// cursor[0] = encounter index (-1 for the unassigned actors), cursor[1] = the actor last returned
// (reset to none), cursor[2] = the next actor to visit: the encounter's first actor, or the head of
// the unassigned actor list.
void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor)
{
    if (ai_globals_ptr->actors_valid == 0) {
        return;
    }

    cursor[0] = (datum_index)encounter_index;
    cursor[1] = (datum_index)k_datum_index_none;

    if (encounter_index == -1) {
        cursor[2] = ai_globals_ptr->unknown_08;
        return;
    }

    cursor[2] = ((encounter *)encounter_data->data)[(uint32_t)encounter_index & 0xffff].first_actor;
}

#if 0
Original Ghidra decompilation (0x4369f0):

void FUN_004369f0(void)

{
  int iVar1;
  uint in_EAX;
  uint *in_ECX;

  iVar1 = DAT_00880354;
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    *in_ECX = in_EAX;
    in_ECX[1] = 0xffffffff;
    if (in_EAX == 0xffffffff) {
      in_ECX[2] = *(uint *)(iVar1 + 8);
      return;
    }
    in_ECX[2] = *(uint *)((in_EAX & 0xffff) * 0x6c + 0x14 + *(int *)(DAT_008802c8 + 0x34));
  }
  return;
}

Real disassembly (0x4369f0-0x436a2d), and the caller-side pointer adjustment at 0x432650
that fixes this function's true field offsets:

004326b6:	pop    edi
004326b7:	add    ecx,0xc          ; ecx now points at iterator + 0xc
004326ba:	pop    esi
004326bb:	jmp    0x4369f0         ; tail call, EAX = encounter_index
#endif
