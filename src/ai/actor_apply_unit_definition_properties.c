// actor_apply_unit_definition_properties  (Ghidra: actor_apply_unit_definition_properties, renamed)
// address 0x426cf0, size 901 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN (from objdump 0x426cf0..0x42707c; the draft was confidence 0.15 with guessed callee signatures).
//   With either override vitality of the ActorVariant above zero (+0x200 body, +0x204 shield),
//   object_initialize_shield_stun_thresholds(EAX unit, ESI &body, EDI &shield). A non-zero +0x20c word goes to
//   unit +0x176. The first four change colours (+0x22c count, 0x20 each at +0x230, color0 +0, color1 +0xc) are
//   interpolated at a random fraction into unit +0x188 + 12i and copied to +0x1b8 + 12i. The initial weapon
//   (+0x70) is created (role 3, or 0 on a server for a type with network deltas) and picked up
//   (unit_pickup_weapon(EAX weapon, ECX unit, stack 2)); a weapon that is not picked up is deleted
//   (object_delete_unparented first when its +4 role is 0; object_delete_recursive(weapon, 0) for roles 0 and 3).
//   Grenades: type +0x180 (-1 none), count min +0x1d0 .. max +0x1d2 at random, added to unit +0x31e + type, and
//   the type stored at +0x31c/+0x31d. Equipment (+0x1cc) whose tag +0x308 word is neither 0 nor 6 is created
//   and selected (unit_try_select_equipment(unit, item, 1)) or deleted. Variant flags bits 4/5 set unit +0x204
//   bits 0x10 (and 0x20 for bit 5), +0x37c = 1 and +0x380 = 1 or 0 by the Unit tag's flag bit 5.
// blam-cc: EAX -> actor_variant_tag, stack -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;           // 0x008603b0
extern tag_instance *tag_instances;       // 0x0087bc14
extern uint32_t random_seed_global;       // 0x00719cd0
extern int16_t network_game_mode;         // 0x00719720, word; 2 = server
extern object_type_definition *object_type_definitions[12]; // 0x0069bfdc

extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t);
    // 0x43f6a0, blam-cc: EAX -> color1, ECX -> color0, stack -> dest, flags, t
extern void object_initialize_shield_stun_thresholds(uint32_t object_index, float *override_max_body_vitality,
    float *override_max_shield_vitality); // 0x4ed440, blam-cc: EAX, ESI, EDI
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,
    datum_index role); // 0x4f53a0, blam-cc: EAX -> placement, stack -> definition_tag, role
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index);
    // 0x56d400, blam-cc: EAX -> weapon_index, ECX -> unit_index, stack -> pickup_mode
extern void object_delete_unparented(uint32_t object_index); // 0x4f5aa0, blam-cc: EDI
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0
extern uint8_t unit_try_select_equipment(uint32_t unit_index, uint32_t new_equipment_object_index,
    int16_t release_current); // 0x56d1a0
extern void object_delete(uint32_t object_index); // 0x4f5bd0, blam-cc: EAX

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

// Creates `definition_tag` for `unit_index`: role 3, or 0 on a server when the tag's object type sends deltas.
static datum_index actor_create_unit_item(datum_index definition_tag, datum_index unit_index)
{
    object_placement_data placement;
    uint32_t role = 3;

    object_placement_data_initialize(&placement, definition_tag, unit_index);
    if (network_game_mode == 2) {
        int16_t type = *(int16_t *)tag_instances[placement.definition_tag & 0xffff].data;

        if (object_type_definitions[type]->network_delta_message_type != -1) {
            role = 0;
        }
    }
    return object_new_with_datum_role_control(&placement, role);
}

void actor_apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index)
{
    uint8_t *variant = (uint8_t *)tag_instances[actor_variant_tag & 0xffff].data;
    uint8_t *unit = object_get(unit_index);
    uint8_t *unit_tag = (uint8_t *)tag_instances[*(datum_index *)&((ActorVariant *)variant)->actor_definition.tag_id & 0xffff].data;
    int16_t i;

    if (((ActorVariant *)variant)->body_vitality > 0.0f || ((ActorVariant *)variant)->shield_vitality > 0.0f) {
        object_initialize_shield_stun_thresholds(unit_index, (float *)(variant + 0x200), (float *)(variant + 0x204));
    }
    if (*(int16_t *)&((ActorVariant *)variant)->forced_shader_permutation != 0) {
        *(int16_t *)&((unit_object *)unit)->base.forced_shader_permutation = *(int16_t *)&((ActorVariant *)variant)->forced_shader_permutation;
    }
    for (i = 0; i < *(int32_t *)&((ActorVariant *)variant)->change_colors.count; i++) {
        uint8_t *change_color = *(uint8_t **)&((ActorVariant *)variant)->change_colors.pointer + i * 0x20;

        if (i < 4) {
            ColorRGB *working = (ColorRGB *)(unit + 0x188 + i * 0xc);

            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            color_interpolate((ColorRGB *)(change_color + 0xc), (ColorRGB *)change_color, working, 1,
                (float)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f);
            *(ColorRGB *)(unit + 0x1b8 + i * 0xc) = *working;
        }
    }
    if (*(datum_index *)&((ActorVariant *)variant)->weapon.tag_id != k_datum_index_none) {
        datum_index weapon = actor_create_unit_item(*(datum_index *)&((ActorVariant *)variant)->weapon.tag_id, unit_index);

        if (weapon != k_datum_index_none && !unit_pickup_weapon(2, weapon, unit_index)) {
            int32_t role = *(int32_t *)(object_get(weapon) + 4);

            if (role == 0) {
                object_delete_unparented(weapon);
                object_delete_recursive(weapon, 0);
            } else if (role == 3) {
                object_delete_recursive(weapon, 0);
            }
        }
    }
    if (*(int16_t *)&((ActorVariant *)variant)->grenade_type != -1) {
        int16_t type = *(int16_t *)&((ActorVariant *)variant)->grenade_type;
        int16_t minimum = *(int16_t *)(variant + 0x1d0);
        int32_t range = (int16_t)(*(int16_t *)(variant + 0x1d2) + 1) - minimum;
        uint8_t *object = object_get(unit_index);

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        object[0x31e + type] = (uint8_t)(object[0x31e + type] +
            (uint8_t)(((uint32_t)range * (random_seed_global >> 0x10)) >> 0x10) + (uint8_t)minimum);
        object[0x31d] = (uint8_t)type;
        object[0x31c] = (uint8_t)type;
    }
    if (*(datum_index *)&((ActorVariant *)variant)->equipment.tag_id != k_datum_index_none) {
        int16_t equipment_kind = *(int16_t *)((uint8_t *)tag_instances[*(datum_index *)&((ActorVariant *)variant)->equipment.tag_id & 0xffff].data
            + 0x308);

        if (equipment_kind != 0 && equipment_kind != 6) {
            datum_index equipment = actor_create_unit_item(*(datum_index *)&((ActorVariant *)variant)->equipment.tag_id, unit_index);

            if (equipment != k_datum_index_none && !unit_try_select_equipment(unit_index, equipment, 1)) {
                object_delete(equipment);
            }
        }
    }
    if (*(uint32_t *)variant & 0x30) {
        if (*(uint32_t *)variant & 0x20) {
            ((unit_object *)unit)->unit.flags |= 0x20;
        }
        ((unit_object *)unit)->unit.flags |= 0x10;
        ((struct unit_object *)unit)->unit.active_camouflage_power = 1.0f;
        ((struct unit_object *)unit)->unit.super_active_camouflage_power = (unit_tag[0] & 0x20) ? 1.0f : 0.0f;
    }
}

#if 0
Original Ghidra decompilation (0x426cf0):

void FUN_00426cf0(uint param_1)

{
  char *pcVar1;
  uint *puVar2;
  byte *pbVar3;
  int iVar4;
  char cVar5;
  uint in_EAX;
  int iVar6;
  int iVar7;
  undefined4 uVar8;
  uint uVar9;
  undefined1 uVar10;
  short sVar11;
  char local_94;
  uint local_88 [34];

  puVar2 = *(uint **)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  pbVar3 = *(byte **)((puVar2[4] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar6 = (param_1 & 0xffff) * 0xc;
  iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  if ((0.0 < (float)puVar2[0x80]) || (0.0 < (float)puVar2[0x81])) {
    object_initialize_shield_stun_thresholds();
  }
  if ((short)puVar2[0x83] != 0) {
    *(short *)(iVar4 + 0x176) = (short)puVar2[0x83];
  }
  sVar11 = 0;
  if (0 < (int)puVar2[0x8b]) {
    iVar7 = 0;
    do {
      if (sVar11 < 4) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        iVar7 = iVar4 + iVar7 * 0xc;
        color_interpolate((undefined4 *)(iVar7 + 0x188),1,
                          (float)(random_seed_global >> 0x10) * 1.5259022e-05);
        *(undefined4 *)(iVar7 + 0x1b8) = *(undefined4 *)(iVar7 + 0x188);
        *(undefined4 *)(iVar7 + 0x1bc) = *(undefined4 *)(iVar7 + 0x18c);
        *(undefined4 *)(iVar7 + 0x1c0) = *(undefined4 *)(iVar7 + 400);
      }
      sVar11 = sVar11 + 1;
      iVar7 = (int)sVar11;
    } while (iVar7 < (int)puVar2[0x8b]);
  }
  if (puVar2[0x1c] != 0xffffffff) {
    object_placement_data_initialize(puVar2[0x1c],param_1);
    uVar8 = 3;
    if ((DAT_00719720 == 2) &&
       (*(int *)((&PTR_PTR_0069bfdc)
                 [**(short **)((local_88[0] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)] + 0x10) != -1))
    {
      uVar8 = 0;
    }
    uVar9 = object_new_with_datum_role_control(local_88,uVar8);
    if ((uVar9 != 0xffffffff) && (cVar5 = FUN_0056d400(2), cVar5 == '\0')) {
      iVar7 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc) + 4);
      if (iVar7 == 0) {
        object_delete_unparented();
      }
      else if (iVar7 != 3) goto LAB_00426ef0;
      object_delete_recursive(uVar9,0);
    }
  }
LAB_00426ef0:
  if ((short)puVar2[0x60] != -1) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    uVar9 = puVar2[0x60];
    iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
    local_94 = (char)(short)puVar2[0x74];
    pcVar1 = (char *)((short)uVar9 + 0x31e + iVar6);
    *pcVar1 = *pcVar1 + (char)(((int)(short)(*(short *)((int)puVar2 + 0x1d2) + 1) -
                               (int)(short)puVar2[0x74]) * (random_seed_global >> 0x10) >> 0x10) +
                        local_94;
    uVar10 = (undefined1)(short)uVar9;
    *(undefined1 *)(iVar6 + 0x31d) = uVar10;
    *(undefined1 *)(iVar6 + 0x31c) = uVar10;
  }
  iVar6 = DAT_0087bc14;
  uVar9 = puVar2[0x73];
  if (((uVar9 != 0xffffffff) &&
      (sVar11 = *(short *)(*(int *)((uVar9 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308),
      sVar11 != 0)) && (sVar11 != 6)) {
    object_placement_data_initialize(uVar9,param_1);
    uVar8 = 3;
    if ((DAT_00719720 == 2) &&
       (*(int *)((&PTR_PTR_0069bfdc)[**(short **)((local_88[0] & 0xffff) * 0x20 + 0x14 + iVar6)] +
                0x10) != -1)) {
      uVar8 = 0;
    }
    iVar6 = object_new_with_datum_role_control(local_88,uVar8);
    if ((iVar6 != -1) && (cVar5 = FUN_0056d1a0(param_1,iVar6,1), cVar5 == '\0')) {
      object_delete();
    }
  }
  if ((*puVar2 & 0x30) != 0) {
    if ((*puVar2 & 0x20) != 0) {
      *(uint *)(iVar4 + 0x204) = *(uint *)(iVar4 + 0x204) | 0x20;
    }
    *(uint *)(iVar4 + 0x204) = *(uint *)(iVar4 + 0x204) | 0x10;
    *(undefined4 *)(iVar4 + 0x37c) = 0x3f800000;
    if ((*pbVar3 & 0x20) != 0) {
      *(undefined4 *)(iVar4 + 0x380) = 0x3f800000;
      return;
    }
    *(undefined4 *)(iVar4 + 0x380) = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
