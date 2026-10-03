// actor_notify_weapon_pickup_once  (Ghidra: actor_notify_weapon_pickup_once; named for this rewrite)
// address 0x42c370, size 97 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: phase-4 summary ("fires a one-shot AI communication event (id 0x25) tied to a
// unit's current weapon, guarded by an 'already notified' flag").
// register convention: ECX -> object_index (in_ECX, the only register Ghidra's own decompile
// surfaces).
// blam-cc: ECX -> object_index
//
// UNSURE: ai_communication_broadcast is shown taking only its event_code here; the other six
// arguments are modeled the same way this module's other single-argument broadcast call
// sites do it (actor_gate_jump_traversal.c), with object arguments set to "none".
// UNSURE: the flag is unconditionally cleared to 0 regardless of which branch runs, so it
// never actually latches from inside this function alone -- something else must set it to 1
// between calls for the "already notified" guard to do anything. Reproduced exactly as
// decompiled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern data_array *actor_data;  // 0x00880360

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340

// blam-cc: ECX -> object_index
// If object_index's controlling actor has not yet been notified of a weapon pickup event
// (actor.unknown_38c is clear), broadcasts communication event 0x25. Always clears the flag
// afterward.
void actor_notify_weapon_pickup_once(datum_index object_index)
{
    object *obj;
    unit_data *unit;
    datum_index actor_index;
    actor *a;

    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    actor_index = unit->actor_index;
    if (actor_index != (datum_index)k_datum_index_none) {
        a = &((actor *)actor_data->data)[actor_index & 0xffff];
        if (a->vehicle_exit_forced == 0) {
            ai_communication_broadcast(0x25, (datum_index)k_datum_index_none,
                                        (datum_index)k_datum_index_none, 0,
                                        (datum_index)k_datum_index_none,
                                        (datum_index)k_datum_index_none, 0);
        }
        a->vehicle_exit_forced = 0;
    }
}

#if 0
Original Ghidra decompilation (0x42c370):

void FUN_0042c370(void)

{
  uint uVar1;
  int iVar2;
  uint in_ECX;

  uVar1 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc) + 500);
  if (uVar1 != 0xffffffff) {
    iVar2 = (uVar1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    if (*(char *)(iVar2 + 0x38c) == '\0') {
      ai_communication_broadcast(0x25);
    }
    *(undefined1 *)(iVar2 + 0x38c) = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
