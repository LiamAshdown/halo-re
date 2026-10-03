// weapon_transfer_ammunition  (Ghidra: item_transfer_ammunition; renamed per
// items_types_notes.md, "walks the Weapon tag magazine block")
// address 0x4c2610, size 553 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/items.h weapon_data.magazines[], weapon_magazine_state (rounds_unloaded 0x06);
//   types/tags.h Weapon.magazines (0x4f0), WeaponMagazine (rounds_reserved_maximum 0x08,
//   magazine_objects TagReflexive 0x64), WeaponMagazineObject (rounds 0x00, equipment
//   TagDependency 0x0c, size 0x1c), Weapon.pickup_sound (tag_id 0x49c). Confirmed against the
//   binary with an offsetof probe against types/tags.h.
// register convention: all four Ghidra-recognized __cdecl parameters already match the source.
// blam-cc: stack -> (target_item_index, source_item_index, requesting_player_index,
//   out_transferred)
// UNSURE: *out_transferred is only ever written inside the per-magazine loop, so if more than
// one target magazine accepts a transfer this pass, only the last one's amount survives -- kept
// exactly as decompiled, not "fixed".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index sound_start_unspatialized(datum_index definition_index, float scale); // 0x543dd0, EDX, stack
extern void equipment_definition_play_pickup_sound(uint32_t equipment_tag_id); // 0x4bbbd0
extern void object_delete(uint32_t object_index); // 0x4f5bd0
extern void object_delete_unparented(datum_index object_index); // 0x4f5aa0
extern void object_delete_recursive(datum_index object_index, uint8_t recurse_siblings); // 0x4f59d0
extern void weapon_notify_ammo_pickup(datum_index item_index, int16_t magazine_index, int16_t rounds); // 0x4c2510

// Moves reserve ammunition from one item into another. If both items share the same weapon tag,
// rounds are moved magazine-for-magazine out of the source's own reserve; otherwise the target's
// magazine_objects list is searched for an entry naming the source's tag (a battery/energy-cell
// style pickup), and if found the whole source object is consumed. Returns non-zero (with the
// magazine count packed into the upper bytes, an artifact of the original code) once any
// magazine has been processed, and reports the amount actually moved through *out_transferred.
uint32_t weapon_transfer_ammunition(datum_index target_item_index, datum_index source_item_index,
    int16_t requesting_player_index, int16_t *out_transferred)
{
    object *target_obj;
    Weapon *target_tag;
    object *source_obj;
    datum_index source_definition_tag;
    uint8_t any_transferred;
    int16_t magazine_index;
    uint32_t result;

    target_obj = ((object_header *)object_data->data)[(uint16_t)target_item_index].data;
    target_tag = (Weapon *)tag_instances[(uint16_t)target_obj->definition_tag].data;
    source_obj = ((object_header *)object_data->data)[(uint16_t)source_item_index].data;
    source_definition_tag = source_obj->definition_tag;
    any_transferred = 0;
    result = 0;

    if (target_tag->magazines.count > 0) {
        for (magazine_index = 0; magazine_index < target_tag->magazines.count; magazine_index++) {
            WeaponMagazine *magazine_tag = (WeaponMagazine *)target_tag->magazines.pointer + magazine_index;
            weapon_data *target_wd = (weapon_data *)((uint8_t *)target_obj + k_item_extension_offset);
            int16_t *target_rounds_unloaded = &target_wd->magazines[magazine_index].rounds_unloaded;
            int16_t moved = 0;

            if (*target_rounds_unloaded < magazine_tag->rounds_reserved_maximum) {
                int16_t space_available = magazine_tag->rounds_reserved_maximum - *target_rounds_unloaded;

                if (target_obj->definition_tag == source_definition_tag) {
                    // Same weapon type: pull straight out of the source's own reserve.
                    weapon_data *source_wd = (weapon_data *)((uint8_t *)source_obj + k_item_extension_offset);
                    int16_t *source_rounds_unloaded = &source_wd->magazines[magazine_index].rounds_unloaded;

                    moved = space_available;
                    if (*source_rounds_unloaded <= space_available) {
                        moved = *source_rounds_unloaded;
                    }
                    if (moved > 0) {
                        *source_rounds_unloaded = *source_rounds_unloaded - moved;
                        if (*(datum_index *)&target_tag->pickup_sound.tag_id != (datum_index)0xffffffff &&
                            requesting_player_index != -1) {
                            // FIXED: EDX = the pickup sound (tag +0x49c, 0x4c26ff)
                            sound_start_unspatialized(*(datum_index *)&target_tag->pickup_sound.tag_id, 1.0f);
                        }
                        if (*source_rounds_unloaded == 0) {
                            object_delete(source_item_index);
                        }
                        if (source_obj->network_role == 0 &&
                            source_wd->magazines[magazine_index].state == _weapon_magazine_reloading) {
                            weapon_notify_ammo_pickup(source_item_index, magazine_index, moved);
                        }
                    }
                    any_transferred = 1;
                } else {
                    // Different weapon type: look for a magazine_objects entry naming the
                    // source's tag (a battery/energy-cell pickup) and consume the whole object.
                    int16_t object_index;

                    for (object_index = 0; object_index < magazine_tag->magazine_objects.count; object_index++) {
                        WeaponMagazineObject *magazine_object =
                            (WeaponMagazineObject *)magazine_tag->magazine_objects.pointer + object_index;

                        if (*(datum_index *)&magazine_object->equipment.tag_id == source_definition_tag) {
                            moved = space_available;
                            if (magazine_object->rounds <= space_available) {
                                moved = magazine_object->rounds;
                            }
                            if (moved > 0) {
                                int32_t source_role;

                                if (requesting_player_index != -1) {
                                    // blam-cc: EAX -> equipment_tag_id. objdump shows
                                    // `mov eax,[edx+0x18]` immediately before the call, i.e.
                                    // this magazine_object's equipment tag id.
                                    equipment_definition_play_pickup_sound(
                                        *(uint32_t *)&magazine_object->equipment.tag_id);
                                }
                                source_role = ((object_header *)object_data->data)[(uint16_t)source_item_index].data->network_role;
                                if (source_role == 0) {
                                    object_delete_unparented(source_item_index);
                                } else if (source_role != 3) {
                                    goto transferred; // UNSURE: original jumps out with the
                                                       // reserve delete skipped for role != 0,3
                                }
                                object_delete_recursive(source_item_index, 0);
                                goto transferred; // moved > 0: LAB_004c27fa, which also
                                                   // latches any_transferred
                            }
                            // moved <= 0: original keeps scanning subsequent magazine_objects
                        }
                    }
                    goto advance; // no magazine_object matched: the original reaches the
                                   // reserve update WITHOUT latching local_21
                }
            transferred:
                any_transferred = 1;
            advance:
                *target_rounds_unloaded = *target_rounds_unloaded + moved;
                *out_transferred = moved;
            }
        }
        result = (uint32_t)any_transferred | ((uint32_t)(uint16_t)magazine_index << 8);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4c2610):

uint __cdecl
item_transfer_ammunition
          (uint target_item_index,uint source_item_index,short unused_arg,short *out_transferred)

{
  ushort *puVar1;
  ushort uVar2;
  uint *puVar3;
  int iVar4;
  uint *puVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  uint uVar11;
  ushort *puVar12;
  short sVar13;
  uint uVar14;
  short sVar15;
  undefined1 local_21;
  int local_1c;

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (target_item_index & 0xffff) * 0xc);
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar8 = (source_item_index & 0xffff) * 0xc;
  puVar5 = *(uint **)(iVar8 + 8 + *(int *)(DAT_008603b0 + 0x34));
  uVar6 = *puVar5;
  iVar9 = 0;
  uVar14 = (uint)puVar5 & 0xffffff00;
  local_21 = 0;
  local_1c = 0;
  if (0 < *(int *)(iVar4 + 0x4f0)) {
    do {
      iVar10 = iVar9 * 0x70 + *(int *)(iVar4 + 0x4f4);
      puVar1 = (ushort *)(iVar9 * 0xc + 0x2b6 + (int)puVar3);
      if ((short)*puVar1 < (short)*(ushort *)(iVar10 + 8)) {
        uVar11 = (uint)*(ushort *)(iVar10 + 8) - (uint)*puVar1;
        uVar14 = 0;
        sVar13 = 0;
        if (*puVar3 == uVar6) {
          iVar7 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
          iVar10 = iVar7 + 0x2b0 + iVar9 * 0xc;
          sVar13 = *(short *)(iVar10 + 6);
          uVar14 = uVar11;
          if (sVar13 <= (short)uVar11) {
            uVar14 = CONCAT22((short)((uint)iVar7 >> 0x10),sVar13);
          }
          if (0 < (short)uVar14) {
            *(short *)(iVar10 + 6) = sVar13 - (short)uVar14;
            if ((*(int *)(iVar4 + 0x49c) != -1) && (unused_arg != -1)) {
              FUN_00543dd0(0x3f800000);
            }
            if (*(short *)(iVar10 + 6) == 0) {
              object_delete();
            }
            if ((puVar3[1] == 0) && ((short)puVar3[iVar9 * 3 + 0xac] == 1)) {
              FUN_004c2510(local_1c,uVar14);
            }
          }
LAB_004c27fa:
          sVar13 = (short)uVar14;
          local_21 = 1;
        }
        else {
          sVar15 = 0;
          if (0 < *(int *)(iVar10 + 100)) {
            iVar9 = 0;
            do {
              puVar12 = (ushort *)(iVar9 * 0x1c + *(int *)(iVar10 + 0x68));
              if (*(uint *)(puVar12 + 0xc) == uVar6) {
                uVar2 = *puVar12;
                uVar14 = uVar11;
                if ((short)uVar2 <= (short)uVar11) {
                  uVar14 = (uint)uVar2;
                }
                if (0 < (short)uVar14) {
                  if (unused_arg != -1) {
                    FUN_004bbbd0();
                  }
                  iVar9 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8) + 4);
                  if (iVar9 == 0) {
                    object_delete_unparented();
                  }
                  else if (iVar9 != 3) goto LAB_004c27fa;
                  object_delete_recursive(source_item_index,0);
                  goto LAB_004c27fa;
                }
              }
              sVar13 = (short)uVar14;
              sVar15 = sVar15 + 1;
              iVar9 = (int)sVar15;
            } while (iVar9 < *(int *)(iVar10 + 100));
          }
        }
        *puVar1 = *puVar1 + sVar13;
        *out_transferred = sVar13;
      }
      local_1c = local_1c + 1;
      iVar9 = (int)(short)local_1c;
    } while (iVar9 < *(int *)(iVar4 + 0x4f0));
    uVar14 = CONCAT31((int3)((uint)local_1c >> 8),local_21);
  }
  return uVar14;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
