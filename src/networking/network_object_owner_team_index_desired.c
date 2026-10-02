// network_object_owner_team_index_desired  (Ghidra: FUN_004e0cf0, unnamed)
// address 0x4e0cf0, size 63 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Looks up the player datum referenced by the
// object's stored index and returns a byte field (likely team or slot id) from it, or -1 if
// unavailable." `object+0xc` is types/objects.h's object::unknown_00c, here read as a plain
// uint16 index (not a validated datum_index) into machine_to_player; the returned byte at
// player+0x67 matches types/game.h player::team_index_desired exactly.
// register convention: EAX = object (object *).
// blam-cc: EAX -> object
// UNSURE: this reveals object::unknown_00c is (at least sometimes) used as a raw
// machine_to_player index rather than a general datum_index; not renamed in types/objects.h.
// UNSURE: `&DAT_006b1460 + index != NULL` in the original is a tautology (the table's address
// plus a small index is never NULL); preserved as a literal check.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern datum_index machine_to_player[16]; // 0x006b1460
extern data_array *player_data; // 0x0087a480
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, memory module

// Returns the desired team index of the player currently occupying object->unknown_00c's
// machine slot, or -1 if the slot is empty, out of range, or the player datum is not live.
int32_t network_object_owner_team_index_desired(object *obj)
{
    uint16_t slot;
    datum_index resolved;
    player *plr;

    slot = *(uint16_t *)&((struct object *)obj)->network_update_tick;
    if (slot != 0xffff && &machine_to_player[slot] != 0 && machine_to_player[slot] != (datum_index)0xffffffff) {
        resolved = machine_to_player[slot];
        plr = (player *)datum_get(resolved, player_data);
        if (plr != 0) {
            return (int32_t)plr->team_index_desired;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4e0cf0):

int FUN_004e0cf0(void)

{
  int in_EAX;
  int iVar1;

  if (((*(ushort *)(in_EAX + 0xc) != 0xffff) &&
      (&DAT_006b1460 + *(ushort *)(in_EAX + 0xc) != (int *)0x0)) &&
     ((&DAT_006b1460)[*(ushort *)(in_EAX + 0xc)] != -1)) {
    iVar1 = datum_get();
    if (iVar1 != 0) {
      return (int)*(char *)(iVar1 + 0x67);
    }
  }
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
