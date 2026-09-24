// game_engine_notify_weapon_ready_state_change  (Ghidra: FUN_00462000; renamed per its summary)
// address 0x462000, size 191 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Notifies the game variant about a pickup-flag state
// change and asks it whether the item pickup is permitted"); types/tags.h WeaponFlags bit 3
// (must_be_readied); types/items.h weapon_data::flags (+0x22c, "weapon_flags" runtime bits,
// unnamed here); types/game.h game_engine_definition::unknown_40/object_expired (+0x40/+0x44);
// src/game/game_engine_is_valid_team_player.c's player_index_from_unit_index signature; types/objects.h
// object::definition_tag (+0x00); types/cache.h tag_instance (stride 0x20, data at +0x14).
// register convention: a weapon object handle in ECX (forwarded to object_try_and_get, matching
// its own established "object handle in ECX" convention); param_1/param_2 are this function's
// own stack parameters (param_1 the same weapon handle reused for player_index_from_unit_index, param_2 a
// second object whose definition tag's weapon_flags is tested).
//   // blam-cc: ECX -> weapon (== param_1), stack -> param_1, other_object
// UNSURE: weapon_data::flags bit 0x20 has no established name; player_index_from_unit_index and the vtable slots
// at +0x40/+0x44 are called with the argument lists Ghidra shows, which may be incomplete.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "game.h"

extern data_array *object_headers; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern datum_index player_index_from_unit_index(datum_index object_or_unit); // 0x474db0, UNSURE signature/behavior

// blam-cc: ECX -> weapon (== param_1), stack -> param_1, other_object
uint32_t game_engine_notify_weapon_ready_state_change(uint32_t param_1, uint32_t other_object)
{
    object *weapon;
    Object *other_definition;

    if (current_game_engine == 0) {
        return 1;
    }

    weapon = object_try_and_get((datum_index)param_1, _object_mask_weapon);
    if (weapon == 0) {
        return 1;
    }

    other_definition = (Object *)tag_instances[
        (((object_header *)object_headers->data)[other_object & 0xffff].data->definition_tag) & 0xffff
    ].data;

    if (((*(uint32_t *)((uint8_t *)other_definition + 0x308) >> 3) & 1) == 0) {
        return 1;
    }

    if ((*(uint32_t *)((uint8_t *)weapon + 0x22c) & 0x20) != 0) {
        *(uint32_t *)((uint8_t *)weapon + 0x22c) &= 0xffffffdf;
        if (current_game_engine->object_expired != 0) {
            ((void (*)(uint32_t))current_game_engine->object_expired)(other_object);
        }
    }
    *(uint32_t *)((uint8_t *)weapon + 0x22c) |= 0x20;

    if (current_game_engine->unknown_40 != 0) {
        uint32_t identifier_result = player_index_from_unit_index(param_1);
        return ((uint32_t (*)(uint32_t, uint32_t))current_game_engine->unknown_40)
            (other_object, identifier_result);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x462000), from tools/pack.py 0x462000:

undefined4 FUN_00462000(undefined4 param_1,uint param_2)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;

  iVar3 = DAT_006f1d20;
  if (DAT_006f1d20 == 0) {
    return 1;
  }
  iVar1 = object_try_and_get(4);
  if ((iVar1 != 0) &&
     ((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc)
                          & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 3 & 1) != 0)) {
    if ((*(uint *)(iVar1 + 0x22c) & 0x20) != 0) {
      *(uint *)(iVar1 + 0x22c) = *(uint *)(iVar1 + 0x22c) & 0xffffffdf;
      if (*(code **)(iVar3 + 0x44) != (code *)0x0) {
        (**(code **)(iVar3 + 0x44))(param_2);
        iVar3 = DAT_006f1d20;
      }
    }
    *(uint *)(iVar1 + 0x22c) = *(uint *)(iVar1 + 0x22c) | 0x20;
    if (*(int *)(iVar3 + 0x40) != 0) {
      uVar2 = FUN_00474db0(param_1);
      uVar2 = (**(code **)(iVar3 + 0x40))(param_2,uVar2);
      return uVar2;
    }
  }
  return 1;
}
#endif
