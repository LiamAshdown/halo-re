// game_engine_location_blocked_by_vehicle  (Ghidra: FUN_00461e60)
// address 0x461e60, size 186 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// RENAMED and CORRECTED by review (was game_engine_find_nearby_vehicle, with no arguments).
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x461e60
//   --stop-address=0x461f1a bin/halo.exe) and against its ONE caller,
//   player_pick_random_starting_location (0x4776d0), which reaches it as
//     mov edx,edi ; call 0x461e60
//   with EDI = "scenario->player_starting_locations.pointer + i * 0x34". So EDX is an incoming
//   argument -- the candidate location, used as a real_point3d * because
//   ScenarioPlayerStartingLocation::position is that record's first field -- and the first pass's
//   reading of EDX as a second (high-dword) return value out of 0x5013a0 was wrong: at 0x461eaa
//   the "push edx" that feeds object_find_in_sphere's `center` is the untouched incoming EDX.
//   0x5013a0 is called with EAX = 0 and ECX = the global at 0x00746f90 and contributes only the
//   leaf index in EAX, which the caller masks with 0x7fffffff before indexing the cluster table.
//   The search is object_find_in_sphere(0, 0x11f, &location_pair, point, 0.1f, out, 0x10) -- a
//   0.1 world-unit sphere, i.e. "is something already sitting exactly on this spawn point".
//   types/objects.h object::type (+0xb4, 1 == _object_type_vehicle);
//   src/objects/damage_apply_area_effect.c's object_find_in_sphere signature.
// register convention: EDX -> point. Returns a bool in AL only.
//   // blam-cc: EDX -> point
// reconciled: R06 global_matg_multiplayer (0x00746f9c) -> ScenarioStructureBSP *global_structure_bsp; the 0x00746f90 extern that was misnamed global_structure_bsp becomes global_collision_bsp (R05 name) to free the name
// reconciled: R05 global_collision_bsp takes its agreed type ModelCollisionGeometryBSP * (was uint8_t *) and is passed to bsp3d_node_find_leaf with the incoming point, as the registers show

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90 (R05; ScenarioStructureBSP +0xb4)
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (formerly global_matg_multiplayer)
extern data_array *object_data;       // 0x008603b0

extern int32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
    real_point3d *point); // 0x5013a0, not in this module; blam-cc: EAX -> node_index, ECX -> bsp,
    // EDX -> point (as src/camera/observer_update_location.c declares it); returns a BSP leaf
    // index in EAX, or -1. Here EDX is still this function's own incoming point (0x461e63).
extern int16_t object_find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location,
    real_point3d *center, float radius, datum_index *out_objects, int16_t max_output); // 0x4f6fe0

// blam-cc: EDX -> point
// True if any object of type 1 (_object_type_vehicle) is already within 0.1 world units of
// `point`. game_engine_rate_player_starting_location's caller uses it to veto a starting
// location that a vehicle is parked on.
uint8_t game_engine_location_blocked_by_vehicle(real_point3d *point)
{
    // The 8-byte "location" pair object_find_in_sphere wants: a BSP leaf index and the cluster
    // index that leaf belongs to.
    struct { int32_t leaf_index; int16_t cluster_index; } location;
    datum_index candidates[16];
    int16_t count;
    int16_t i;

    location.leaf_index = bsp3d_node_find_leaf(0, global_collision_bsp, point);
    if (location.leaf_index == -1) {
        location.cluster_index = -1;
    } else {
        location.cluster_index = (int16_t)((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[location.leaf_index & 0x7fffffff].cluster;
    }

    count = object_find_in_sphere(0, 0x11f, &location, point, 0.1f, candidates, 0x10);

    for (i = 0; i < count; i = i + 1) {
        object *obj = ((object_header *)object_data->data)[candidates[i] & 0xffff].data;
        // The original re-fetches the same object pointer and checks it for NULL only inside
        // the type==1 branch; preserved.
        if (obj != 0 && obj->type == 1) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x461e60), from tools/pack.py 0x461e60:

uint FUN_00461e60(void)

{
  uint uVar1;
  short sVar2;
  undefined8 uVar3;
  uint auStackY_20040 [32758];
  int local_48;
  undefined2 local_44;
  uint local_40 [16];

  uVar3 = FUN_005013a0();
  local_48 = (int)uVar3;
  if (local_48 == -1) {
    local_44 = 0xffff;
  }
  else {
    local_44 = *(undefined2 *)(local_48 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  }
  uVar1 = object_find_in_sphere
                    (0,0x11f,&local_48,(int)((ulonglong)uVar3 >> 0x20),0x3dcccccd,local_40,0x10);
  sVar2 = 0;
  if (0 < (short)uVar1) {
    do {
      if (*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_40[sVar2] & 0xffff) * 0xc)
                    + 0xb4) == 1) {
        uVar1 = (local_40[sVar2] & 0xffff) * 3;
        if (*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_40[sVar2] & 0xffff) * 0xc) != 0) {
          return CONCAT31((int3)(uVar1 >> 8),1);
        }
        break;
      }
      sVar2 = sVar2 + 1;
    } while (sVar2 < (short)uVar1);
  }
  return uVar1 & 0xffffff00;
}
#endif
