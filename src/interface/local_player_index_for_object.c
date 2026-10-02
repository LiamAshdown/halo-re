// local_player_index_for_object  (Ghidra: FUN_004926f0, renamed per types/interface.h)
// address 0x4926f0, size 62 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase4/interface_types_notes.md first_person_weapon_interface note:
// "local_player_index_for_object @0x4926f0 matches against [weapon_index]"; loop bound `< 1`
// matches every other single-local-player loop in this module (retail PC has one entry).
// register convention: object_index in ESI (unaff_ESI). // blam-cc: object_index=ESI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98

// Finds which local player's first-person weapon interface currently has object_index attached
// as its weapon, returning -1 if none matches.
int32_t local_player_index_for_object(datum_index object_index)
{
    int32_t i;

    for (i = 0; i < 1; i++) {
        if (first_person_weapon_interfaces[i].weapon_index == object_index &&
            first_person_weapon_interfaces[i].attached != 0) {
            break;
        }
    }
    if (i == 1) {
        return -1;
    }
    return i;
}

#if 0
Original Ghidra decompilation (0x4926f0):

int FUN_004926f0(void)

{
  short sVar1;
  int unaff_ESI;

  sVar1 = 0;
  do {
    if ((*(int *)(sVar1 * 0x1ea0 + 8 + DAT_006b2d98) == unaff_ESI) &&
       (*(char *)(sVar1 * 0x1ea0 + DAT_006b2d98) != '\0')) break;
    sVar1 = sVar1 + 1;
  } while (sVar1 < 1);
  if (sVar1 == 1) {
    return -1;
  }
  return (int)sVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
