// players_any_pending_seat_or_respawn  (Ghidra: FUN_00475090; named per this rewrite)
// address 0x475090, size 354 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Scans all players and returns whether any is in a
//   pending state requiring respawn/seat handling"); types/objects.h object (type +0xb4, flags
//   +0x10, parent_object +0x11c, _object_type_biped/_object_type_vehicle,
//   _object_mask_vehicle); types/units.h vehicle_data::airborne_ticks (+0x4d0); types/tags.h
//   Item::item_flags (tag+0x17c, bit 0x40 unnamed). objdump -d -M intel
//   --start-address=0x475090 --stop-address=0x4751f7 bin/halo.exe pins every register and the
//   shared tail the "vehicle with no parent" and "attached to a vehicle whose tag has item_flags
//   bit 0x40" branches jump into together.
// register convention: no arguments; return value in EAX.
//
// UNSURE: biped_is_idle_eligible (units module, not in this batch) and the Item::item_flags bit 0x40 test
// are both unnamed; the whole function's real intent (why an unparented biped defers to
// biped_is_idle_eligible, an unparented vehicle or one seated in a bit-0x40 vehicle checks its own or its
// parent's airborne_ticks against 2) is inferred only from the field types, not independently
// confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern uint8_t biped_is_idle_eligible(datum_index unit_handle); // 0x55e8e0, units module, not in this
    // batch; blam-cc: EAX -> unit_handle; UNSURE purpose

// For every player with a unit: if that unit's root ancestor (walking parent_object) has object
// flags bit 0x200000 set, returns true immediately. Otherwise, if the unit itself has no parent:
// a biped defers to biped_is_idle_eligible (true if it says so); a vehicle falls into the shared tail
// below testing its OWN airborne_ticks. If the unit does have a parent, and that parent is a
// vehicle (_object_mask_vehicle) whose tag's Item::item_flags has bit 0x40 set, falls into the
// same shared tail testing the PARENT's airborne_ticks: true if greater than 2. Returns false if
// no player matches any of this.
uint8_t players_any_pending_seat_or_respawn(void)
{
    data_iterator iter;
    player *plr;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;

    plr = (player *)data_iterator_next(&iter);
    while (plr != (player *)0) {
        if (plr->unit != (datum_index)-1) {
            object *unit_obj = ((object_header *)object_data->data)[plr->unit & 0xffff].data;
            datum_index walk = plr->unit;
            datum_index root;
            object *root_obj;
            object *airborne_check_obj;

            do {
                root = walk;
                walk = ((object_header *)object_data->data)[root & 0xffff].data->parent_object;
            } while (walk != (datum_index)-1);
            root_obj = ((object_header *)object_data->data)[root & 0xffff].data;
            if ((root_obj->flags & 0x200000) != 0) {
                return 1;
            }

            airborne_check_obj = (object *)0;
            if (unit_obj->parent_object == (datum_index)-1) {
                if (unit_obj->type == _object_type_biped) {
                    if (biped_is_idle_eligible(plr->unit) != 0) {
                        return 1;
                    }
                } else if (unit_obj->type == _object_type_vehicle) {
                    airborne_check_obj = unit_obj;
                }
            } else {
                object_header *parent_header = 0;
                int16_t parent_index = (int16_t)unit_obj->parent_object;
                if (parent_index >= 0 && parent_index < object_data->maximum_count) {
                    object_header *candidate = &((object_header *)object_data->data)[parent_index];
                    int16_t parent_salt = (int16_t)((uint32_t)unit_obj->parent_object >> 16);
                    if (candidate->identifier != 0 &&
                        (parent_salt == 0 || candidate->identifier == parent_salt)) {
                        parent_header = candidate;
                    }
                }
                if (parent_header != 0 && (1u << (parent_header->type & 0x1f) & _object_mask_vehicle) != 0 &&
                    parent_header->data != 0) {
                    Item *parent_tag = (Item *)tag_instances[parent_header->data->definition_tag & 0xffff].data;
                    if ((parent_tag->item_flags & 0x40) != 0) { // UNSURE: unnamed ItemFlags bit
                        airborne_check_obj = parent_header->data;
                    }
                }
            }

            if (airborne_check_obj != (object *)0 &&
                *((uint8_t *)airborne_check_obj + 0x4d0) > 2) {
                return 1;
            }
        }
        plr = (player *)data_iterator_next(&iter);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x475090), from tools/pack.py 0x475090:

undefined4 FUN_00475090(void)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  char cVar6;
  int iVar7;
  int iVar8;
  uint *puVar9;
  short sVar10;
  short sVar11;
  int iVar12;

  iVar7 = data_iterator_next();
  iVar5 = DAT_0087bc14;
  iVar4 = DAT_008603b0;
  do {
    if (iVar7 == 0) {
      return 0;
    }
    uVar1 = *(uint *)(iVar7 + 0x34);
    if (uVar1 != 0xffffffff) {
      iVar7 = *(int *)(iVar4 + 0x34);
      puVar9 = *(uint **)(iVar7 + 8 + (uVar1 & 0xffff) * 0xc);
      uVar3 = 0xffffffff;
      while (uVar2 = uVar1, uVar2 != 0xffffffff) {
        uVar3 = uVar2;
        uVar1 = *(uint *)(*(int *)(iVar7 + 8 + (uVar2 & 0xffff) * 0xc) + 0x11c);
      }
      if ((*(uint *)(*(int *)(iVar7 + 8 + (uVar3 & 0xffff) * 0xc) + 0x10) & 0x200000) != 0) {
        return 1;
      }
      uVar1 = puVar9[0x47];
      if (uVar1 == 0xffffffff) {
        if ((short)puVar9[0x2d] == 0) {
          cVar6 = FUN_0055e8e0();
          if (cVar6 != '\0') {
            return 1;
          }
        }
        else if ((short)puVar9[0x2d] == 1) goto LAB_004751dd;
      }
      else {
        iVar12 = 0;
        sVar10 = (short)uVar1;
        if ((-1 < sVar10) && (sVar10 < *(short *)(iVar4 + 0x20))) {
          iVar8 = (int)*(short *)(iVar4 + 0x22) * (int)sVar10;
          sVar10 = *(short *)(iVar8 + iVar7);
          if ((sVar10 != 0) &&
             ((sVar11 = (short)(uVar1 >> 0x10), sVar11 == 0 || (sVar10 == sVar11)))) {
            iVar12 = iVar8 + iVar7;
          }
        }
        if ((((iVar12 != 0) && ((1 << (*(byte *)(iVar12 + 3) & 0x1f) & 2U) != 0)) &&
            (puVar9 = *(uint **)(iVar12 + 8), puVar9 != (uint *)0x0)) &&
           ((*(byte *)(*(int *)((*puVar9 & 0xffff) * 0x20 + 0x14 + iVar5) + 0x17c) & 0x40) != 0)) {
LAB_004751dd:
          if (2 < (byte)puVar9[0x134]) {
            return 1;
          }
        }
      }
    }
    iVar7 = data_iterator_next();
  } while( true );
}
#endif
