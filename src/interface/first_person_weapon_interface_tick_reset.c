// first_person_weapon_interface_tick_reset  (Ghidra: FUN_004942e0, unnamed)
// address 0x4942e0, size 90 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4942e0..0x494339; offsets probed)
// evidence: out/phase4/interface_functions.md "Tears down/resets a local player's first-person
// weapon interface state and reseeds its interface timer."; types/interface.h
// first_person_weapon_interface::shutdown_countdown ("reseeded to 0x1e by 0x4942e0") is an exact
// match for this function's final store.
// register convention: local player index in AX (in_AX). // blam-cc: AX -> local_player_index
// UNSURE: object_data (0x008603b0) and tag_instances (0x0087bc14) are both referenced by this
// function per the pack tool's global scan, but Ghidra's decompile shows the
// predicted_resource_list_touch call with zero visible arguments and no visible tag lookup at
// all -- meaning the weapon-object-to-tag-data resolution (the same pattern used throughout this
// module) almost certainly happens here too, but was fully elided. The TagReflexive this passes
// (presumably the weapon tag's own "predicted resources" list) could not be pinned to a real
// offset within the Weapon tag in this batch; called with an explicit placeholder offset of 0,
// clearly wrong structurally, rather than omitting the call and silently changing behavior.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98
extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances; // 0x0087bc14

extern void predicted_resource_list_touch(TagReflexive *resources); // 0x4449f0

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
        predicted_resource_list_touch((TagReflexive *)(weapon_tag_data + 0x4e4)); // 0x494326: Weapon predicted resources
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
