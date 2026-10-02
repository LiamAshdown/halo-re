// weapon_network_baseline_take  (Ghidra: missed_4c5e80, created by hand this pass -- Ghidra never
// recovered it as a function; only reachable through the weapon object_type_definition row)
// address 0x4c5e80, size 131 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: the weapon row (0x0069b748) carries this address at +0x68, the same column that
//   holds projectile_network_baseline_take (0x4c0ed0, "+0x68 projectile_network_baseline_take")
//   and equipment_network_baseline_take (0x4bc070, this batch). types/items.h weapon_data
//   (network_baseline_index 0x2e1, network_state_valid 0x2e0, network_sequence 0x2e2,
//   network_state 0x2e4, age 0x240), weapon_network_state (position 0x00, velocity 0x0c,
//   rounds_unloaded[2] 0x24, age 0x28), weapon_magazine_state.rounds_unloaded (0x06);
//   types/objects.h object.position (0x05c), object.velocity (0x068). Callee object_try_and_get
//   (0x4f6ec0), mask _object_mask_weapon (0x004).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0

// Takes a fresh network baseline for a weapon: bumps network_baseline_index, snapshots the
// object's current position and velocity, both magazines' rounds_unloaded and the weapon's age
// into weapon_data.network_state, marks the state valid and resets the sequence counter to 0.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> item_index
void weapon_network_baseline_take(uint32_t item_index)
{
    object *obj = object_try_and_get(item_index, _object_mask_weapon);

    if (obj != 0) {
        weapon_data *wd = (weapon_data *)((uint8_t *)obj + k_item_extension_offset);

        wd->network_baseline_index++;
        wd->network_state.position = obj->position;
        wd->network_state.velocity = obj->velocity;
        wd->network_state.rounds_unloaded[0] = wd->magazines[0].rounds_unloaded;
        wd->network_state_valid = 1;
        wd->network_sequence = 0;
        wd->network_state.rounds_unloaded[1] = wd->magazines[1].rounds_unloaded;
        wd->network_state.age = wd->age;
    }
}

#if 0
Original Ghidra decompilation (0x4c5e80):

void missed_4c5e80(void)

{
  int iVar1;

  iVar1 = object_try_and_get(4);
  if (iVar1 != 0) {
    *(char *)(iVar1 + 0x2e1) = *(char *)(iVar1 + 0x2e1) + '\x01';
    *(undefined4 *)(iVar1 + 0x2e4) = *(undefined4 *)(iVar1 + 0x5c);
    *(undefined4 *)(iVar1 + 0x2e8) = *(undefined4 *)(iVar1 + 0x60);
    *(undefined4 *)(iVar1 + 0x2ec) = *(undefined4 *)(iVar1 + 100);
    *(undefined4 *)(iVar1 + 0x2f0) = *(undefined4 *)(iVar1 + 0x68);
    *(undefined4 *)(iVar1 + 0x2f4) = *(undefined4 *)(iVar1 + 0x6c);
    *(undefined4 *)(iVar1 + 0x2f8) = *(undefined4 *)(iVar1 + 0x70);
    *(undefined2 *)(iVar1 + 0x308) = *(undefined2 *)(iVar1 + 0x2b6);
    *(undefined1 *)(iVar1 + 0x2e0) = 1;
    *(undefined1 *)(iVar1 + 0x2e2) = 0;
    *(undefined2 *)(iVar1 + 0x30a) = *(undefined2 *)(iVar1 + 0x2c2);
    *(undefined4 *)(iVar1 + 0x30c) = *(undefined4 *)(iVar1 + 0x240);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
