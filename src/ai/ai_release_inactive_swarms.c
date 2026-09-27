// ai_release_inactive_swarms  (Ghidra: ai_release_inactive_swarms, already named)
// address 0x42abd0, size 178 bytes
// name confidence: 0.9   rewrite confidence: 0.5
// evidence: cea-pdb string match ("%d swarm units"). types/ai.h actor.swarm(0x06)/
//   active(0x08)/unknown_0c(0x0c, "actor_new sets none", a datum_index -- here compared
//   against -1 to gate release). Calls actor_delete_or_release_unit (0x4288e0, already
//   rewritten in this module) and actor_iterator_next, whose dropped iterator-state argument
//   is already recovered in src/ai/ai_mark_recognized_objects_for_reaction.c (reused here
//   verbatim rather than re-derived).
//   UNSURE: `is_dead` (Ghidra's untraced local_8, only meaningful when ai_globals.actors_valid
//   was already false at entry) matches actor_delete_or_release_unit's own untraced flag;
//   modeled the same way, but not independently re-verified with objdump for this call site.
// register convention: __cdecl, buffer and has_more on the stack.
//   // blam-cc: stack -> buffer, has_more

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>
#include <stdio.h>

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *encounter_data; // 0x008802c8
extern data_array *actor_data;     // 0x00880360

extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70
extern void actor_delete_or_release_unit(datum_index actor_index, uint8_t is_dead); // 0x4288e0
extern int _sprintf(char *buffer, const char *format, ...); // 0x623693

// blam-cc: stack -> buffer, has_more
// Iterates every active actor as part of AI global cleanup, releasing each currently
// inactive swarm's units (accumulating its component count into a "%d swarm units" status
// string) and deleting the rest; reports whether any were released.
int ai_release_inactive_swarms(char *buffer, uint8_t *has_more)
{
    actor_iterator_state iterator;
    actor *a;
    uint8_t is_dead = 0;
    int16_t total = 0;

    if (ai_globals_ptr->actors_valid) {
        is_dead = 0xff; // matches the original's local_8 = 0xffffffff, truncated to the byte
                         // parameter; see UNSURE above
    }

    iterator.filter_array = encounter_data;
    iterator.unknown_04 = 0;
    iterator.cursor = -1;
    iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ 0x69746572;
    iterator.unknown_10 = 0;
    iterator.active = 1;
    iterator.actor_index = -1;
    iterator.unknown_18 = -1;

    a = actor_iterator_next(&iterator);
    while (a != 0) {
        if (a->swarm != 0 && a->active == 0 && a->unknown_0c != (datum_index)k_datum_index_none) {
            datum_index actor_index = iterator.actor_index /* the full handle, salt included */;
            total = total + a->cluster_count;
            actor_delete_or_release_unit(actor_index, is_dead);
        }
        a = actor_iterator_next(&iterator);
    }

    _sprintf(buffer, "%d swarm units", (int)total);
    *has_more = 0;
    return total > 0;
}

#if 0
Original Ghidra decompilation (0x42abd0):

int __cdecl ai_release_inactive_swarms(char *buffer,uchar *has_more)

{
  int iVar1;
  short local_20;
  undefined4 local_8;

  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_8 = 0xffffffff;
  }
  iVar1 = actor_iterator_next();
  local_20 = 0;
  while (iVar1 != 0) {
    if (((*(char *)(iVar1 + 6) != '\0') && (*(char *)(iVar1 + 8) == '\0')) &&
       (*(int *)(iVar1 + 0xc) != -1)) {
      local_20 = local_20 + *(short *)(iVar1 + 0x1e);
      actor_delete_or_release_unit(local_8);
    }
    iVar1 = actor_iterator_next();
  }
  _sprintf(buffer,"%d swarm units",(int)local_20);
  *has_more = '\0';
  return (uint)(0 < local_20);
}
#endif
