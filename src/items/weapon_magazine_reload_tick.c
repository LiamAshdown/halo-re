// weapon_magazine_reload_tick  (Ghidra: FUN_004c3900; named from
// out/phase4/items_functions.md, "Advances a one-round-at-a-time reload for a trigger, either
// loading another round or finishing the reload and notifying observers")
// address 0x4c3900, size 284 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4c3900..0x4c3a1b; game-engine test fixed; offsets probed)
// evidence: types/items.h weapon_magazine_state, weapon_control_flags (0x26 = primary|secondary
//   trigger|inhibited bits), object.flags (_object_changed_bit); types/tags.h
//   WeaponMagazine.flags/.rounds_reloaded (0x18)/.rounds_loaded_maximum (0x0a); globals
//   network_game_mode, weapon_bottomless_clip (0x0087abc2).
// register convention: item index is a Ghidra-recognized parameter; magazine index in CX.
// blam-cc: stack -> item_index, CX -> magazine_index
// UNSURE: weapon_trigger_begin_reload and weapon_notify_reload_step are called here with fewer
// visible arguments than their own recognized signatures; item_index and magazine_index are
// threaded through explicitly (matching this module's register convention), and
// is_client_predicted is assumed 0 for this host-side continuation.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "units.h"
#include "game.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t network_game_mode;   // 0x00719720
extern uint8_t weapon_bottomless_clip; // 0x0087abc2
extern game_engine_definition *current_game_engine;

extern void weapon_trigger_begin_reload(datum_index item_index, int16_t magazine_index, int8_t is_client_predicted); // 0x4c35b0
extern void weapon_notify_reload_step(datum_index item_index, int16_t magazine_index); // 0x4c37b0

// Host-side continuation of a magazine reload: moves rounds_reloaded worth of ammunition from
// reserve into the magazine, and either starts loading the next round or finishes the reload.
void weapon_magazine_reload_tick(datum_index item_index, int16_t magazine_index)
{
    object *item_obj;
    weapon_data *wd;
    item_data *id;
    Weapon *weapon_tag;
    WeaponMagazine *magazine_tag;
    weapon_magazine_state *magazine;
    int16_t old_unloaded;
    int16_t new_loaded;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    id = (item_data *)((uint8_t *)item_obj + k_item_data_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;
    magazine = &wd->magazines[magazine_index];

    if (magazine_tag->flags & 1) {
        magazine->rounds_loaded = 0;
    }

    old_unloaded = magazine->rounds_unloaded;
    new_loaded = (old_unloaded <= magazine_tag->rounds_reloaded) ? old_unloaded : magazine_tag->rounds_reloaded;
    new_loaded = magazine->rounds_loaded + new_loaded;
    if (new_loaded > magazine_tag->rounds_loaded_maximum) {
        new_loaded = magazine_tag->rounds_loaded_maximum;
    }

    {
        int skip = 0;
        // 0x4c397f..0x4c39a4: the test is on current_game_engine (0x6f1d20), not network_game_mode. FIXED 2026-09-27.
        if (current_game_engine == 0) {
            if (weapon_bottomless_clip != 0 || (id->flags & _item_held_by_player_bit) == 0) skip = 1;
        } else if (weapon_bottomless_clip != 0) {
            skip = 1;
        }
        if (!skip) {
            magazine->rounds_unloaded = (magazine->rounds_loaded - new_loaded) + old_unloaded;
        }
    }

    magazine->rounds_loaded = new_loaded;
    magazine->state = _weapon_magazine_chamber_pending;
    magazine->state_ticks = 0;

    if (magazine->rounds_unloaded > 0 && new_loaded < magazine_tag->rounds_loaded_maximum &&
        (magazine_tag->flags & 1) == 0 && (wd->control_flags & 0x26) == 0) {
        weapon_trigger_begin_reload(item_index, magazine_index, 0);
        return;
    }
    if (item_obj->network_role == 0 && network_game_mode == 2) {
        weapon_notify_reload_step(item_index, magazine_index);
    }
    item_obj->flags = item_obj->flags | _object_changed_bit;
}

#if 0
Original Ghidra decompilation (0x4c3900):

void FUN_004c3900(uint param_1)

{
  uint *puVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  int iVar5;
  byte *pbVar6;
  short in_CX;
  short sVar7;

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar5 = in_CX * 0x70;
  iVar4 = *(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x4f4);
  pbVar6 = (byte *)(iVar5 + iVar4);
  puVar1 = puVar3 + in_CX * 3 + 0xac;
  if ((*(byte *)(iVar5 + iVar4) & 1) != 0) {
    *(undefined2 *)(puVar1 + 2) = 0;
  }
  sVar2 = *(short *)((int)puVar1 + 6);
  sVar7 = sVar2;
  if (*(short *)(pbVar6 + 0x18) <= sVar2) {
    sVar7 = *(short *)(pbVar6 + 0x18);
  }
  sVar7 = (short)puVar1[2] + sVar7;
  if (*(short *)(pbVar6 + 10) < sVar7) {
    sVar7 = *(short *)(pbVar6 + 10);
  }
  if (DAT_006f1d20 == 0) {
    if ((DAT_0087abc2 != '\0') || ((puVar3[0x7d] & 2) == 0)) goto LAB_004c39b5;
  }
  else if (DAT_0087abc2 != '\0') goto LAB_004c39b5;
  *(short *)((int)puVar1 + 6) = ((short)puVar1[2] - sVar7) + sVar2;
LAB_004c39b5:
  *(short *)(puVar1 + 2) = sVar7;
  *(undefined2 *)puVar1 = 2;
  *(undefined2 *)((int)puVar1 + 2) = 0;
  if ((((0 < *(short *)((int)puVar1 + 6)) && (sVar7 < *(short *)(pbVar6 + 10))) &&
      ((*pbVar6 & 1) == 0)) && ((puVar3[0x8c] & 0x26) == 0)) {
    weapon_trigger_begin_reload(param_1);
    return;
  }
  if ((puVar3[1] == 0) && (DAT_00719720 == 2)) {
    FUN_004c37b0();
  }
  puVar3[4] = puVar3[4] | 0x4000000;
  return;
}
#endif
