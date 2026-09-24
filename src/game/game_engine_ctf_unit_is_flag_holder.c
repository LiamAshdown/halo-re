// game_engine_ctf_unit_is_flag_holder  (Ghidra: FUN_00469780; named per its summary)
// address 0x469780, size 96 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/game_functions.md ("Checks whether a unit is currently a valid holder of
//   its team's objective (flag) object by walking the object/equipment reference chain");
//   types/game.h player::team (0x20), player_data (0x0087a480); ctf_team_flag_object (0x006b0e90,
//   this batch); object.owner_linkage (0xc0, types/objects.h); item_data.ignore_object_index
//   (0x11c is NOT item's -- see UNSURE).
// register convention: player pointer in in_EAX.
//   // blam-cc: EAX -> player
// UNSURE: field offsets +0xc0 and +0x11c are read here off the RESULT of object_try_and_get,
//   i.e. off a flag/weapon object and then off a player-unit object respectively; the second one
//   at +0x11c does not obviously match object.parent_object (also 0x11c) for a unit -- kept
//   literal.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern datum_index ctf_team_flag_object[2]; // 0x006b0e90
extern data_array *player_data;             // 0x0087a480

extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0

// blam-cc: EAX -> player
// True when the player's team has a live flag object, that flag object's owner_linkage (0xc0,
// read here as a player handle, matching ctf_engine_flag_tick.c's identical chain) resolves
// through datum_get against player_data, the resolved player's controlled unit resolves via
// object_try_and_get(_object_mask_unit), and that unit's own +0x11c field is set.
uint8_t game_engine_ctf_unit_is_flag_holder(player *p)
{
    datum_index flag_object;
    object *flag_obj;
    player *carrier;
    object *unit_obj;

    if (p == (player *)0) {
        return 0;
    }
    flag_object = ctf_team_flag_object[p->team];
    if (flag_object == (datum_index)0xffffffff) {
        return 0;
    }
    flag_obj = object_try_and_get(flag_object, _object_mask_weapon);
    if (flag_obj == (object *)0) {
        return 0;
    }
    if (*(int32_t *)((uint8_t *)flag_obj + 0xc0) == -1) {
        return 0;
    }
    carrier = (player *)datum_get((datum_index)*(uint32_t *)((uint8_t *)flag_obj + 0xc0), player_data);
    if (carrier == (player *)0) {
        return 0;
    }
    unit_obj = object_try_and_get(carrier->unit, _object_mask_unit);
    if (unit_obj == (object *)0 || *(int32_t *)((uint8_t *)unit_obj + 0x11c) == -1) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x469780), from tools/pack.py 0x469780:

undefined4 FUN_00469780(void)

{
  int in_EAX;
  int iVar1;

  if ((((in_EAX != 0) && (*(int *)(&DAT_006b0e90 + *(int *)(in_EAX + 0x20) * 4) != -1)) &&
      (iVar1 = object_try_and_get(4), iVar1 != 0)) &&
     (((*(int *)(iVar1 + 0xc0) != -1 && (iVar1 = datum_get(), iVar1 != 0)) &&
      ((iVar1 = object_try_and_get(3), iVar1 != 0 && (*(int *)(iVar1 + 0x11c) != -1)))))) {
    return 1;
  }
  return 0;
}
#endif
