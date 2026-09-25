// equipment_network_baseline_take  (Ghidra: missed_4bc070, created by hand this pass -- Ghidra
// never recovered it as a function; only reachable through the equipment object_type_definition
// row)
// address 0x4bc070, size 118 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: the equipment row (0x0069b810) carries this address at +0x68, the same column that
//   holds projectile_network_baseline_take (0x4c0ed0, "+0x68 projectile_network_baseline_take")
//   and weapon_network_baseline_take (0x4c5e80, this batch); all three snapshot the live
//   object.position/velocity(/angular_velocity) into the type's own replicated network_state and
//   bump the baseline index. types/items.h equipment_data (network_baseline_index 0x245,
//   network_state_valid 0x244, network_sequence 0x246, network_state 0x248),
//   equipment_network_state (position 0x00, velocity 0x0c, angular_velocity 0x18);
//   types/objects.h object.position (0x05c), object.velocity (0x068), object.angular_velocity
//   (0x08c). Callee object_try_and_get (0x4f6ec0), mask _object_mask_equipment (0x008).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0

// Takes a fresh network baseline for an equipment item: bumps network_baseline_index, snapshots
// the object's current position, velocity and angular_velocity into equipment_data.network_state,
// marks the state valid and resets the sequence counter to 0.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> item_index
void equipment_network_baseline_take(uint32_t item_index)
{
    object *obj = object_try_and_get(item_index, _object_mask_equipment);

    if (obj != 0) {
        equipment_data *ed = (equipment_data *)((uint8_t *)obj + k_item_extension_offset);

        ed->network_baseline_index++;
        ed->network_state.position = obj->position;
        ed->network_state.velocity = obj->velocity;
        ed->network_state_valid = 1;
        ed->network_sequence = 0;
        ed->network_state.angular_velocity = obj->angular_velocity;
    }
}

#if 0
Original Ghidra decompilation (0x4bc070):

void missed_4bc070(void)

{
  int iVar1;

  iVar1 = object_try_and_get(8);
  if (iVar1 != 0) {
    *(char *)(iVar1 + 0x245) = *(char *)(iVar1 + 0x245) + '\x01';
    *(undefined4 *)(iVar1 + 0x248) = *(undefined4 *)(iVar1 + 0x5c);
    *(undefined4 *)(iVar1 + 0x24c) = *(undefined4 *)(iVar1 + 0x60);
    *(undefined4 *)(iVar1 + 0x250) = *(undefined4 *)(iVar1 + 100);
    *(undefined4 *)(iVar1 + 0x254) = *(undefined4 *)(iVar1 + 0x68);
    *(undefined4 *)(iVar1 + 600) = *(undefined4 *)(iVar1 + 0x6c);
    *(undefined4 *)(iVar1 + 0x25c) = *(undefined4 *)(iVar1 + 0x70);
    *(undefined1 *)(iVar1 + 0x244) = 1;
    *(undefined1 *)(iVar1 + 0x246) = 0;
    *(undefined4 *)(iVar1 + 0x260) = *(undefined4 *)(iVar1 + 0x8c);
    *(undefined4 *)(iVar1 + 0x264) = *(undefined4 *)(iVar1 + 0x90);
    *(undefined4 *)(iVar1 + 0x268) = *(undefined4 *)(iVar1 + 0x94);
  }
  return;
}
#endif
