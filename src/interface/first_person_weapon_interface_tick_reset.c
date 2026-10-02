// first_person_weapon_interface_tick_reset  (Ghidra: FUN_004942e0, unnamed)
// address 0x4942e0, size 90 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4942e0..0x494339; offsets probed)
// evidence: out/phase4/interface_functions.md "Tears down/resets a local player's first-person
// weapon interface state and reseeds its interface timer."; types/interface.h
// first_person_weapon_interface::shutdown_countdown ("reseeded to 0x1e by 0x4942e0") is an exact
// match for this function's final store.
// register convention: local player index in AX (in_AX). // blam-cc: AX -> local_player_index
// Resolution chain (0x4942fb..0x494326): interface +8 weapon_index -> object -> definition tag data -> +0x4e4 (Weapon.more_predicted_resources),
// passed to predicted_resource_list_touch in ESI.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98
extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances; // 0x0087bc14

extern void predicted_resource_list_touch(TagReflexive *resources); // 0x4449f0, blam-cc: ESI resources

// blam-cc: AX -> local_player_index
// If local_player_index's interface currently has a weapon, touches that weapon's predicted
// resources (forcing them into their streaming caches); either way, reseeds the interface's
// shutdown countdown to 0x1e.
void first_person_weapon_interface_tick_reset(int16_t local_player_index)
{
    first_person_weapon_interface *fp = &first_person_weapon_interfaces[local_player_index];

    if (fp->weapon_index != (datum_index)0xffffffff) {
        struct object *weapon_object = *(struct object **)((char *)object_data->data + 8 +
                                                             (uint16_t)fp->weapon_index * 0xc);
        char *weapon_tag_data =
            (char *)tag_instances[(uint16_t)(*(uint32_t *)weapon_object)].data;
        predicted_resource_list_touch(&((Weapon *)weapon_tag_data)->more_predicted_resources); // 0x494326: tag +0x4e4
    }
    fp->shutdown_countdown = 0x1e;
}

#if 0
Original Ghidra decompilation (0x4942e0):

void FUN_004942e0(void)

{
  short in_AX;
  int iVar1;

  iVar1 = in_AX * 0x1ea0 + DAT_006b2d98;
  if (*(int *)(in_AX * 0x1ea0 + 8 + DAT_006b2d98) != -1) {
    predicted_resource_list_touch();
  }
  *(undefined2 *)(iVar1 + 0x12) = 0x1e;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
