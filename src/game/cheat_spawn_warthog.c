// cheat_spawn_warthog  (Ghidra: cheat_spawn_warthog, already named)
// address 0x45a5c0, size 97 bytes
// name confidence: 0.85   rewrite confidence: 0.6
// evidence: the "warthog" string; calls cheat_spawn_objects_near_camera. Globals+0x164/+0x168
//   are the tag-index table's valid flag and base pointer respectively (the same shape
//   game_engine_load_from_variant and other tag-table walks in this module use), each entry a
//   0x10-byte record with a name pointer at +0x04 and (per cheat_spawn_objects_near_camera's own
//   record layout) a tag handle at +0xc.
// register convention: no arguments.
//
// UNSURE: FUN_00625430 is a CRT/compiler string-compare helper (strcmp-shaped, given the two
// arguments and zero-means-no-match usage) but is not in this batch; modelled as such.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

extern Globals *global_globals; // 0x00746fa0

extern int32_t FUN_00625430(const char *a, const char *b); // 0x625430, UNSURE: CRT strcmp-shaped
extern void cheat_spawn_objects_near_camera(TagDependency *tag_array, int16_t count); // 0x45a800

// Finds the "warthog" vehicle tag by name in the loaded tag-index table and spawns one near the
// camera.
void cheat_spawn_warthog(void)
{
    GlobalsMultiplayerInformation *info;
    GlobalsVehicle *vehicles;
    int32_t count;
    int32_t i;

    // RESOLVED (phase 4 review): globals+0x164/+0x168 is Globals::multiplayer_information, and
    // the +0x20/+0x24 pair inside it is GlobalsMultiplayerInformation::vehicles (0x10 for the
    // flag TagDependency plus 0x10 for the unit one). The +4 the loop adds to each 0x10-byte
    // entry is TagDependency::path_pointer, so the comparison is against the tag PATH.
    info = (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
    if (global_globals->multiplayer_information.count != 0) {
        count = (int32_t)info->vehicles.count;
        vehicles = (GlobalsVehicle *)info->vehicles.pointer;
        i = 0;
        if (0 < count) {
            while (FUN_00625430((const char *)vehicles[i].vehicle.path_pointer, "warthog") == 0) {
                i = i + 1;
                if (count <= i) {
                    return;
                }
            }
            cheat_spawn_objects_near_camera(&vehicles[i].vehicle, 1);
        }
    }
}

#if 0
Original Ghidra decompilation (0x45a5c0), from tools/pack.py 0x45a5c0:

void __cdecl cheat_spawn_warthog(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;

  iVar1 = *(int *)(DAT_00746fa0 + 0x168);
  if (*(int *)(DAT_00746fa0 + 0x164) != 0) {
    iVar2 = *(int *)(iVar1 + 0x24);
    iVar5 = 0;
    if (0 < *(int *)(iVar1 + 0x20)) {
      puVar4 = (undefined4 *)(iVar2 + 4);
      while (iVar3 = FUN_00625430(*puVar4,"warthog"), iVar3 == 0) {
        iVar5 = iVar5 + 1;
        puVar4 = puVar4 + 4;
        if (*(int *)(iVar1 + 0x20) <= iVar5) {
          return;
        }
      }
      cheat_spawn_objects_near_camera(iVar5 * 0x10 + iVar2,1);
    }
  }
  return;
}
#endif
