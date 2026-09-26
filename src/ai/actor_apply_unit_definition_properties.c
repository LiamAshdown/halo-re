// actor_apply_unit_definition_properties  (Ghidra: actor_apply_unit_definition_properties, renamed)
// address 0x426cf0, size 901 bytes
// name confidence: 0.4   rewrite confidence: 0.15
// evidence: phase-4 summary "Applies a unit definition's AI-related properties (grenade
// timing, notice range, attached weapon/child objects, shield flags) to a newly placed unit
// object." types/objects.h object_placement_data (already established, canonical form in
// src/game/cheat_spawn_objects_near_camera.c) and object_type_role_table (0x0069bfdc,
// likewise). Calls object_new_with_datum_role_control / object_placement_data_initialize
// (both established), object_initialize_shield_stun_thresholds, color_interpolate,
// object_delete/object_delete_recursive/object_delete_unparented, and unit_try_select_equipment/
// unit_pickup_weapon, none independently confirmed here.
//   UNSURE: this is one of the least-confident rewrites in this pass. Every dword-indexed
//   read off the ActorVariant tag data (puVar2[...]) and the Actor tag data (pbVar3) is kept
//   as a raw offset -- neither types/tags.h's ActorVariant nor Actor struct is broken out to
//   the individual fields this function reaches (shield thresholds, grenade velocity/count
//   ranges, a child-object tag reference, a "cannot see" random-color table, a parented
//   weapon tag reference). The object_placement_data local (`local_88`) this function builds
//   before calling object_new_with_datum_role_control is likewise not shown in full by
//   Ghidra (only its first dword, used for object_type_role_table's lookup, and the pointer
//   itself passed to the two established helpers); modeled as calling
//   object_placement_data_initialize to fill it rather than re-deriving each field.
// register convention: EAX -> actor_variant_tag, stack -> unit_index (Ghidra's own "param_1").
//   // blam-cc: EAX -> actor_variant_tag, stack -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global; // 0x00719cd0
extern int32_t map_difficulty_or_kind; // 0x00719720, UNSURE name (compared against 2 here and in actor_place_new_unit)
extern void **object_type_role_table; // 0x0069bfdc

extern void object_initialize_shield_stun_thresholds(datum_index object_index); // 0x4ed440, UNSURE signature
extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t);
    // 0x43f6a0, blam-cc: EAX -> color1, ECX -> color0, stack -> dest, flags, t
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag, datum_index role); // 0x4f53a0
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern void object_delete(datum_index object_index); // 0x4f5bd0, UNSURE signature
extern void object_delete_recursive(datum_index object_index, uint32_t flag); // 0x4f59d0, UNSURE signature
extern void object_delete_unparented(datum_index object_index); // 0x4f5aa0, UNSURE signature
extern uint8_t unit_pickup_weapon(int32_t param); // 0x56d400, UNSURE signature
extern uint8_t unit_try_select_equipment(datum_index unit_index, datum_index child_object_index, int32_t param); // 0x56d1a0, UNSURE signature

// blam-cc: EAX -> actor_variant_tag, stack -> unit_index
// Applies a unit definition's AI-related properties to a newly placed unit object: shield
// stun thresholds, a notice-range override, up to four randomized "cannot see" colors,
// spawning an optional attached weapon object (retrying with a fallback role in single
// player), a randomized grenade-count seed, spawning an optional parented child object, and,
// for units that carry either of two type flags, initializing extra shield-related fields.
void actor_apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index)
{
    const uint8_t *variant_tag_data = (const uint8_t *)(tag_instances[actor_variant_tag & 0xffff].data);
    const uint32_t *variant = (const uint32_t *)variant_tag_data;
    const uint8_t *actor_tag_data = (const uint8_t *)(tag_instances[variant[4] & 0xffff].data); // ActorVariant.actor_definition.tag_id
    object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    object_placement_data placement;

    if (*(const float *)&variant[0x80] > 0.0f || *(const float *)&variant[0x81] > 0.0f) {
        object_initialize_shield_stun_thresholds(unit_index);
    }
    if ((int16_t)variant[0x83] != 0) {
        *(int16_t *)((uint8_t *)unit_object + 0x176) = (int16_t)variant[0x83]; // UNSURE offset
    }

    {
        int32_t i;
        for (i = 0; i < (int32_t)variant[0x8b] && i < 4; i++) {
            uint8_t *slot = (uint8_t *)unit_object + 0x188 + i * 0xc; // UNSURE offset/stride
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            // 0x426df1..0x426e0b: EAX = the variant change color's +0xc color, ECX = its +0 color (entries of 0x20 at
            // variant +0x230), stack: the unit's working color, 1, the random fraction
            {
                uint8_t *change_color = *(uint8_t **)((uint8_t *)variant + 0x230) + i * 0x20;

                color_interpolate((ColorRGB *)(change_color + 0xc), (ColorRGB *)change_color, (ColorRGB *)slot, 1,
                    (float)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f);
            }
            *(uint32_t *)(slot + 0x30) = *(uint32_t *)slot;
            *(uint32_t *)(slot + 0x34) = *(uint32_t *)(slot + 4);
            *(uint32_t *)(slot + 0x38) = *(uint32_t *)(slot + 8);
        }
    }

    if (variant[0x1c] != 0xffffffff) {
        uint32_t role = 3;
        datum_index new_object;

        object_placement_data_initialize(&placement, (datum_index)variant[0x1c], unit_index);
        if (map_difficulty_or_kind == 2 &&
            *(int32_t *)((uint8_t *)object_type_role_table
                 [*(const int16_t *)(tag_instances[placement.definition_tag & 0xffff].data)] +
             0x10) != -1) {
            // The original indexes object_type_role_table by the OBJECT TYPE stored in the
            // first int16 of the placement definition's tag data
            // (Ghidra: (&PTR_PTR_0069bfdc)[**(short **)((local_88[0] & 0xffff) * 0x20 +
            // 0x14 + DAT_0087bc14)]), not by the tag index itself.
            role = 0;
        }
        new_object = object_new_with_datum_role_control(&placement, role);
        if (new_object != (datum_index)k_datum_index_none && unit_pickup_weapon(2) == 0) {
            object *new_obj = ((object_header *)object_data->data)[new_object & 0xffff].data;
            int32_t network_role = new_obj->network_role;
            if (network_role == 0) {
                object_delete_unparented(new_object);
                object_delete_recursive(new_object, 0);
            } else if (network_role == 3) {
                object_delete_recursive(new_object, 0);
            }
        }
    }

    if ((int16_t)variant[0x60] != -1) {
        uint32_t grenade_count = variant[0x60];
        uint8_t min_count = (uint8_t)(int16_t)variant[0x74]; // UNSURE offset
        int16_t max_count = *(const int16_t *)((const uint8_t *)variant + 0x1d2); // UNSURE offset
        uint8_t roll;
        uint8_t *grenade_slot;

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        grenade_slot = (uint8_t *)unit_object + (int16_t)grenade_count + 0x31e;
        *grenade_slot = *grenade_slot + (uint8_t)((((int32_t)(int16_t)(max_count + 1) - (int16_t)variant[0x74]) *
                                                   (int32_t)(random_seed_global >> 0x10)) >> 0x10) + min_count;
        roll = (uint8_t)(int16_t)grenade_count;
        *((uint8_t *)unit_object + 0x31d) = roll;
        *((uint8_t *)unit_object + 0x31c) = roll;
    }

    {
        datum_index weapon_tag = (datum_index)variant[0x73]; // UNSURE offset
        if (weapon_tag != (datum_index)k_datum_index_none) {
            int16_t weapon_class = *(const int16_t *)((const uint8_t *)(tag_instances[weapon_tag & 0xffff].data) + 0x308); // UNSURE offset
            if (weapon_class != 0 && weapon_class != 6) {
                uint32_t role = 3;
                datum_index new_object;

                object_placement_data_initialize(&placement, weapon_tag, unit_index);
                if (map_difficulty_or_kind == 2 &&
                    *(int32_t *)((uint8_t *)object_type_role_table
                 [*(const int16_t *)(tag_instances[placement.definition_tag & 0xffff].data)] +
             0x10) != -1) {
            // The original indexes object_type_role_table by the OBJECT TYPE stored in the
            // first int16 of the placement definition's tag data
            // (Ghidra: (&PTR_PTR_0069bfdc)[**(short **)((local_88[0] & 0xffff) * 0x20 +
            // 0x14 + DAT_0087bc14)]), not by the tag index itself.
                    role = 0;
                }
                new_object = object_new_with_datum_role_control(&placement, role);
                if (new_object != (datum_index)k_datum_index_none && unit_try_select_equipment(unit_index, new_object, 1) == 0) {
                    object_delete(new_object);
                }
            }
        }
    }

    if ((*(const uint32_t *)variant_tag_data & 0x30) != 0) {
        if ((*(const uint32_t *)variant_tag_data & 0x20) != 0) {
            *(uint32_t *)((uint8_t *)unit_object + 0x204) |= 0x20; // UNSURE offset
        }
        *(uint32_t *)((uint8_t *)unit_object + 0x204) |= 0x10;
        *(float *)((uint8_t *)unit_object + 0x37c) = 1.0f; // UNSURE offset
        if ((*actor_tag_data & 0x20) != 0) {
            *(float *)((uint8_t *)unit_object + 0x380) = 1.0f; // UNSURE offset
        } else {
            *(float *)((uint8_t *)unit_object + 0x380) = 0.0f;
        }
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
