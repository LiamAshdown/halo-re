// game_engine_touch_multiplayer_predicted_resources  (Ghidra: FUN_00466890; named per
// out/phase4/game_functions.md: "Resolves a set of tag dependencies (a 6-entry and a 16-entry
// TagDependency array) inside a map-globals-like structure, selected by a difficulty/type
// index.")
// address 0x466890, size 688 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x466890
//   --stop-address=0x466b60) because Ghidra's decompile drops the EAX argument setup before
//   every call to object_notify_predicted_resources_if_valid (0x4f7ad0, src/objects/) AND
//   mis-renders the jump table: nibble value 5 shares the "touch all six vehicles" default path
//   (confirmed from the raw jump table bytes at 0x466b40), which Ghidra's switch listing folds
//   into `default` while still separately (and correctly) listing cases 2/3/4/6/7. Offsets:
//   types/tags.h Globals::multiplayer_information (TagReflexive, data pointer at globals+0x168)
//   -> GlobalsMultiplayerInformation::vehicles (TagReflexive at +0x20, data pointer at +0x24)
//   -> GlobalsVehicle (0x10 bytes, tag_id at +0xc); types/tags.h Globals::weapon_list (data
//   pointer at globals+0x150) -> GlobalsWeapon (0x10 bytes, tag_id at +0xc), 16 entries; the
//   game_variant fields already named in types/game.h (vehicle_set at 0x006f1ce8,
//   game_engine_index at 0x006f1cb8).
// register convention: no parameters; pure global-state driven.
// UNSURE: this function's own name; why array slots 3, 10 and 11 of the 16-entry weapon list are
//   skipped in the closing loop (10 and 11 are the same two elements the oddball-ball / ctf-flag
//   checks above already touch individually; 3 is unexplained).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Globals *global_globals;          // 0x00746fa0
extern game_variant game_engine_variant; // 0x006f1c88

extern tag_instance *tag_instances; // 0x0087bc14

extern void object_notify_predicted_resources_if_valid(datum_index definition_tag); // 0x4f7ad0, blam-cc: EAX
extern void predicted_resource_list_touch(TagReflexive *resources); // 0x4449f0, blam-cc: ESI ->
    // resources (matches src/cache/predicted_resource_list_touch.c)

// blam-cc: (raw tag id) -> ESI-relative predicted_resources_field
static void touch_tag_if_valid(int32_t tag_id)
{
    uint8_t *tag_data;
    if (tag_id != -1) {
        tag_data = (uint8_t *)tag_instances[tag_id & 0xffff].data;
        predicted_resource_list_touch((TagReflexive *)(tag_data + 0x170));
    }
}

// Pre-touches (loads/pins) the predicted-resource lists of every multiplayer vehicle and weapon
// tag the current map's globals tag might spawn: one vehicle selected by the active variant's
// vehicle_set nibble (or all six when that nibble is 0, 1, 5 or 8), the oddball ball weapon when
// the engine is oddball, the ctf flag weapon when the engine is ctf, and every other weapon-list
// entry except the three slots already covered above.
void game_engine_touch_multiplayer_predicted_resources(void)
{
    GlobalsMultiplayerInformation *mp_info;
    GlobalsVehicle *vehicles;
    GlobalsWeapon *weapons;
    int32_t weapon_tags[16];
    int32_t i;

    mp_info = (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
    vehicles = (GlobalsVehicle *)mp_info->vehicles.pointer;

    switch (game_engine_variant.red_vehicle_set & 0xf) {
    case 2:
        object_notify_predicted_resources_if_valid((datum_index)*(int32_t *)&vehicles[0].vehicle.tag_id);
        break;
    case 3:
        object_notify_predicted_resources_if_valid((datum_index)*(int32_t *)&vehicles[1].vehicle.tag_id);
        break;
    case 4:
        object_notify_predicted_resources_if_valid((datum_index)*(int32_t *)&vehicles[2].vehicle.tag_id);
        break;
    case 6:
        object_notify_predicted_resources_if_valid((datum_index)*(int32_t *)&vehicles[3].vehicle.tag_id);
        break;
    case 7:
        object_notify_predicted_resources_if_valid((datum_index)*(int32_t *)&vehicles[4].vehicle.tag_id);
        break;
    default: // 0, 1, 5, 8 (and any other value the switch's jump table does not cover)
        touch_tag_if_valid(*(int32_t *)&vehicles[0].vehicle.tag_id);
        touch_tag_if_valid(*(int32_t *)&vehicles[1].vehicle.tag_id);
        touch_tag_if_valid(*(int32_t *)&vehicles[2].vehicle.tag_id);
        touch_tag_if_valid(*(int32_t *)&vehicles[3].vehicle.tag_id);
        touch_tag_if_valid(*(int32_t *)&vehicles[4].vehicle.tag_id);
        touch_tag_if_valid(*(int32_t *)&vehicles[5].vehicle.tag_id);
        break;
    }

    weapons = (GlobalsWeapon *)global_globals->weapon_list.pointer;

    if (game_engine_variant.game_engine_index == _game_engine_oddball) {
        touch_tag_if_valid(*(int32_t *)&weapons[10].weapon.tag_id);
    }
    if (game_engine_variant.game_engine_index == _game_engine_ctf) {
        touch_tag_if_valid(*(int32_t *)&weapons[11].weapon.tag_id);
    }

    weapon_tags[0] = *(int32_t *)&weapons[0].weapon.tag_id;
    weapon_tags[1] = *(int32_t *)&weapons[1].weapon.tag_id;
    weapon_tags[2] = *(int32_t *)&weapons[2].weapon.tag_id;
    weapon_tags[3] = -1; // UNSURE: unexplained skip
    weapon_tags[4] = *(int32_t *)&weapons[4].weapon.tag_id;
    weapon_tags[5] = *(int32_t *)&weapons[5].weapon.tag_id;
    weapon_tags[6] = *(int32_t *)&weapons[6].weapon.tag_id;
    weapon_tags[7] = *(int32_t *)&weapons[7].weapon.tag_id;
    weapon_tags[8] = *(int32_t *)&weapons[8].weapon.tag_id;
    weapon_tags[9] = *(int32_t *)&weapons[9].weapon.tag_id;
    weapon_tags[10] = -1; // already touched above (oddball ball)
    weapon_tags[11] = -1; // already touched above (ctf flag)
    weapon_tags[12] = *(int32_t *)&weapons[12].weapon.tag_id;
    weapon_tags[13] = *(int32_t *)&weapons[13].weapon.tag_id;
    weapon_tags[14] = *(int32_t *)&weapons[14].weapon.tag_id;
    weapon_tags[15] = *(int32_t *)&weapons[15].weapon.tag_id;

    for (i = 0; i < 16; i = i + 1) {
        if (weapon_tags[i] != -1) {
            object_notify_predicted_resources_if_valid((datum_index)weapon_tags[i]);
        }
    }
}

#if 0
Original Ghidra decompilation (0x466890), from tools/pack.py 0x466890:

void FUN_00466890(void)

{
  int iVar1;
  int iVar2;
  int local_40 [16];

  iVar2 = *(int *)(DAT_00746fa0 + 0x168);
  switch(DAT_006f1ce8 & 0xf) {
  case 2:
    FUN_004f7ad0();
    break;
  case 3:
    FUN_004f7ad0();
    break;
  case 4:
    FUN_004f7ad0();
    break;
  default:
    if (*(int *)(*(int *)(iVar2 + 0x24) + 0xc) != -1) {
      predicted_resource_list_touch();
    }
    if (*(int *)(*(int *)(iVar2 + 0x24) + 0x1c) != -1) {
      predicted_resource_list_touch();
    }
    if (*(int *)(*(int *)(iVar2 + 0x24) + 0x2c) != -1) {
      predicted_resource_list_touch();
    }
    if (*(int *)(*(int *)(iVar2 + 0x24) + 0x3c) != -1) {
      predicted_resource_list_touch();
    }
    if (*(int *)(*(int *)(iVar2 + 0x24) + 0x4c) != -1) {
      predicted_resource_list_touch();
    }
    if (*(int *)(*(int *)(iVar2 + 0x24) + 0x5c) != -1) {
      predicted_resource_list_touch();
    }
    break;
  case 6:
    FUN_004f7ad0();
    break;
  case 7:
    FUN_004f7ad0();
  }
  if (DAT_006f1cb8 == 3) {
    if (*(int *)(*(int *)(DAT_00746fa0 + 0x150) + 0xac) == -1) goto LAB_00466a71;
    predicted_resource_list_touch();
  }
  if ((DAT_006f1cb8 == 1) && (*(int *)(*(int *)(DAT_00746fa0 + 0x150) + 0xbc) != -1)) {
    predicted_resource_list_touch();
  }
LAB_00466a71:
  iVar2 = *(int *)(DAT_00746fa0 + 0x150);
  local_40[0] = *(int *)(iVar2 + 0xc);
  local_40[1] = *(undefined4 *)(iVar2 + 0x1c);
  local_40[2] = *(undefined4 *)(iVar2 + 0x2c);
  local_40[4] = *(undefined4 *)(iVar2 + 0x4c);
  local_40[5] = *(undefined4 *)(iVar2 + 0x5c);
  local_40[6] = *(undefined4 *)(iVar2 + 0x6c);
  local_40[7] = *(undefined4 *)(iVar2 + 0x7c);
  local_40[8] = *(undefined4 *)(iVar2 + 0x8c);
  local_40[9] = *(undefined4 *)(iVar2 + 0x9c);
  local_40[0xc] = *(undefined4 *)(iVar2 + 0xcc);
  local_40[3] = 0xffffffff;
  local_40[10] = 0xffffffff;
  local_40[0xb] = 0xffffffff;
  local_40[0xd] = *(undefined4 *)(iVar2 + 0xdc);
  local_40[0xe] = *(undefined4 *)(iVar2 + 0xec);
  local_40[0xf] = *(undefined4 *)(iVar2 + 0xfc);
  iVar2 = 0;
  do {
    if ((local_40[iVar2] != -1) && (iVar1 = FUN_00462df0(), iVar1 != -1)) {
      predicted_resource_list_touch();
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 0x10);
  return;
}

Raw disassembly (objdump -d -M intel --start-address=0x466890 --stop-address=0x466b60) confirmed:
the EAX argument to every 0x4f7ad0 call is loaded immediately beforehand from
[[globals+0x168]+0x24 + <vehicle slot offset>], and the jump table at 0x466b40 is
{0x4668b7, 0x4668c7, 0x4668d7, 0x466907, 0x4668e7, 0x4668f7} for nibble values 2..7, i.e.
nibble 5 shares the "touch all six" target (0x466907) with the out-of-range default.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
