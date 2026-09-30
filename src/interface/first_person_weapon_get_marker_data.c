// first_person_weapon_get_marker_data  (Ghidra: already named)
// address 0x492ad0, size 176 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Looks up a named marker (e.g. 'flashlight') on
// the local player's first-person weapon model and returns its transform data."; call to
// model_markers_get_by_name matches the established prototype from
// src/objects/object_get_node_local_transform.c.
// register convention: all four parameters are Ghidra-recognized (param_1..param_4); param_2 is
// never read anywhere in the function body (a genuinely dead parameter in the original binary).
// UNSURE: object_try_and_get's object-index argument is not visible in Ghidra's decompilation of
// this function (called as `object_try_and_get(4)` with only the type mask shown); param_1
// (weapon_index) is the only object this function is otherwise checking, so it is passed here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "objects.h"

extern tag_instance *tag_instances; // 0x0087bc14, types/cache.h
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98

extern void *object_try_and_get(datum_index object_index, uint32_t mask); // 0x4f6ec0, objects module
extern int16_t camera_get_type_for_player(int16_t player_index); // 0x445ac0, module camera; blam-cc: player_index in CX (in_CX)
extern int32_t local_player_index_for_weapon(datum_index weapon_index); // 0x494010, this module
extern int16_t model_markers_get_by_name(datum_index model_tag_id, const char *name, uint8_t *region_permutations,
    int16_t *node_remap, real_matrix4x3 *node_matrices, uint8_t mirrored, object_marker *out, int16_t maximum);
    // 0x4d7850, ECX model_tag_id, EAX name

// If weapon_index both exists as a live object and is the local player's current first-person
// weapon, and the active camera is first-person, and the weapon's weapon_hud_interface tag is
// both present and has both a marker-name table (+0x468) and a pickup-notification dependency
// (+0x478), looks up a named marker on the first-person weapon model and returns its transform.
// Returns 0 on any failed gate.
uint32_t first_person_weapon_get_marker_data(datum_index weapon_index, const char *marker_name,
                                              object_marker *out, uint32_t maximum)
{
    object *obj;
    int32_t local_player;
    int16_t camera_type;
    first_person_weapon_interface *fp;
    uint8_t *item_tag_data;

    obj = (object *)object_try_and_get(weapon_index, 4);
    if (obj == 0) {
        return 0;
    }

    local_player = local_player_index_for_weapon(weapon_index);
    if ((int16_t)local_player == -1) {
        return 0;
    }

    camera_type = camera_get_type_for_player((int16_t)local_player);
    if (camera_type != 0) {
        return 0;
    }

    fp = &first_person_weapon_interfaces[local_player];
    item_tag_data = *(uint8_t **)((uint8_t *)tag_instances +
                                   (obj->definition_tag & 0xffff) * 0x20 + 0x14);

    if (fp->weapon_hud_valid != 0 && *(int32_t *)(item_tag_data + 0x468) != -1 &&
        *(int32_t *)(item_tag_data + 0x478) != -1) {
        // 0x492b50..0x492b6d: ECX = tag +0x468 (the model), EAX = marker_name, push 0, fp+0x1d8e, fp+0x108c, 0, out,
        // maximum
        return (uint32_t)model_markers_get_by_name(*(datum_index *)(item_tag_data + 0x468), marker_name,
            (uint8_t *)0, fp->weapon_hud_element, (real_matrix4x3 *)fp->node_matrices, 0, out, (int16_t)maximum);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x492ad0):

uint first_person_weapon_get_marker_data
               (undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  uint *puVar2;
  uint uVar3;
  short sVar4;

  puVar2 = (uint *)object_try_and_get(4);
  uVar3 = 0;
  if (puVar2 != (uint *)0x0) {
    uVar3 = FUN_00494010(param_1);
    sVar4 = (short)uVar3;
    if (sVar4 != -1) {
      uVar3 = camera_get_type_for_player();
      if ((short)uVar3 == 0) {
        uVar3 = sVar4 * 0x1ea0 + DAT_006b2d98;
        iVar1 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if (((*(char *)(uVar3 + 0x1d8c) != '\0') && (*(int *)(iVar1 + 0x468) != -1)) &&
           (*(int *)(iVar1 + 0x478) != -1)) {
          uVar3 = model_markers_get_by_name(0,uVar3 + 0x1d8e,uVar3 + 0x108c,0,param_3,param_4);
          return uVar3;
        }
      }
    }
  }
  return uVar3 & 0xffff0000;
}
#endif
