// projectile_network_baseline_take  (Ghidra: missed_4c0ed0, created by hand this pass -- Ghidra
// never recovered it as a function; only reachable through the projectile
// object_type_definition row)
// address 0x4c0ed0, size 90 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: named already in out/phase4/projectiles_types_notes.md ("+0x68
//   projectile_network_baseline_take -- missed"; "network_baseline_index ... 0x27a ... 0x4c0ed0
//   increments"; "network_sequence ... 0x27b ... 0x4c0ed0 ... zero it"; "network_state (0x18)
//   ... 0x4c0ed0 fills it from object.position/object.velocity"; "network_state_valid ... 0x279
//   ... 0x4c0ed0 set 1"). types/projectiles.h projectile_data (network_baseline_index 0x27a,
//   network_state_valid 0x279, network_sequence 0x27b, network_state 0x27c),
//   projectile_network_state (position 0x00, velocity 0x0c); types/objects.h object.position
//   (0x05c), object.velocity (0x068). Callee object_try_and_get (0x4f6ec0), mask
//   _object_mask_projectile (0x020).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0

// Takes a fresh network baseline for a projectile: bumps network_baseline_index, snapshots the
// object's current position and velocity into projectile_data.network_state, marks the state
// valid and resets the sequence counter to 0.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> object_index
void projectile_network_baseline_take(uint32_t object_index)
{
    object *obj = object_try_and_get(object_index, _object_mask_projectile);

    if (obj != 0) {
        projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);

        proj->network_baseline_index++;
        proj->network_state.position = obj->position;
        proj->network_state_valid = 1;
        proj->network_sequence = 0;
        proj->network_state.velocity = obj->velocity;
    }
}

#if 0
Original Ghidra decompilation (0x4c0ed0):

void missed_4c0ed0(void)

{
  int iVar1;

  iVar1 = object_try_and_get(0x20);
  if (iVar1 != 0) {
    *(char *)(iVar1 + 0x27a) = *(char *)(iVar1 + 0x27a) + '\x01';
    *(undefined4 *)(iVar1 + 0x27c) = *(undefined4 *)(iVar1 + 0x5c);
    *(undefined4 *)(iVar1 + 0x280) = *(undefined4 *)(iVar1 + 0x60);
    *(undefined4 *)(iVar1 + 0x284) = *(undefined4 *)(iVar1 + 100);
    *(undefined1 *)(iVar1 + 0x279) = 1;
    *(undefined1 *)(iVar1 + 0x27b) = 0;
    *(undefined4 *)(iVar1 + 0x288) = *(undefined4 *)(iVar1 + 0x68);
    *(undefined4 *)(iVar1 + 0x28c) = *(undefined4 *)(iVar1 + 0x6c);
    *(undefined4 *)(iVar1 + 0x290) = *(undefined4 *)(iVar1 + 0x70);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
