// weapon_get_first_person_animation_time  (Ghidra: FUN_004c2f80; renamed per
// items_types_notes.md: "reads Weapon.first_person_animations ... walks the animation graph's
// 0xb4-stride animation block and returns a frame count. The name currently sits on 0x4c6340" --
// that address is one of the shell/main fragments and is not a real function; this is the
// genuine owner)
// address 0x4c2f80, size 225 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: types/tags.h Weapon.first_person_animations (tag_id 0x478), ModelAnimations
//   .first_person_weapons (TagReflexive 0x48) and .animations (TagReflexive 0x74),
//   ModelAnimationsAnimation (frame_count 0x22, key_frame_index 0x34, size 0xb4),
//   ModelAnimationsFirstPersonWeapon (uint16 animation, size 2), Weapon.weapon_type (0x4e2).
//   Confirmed against the binary with an offsetof probe against types/tags.h.
// register convention: item index in EAX; animation index in CX; category and mode are
//   Ghidra-recognized stack parameters.
// blam-cc: EAX -> item_index, CX -> animation_index, stack -> (category, mode)
// VERIFIED 2026-09-27 against objdump 0x4c2f80..0x4c3060. Call-site CX values: weapon_ready 0xa (0x4c28b8),
// weapon_trigger_begin_reload 7 (0x4c3781), weapon_reset_triggers 7 (0x4c4bdd), biped_update 0xd (0x559d45,
// 0x559d73). The weapon_type 1 override (fp animation slot 0x17, used for modes 0 / 2) is preserved literally,
// including the original's read of animations[-1] when the weapon has fewer than 0x18 fp animations.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "fn_items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Returns the frame count (category 0) or key_frame_index (category 1) of one of a weapon's
// first-person animations, or 0 when the graph/index is not available. For weapon_type 1 and
// category 0 with mode 0 or 2, substitutes a fixed override animation's frame count instead.
int16_t weapon_get_first_person_animation_time(datum_index item_index, int16_t animation_index,
    int16_t category, int16_t mode)
{
    object *item_obj;
    Weapon *weapon_tag;
    datum_index graph_tag_id;
    int16_t result = 0;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    graph_tag_id = *(datum_index *)&weapon_tag->first_person_animations.tag_id;

    if (graph_tag_id != (datum_index)0xffffffff) {
        ModelAnimations *graph = (ModelAnimations *)tag_instances[(uint16_t)graph_tag_id].data;

        if (graph->first_person_weapons.count != 0) {
            ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *fp_weapon =
                (ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *)graph->first_person_weapons.pointer;

            if (fp_weapon != 0 && animation_index >= 0 &&
                animation_index < fp_weapon->animations.count) {
                int16_t resolved = ((uint16_t *)fp_weapon->animations.pointer)[animation_index];

                if (resolved != -1) {
                    ModelAnimationsAnimation *animations =
                        (ModelAnimationsAnimation *)graph->animations.pointer;

                    if (category == 0) {
                        result = animations[resolved].frame_count;
                    } else if (category == 1) {
                        result = animations[resolved].key_frame_index;
                    }

                    if (category == 0 && weapon_tag->weapon_type == 1) {
                        int16_t override_index;

                        if (fp_weapon->animations.count < 0x18) {
                            override_index = -1;
                        } else {
                            override_index = ((uint16_t *)fp_weapon->animations.pointer)[0x17];
                        }
                        if (mode == 0 || mode == 2) {
                            result = animations[override_index].frame_count;
                        }
                    }
                }
            }
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4c2f80):

undefined2 FUN_004c2f80(short param_1,short param_2)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  undefined2 uVar5;
  uint in_EAX;
  short in_CX;
  int iVar6;
  int iVar7;

  iVar7 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar2 = *(uint *)(iVar7 + 0x478);
  uVar5 = 0;
  if ((((uVar2 != 0xffffffff) &&
       (iVar3 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), *(int *)(iVar3 + 0x48) != 0
       )) && (iVar4 = *(int *)(iVar3 + 0x4c), iVar4 != 0)) && (-1 < in_CX)) {
    if ((int)in_CX < *(int *)(iVar4 + 0x10)) {
      sVar1 = *(short *)(*(int *)(iVar4 + 0x14) + in_CX * 2);
      if (sVar1 != -1) {
        iVar6 = sVar1 * 0xb4 + *(int *)(iVar3 + 0x78);
        if (param_1 == 0) {
          uVar5 = *(undefined2 *)(iVar6 + 0x22);
        }
        else if (param_1 == 1) {
          uVar5 = *(undefined2 *)(iVar6 + 0x34);
        }
        if ((param_1 == 0) && (*(short *)(iVar7 + 0x4e2) == 1)) {
          if (*(int *)(iVar4 + 0x10) < 0x18) {
            iVar7 = -1;
          }
          else {
            iVar7 = (int)*(short *)(*(int *)(iVar4 + 0x14) + 0x2e);
          }
          if ((param_2 == 0) || (param_2 == 2)) {
            uVar5 = *(undefined2 *)(iVar7 * 0xb4 + *(int *)(iVar3 + 0x78) + 0x22);
          }
        }
      }
    }
  }
  return uVar5;
}
#endif
