// unit_scripting_set_or_drop_weapon  (Ghidra: FUN_0056ddb0)
// address 0x56ddb0, size 270 bytes, name confidence 0.3, rewrite confidence 0.85
// functions.md: "Script/console-callable helper that resolves weapon-name arguments to object
// ids and updates or drops the unit's selected weapon accordingly."
// blam-cc: EAX -> message (the decode context)
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: the network action (dispatcher type 0x1b)
// decodes (0x4ec590, EAX context, ECX destination) into a 12-byte local record: +0 the unit's network key (0 =
// none), +4 the weapon's network key (0 = none), +8 the drop's force flag. Both keys map through the pooled-node
// table (0x687130 +0x28). If the unit's current weapon (unit_get_weapon_object_index with its current index +0x2f2)
// is not that weapon, the weapon's slot among +0x2f8[4] becomes the desired index (+0x2f4) and is readied; then,
// if the current weapon is that weapon, it is dropped. A reliable message (**message != 0) is rejected through
// 0x4ec670. The previous C took the keys and the flag as caller parameters, which the dispatcher never passes, and
// asked for weapon slot 0 instead of the current one.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t *object_network_id_table; // 0x00687130, the pooled-node table (+0x28: key -> object index)

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970, EAX, CX
extern void unit_ready_desired_weapon(uint32_t unit_index, uint8_t force); // 0x56d6e0, stack (unit, force)
extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0

typedef struct unit_set_or_drop_weapon_message {
    int32_t unit_key;      // 0x00
    int32_t weapon_key;    // 0x04
    int32_t force;         // 0x08, pushed whole as the drop's force argument
} unit_set_or_drop_weapon_message;

void unit_scripting_set_or_drop_weapon(int32_t *message)
{
    unit_set_or_drop_weapon_message decoded;
    int32_t *keys;
    uint32_t unit_index;
    uint8_t *unit;
    datum_index weapon = k_datum_index_none;
    datum_index current = k_datum_index_none;
    int16_t current_index;
    int32_t i;

    if (*(int32_t *)*message != 0) {
        message_delta_decode_compound_field_staged(message);
        return;
    }
    if (message_delta_decode_compound_field(message, &decoded) == 0 || decoded.unit_key == 0) {
        return;
    }
    keys = *(int32_t **)(object_network_id_table + 0x28);
    unit_index = (uint32_t)keys[decoded.unit_key];
    if (unit_index == 0xffffffff) {
        return;
    }
    unit = (uint8_t *)object_try_and_get(unit_index, 3);
    if (unit == 0) {
        return;
    }
    if (decoded.weapon_key != 0) {
        weapon = (datum_index)keys[decoded.weapon_key];
    }
    if (unit_get_weapon_object_index(unit_index, ((unit_object *)unit)->unit.current_weapon_index) != weapon) {
        for (i = 0; i < 4; i++) {
            if (((datum_index *)(unit + 0x2f8))[i] == weapon) {
                ((unit_object *)unit)->unit.desired_weapon_index = (int16_t)i;
                unit_ready_desired_weapon(unit_index, 1);
                break;
            }
        }
    }
    current_index = ((unit_object *)unit)->unit.current_weapon_index;
    if (current_index != -1) {
        current = ((datum_index *)(unit + 0x2f8))[current_index];
    }
    if (current == weapon) {
        unit_drop_current_weapon(unit_index, (uint8_t)decoded.force);
    }
}

#if 0
Original Ghidra decompilation (0x56ddb0):

void FUN_0056ddb0(void)

{
  short sVar1;
  uint uVar2;
  char cVar3;
  undefined4 *in_EAX;
  int iVar4;
  int iVar5;
  int *piVar6;
  int iVar7;
  int local_c;
  int local_8;
  undefined4 local_4;

  if (*(int *)*in_EAX == 0) {
    cVar3 = FUN_004ec590();
    if ((cVar3 != '\0') && (local_c != 0)) {
      iVar5 = *(int *)(PTR_DAT_00687130 + 0x28);
      uVar2 = *(uint *)(iVar5 + local_c * 4);
      if ((uVar2 != 0xffffffff) && (iVar4 = object_try_and_get(3), iVar4 != 0)) {
        iVar7 = -1;
        if (local_8 != 0) {
          iVar7 = *(int *)(iVar5 + local_8 * 4);
        }
        iVar5 = unit_get_weapon_object_index();
        if (iVar5 != iVar7) {
          iVar5 = 0;
          piVar6 = (int *)(iVar4 + 0x2f8);
          do {
            if (*piVar6 == iVar7) {
              *(short *)(iVar4 + 0x2f4) = (short)iVar5;
              unit_ready_desired_weapon(uVar2,1);
              break;
            }
            iVar5 = iVar5 + 1;
            piVar6 = piVar6 + 1;
          } while (iVar5 < 4);
        }
        iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
        sVar1 = *(short *)(iVar5 + 0x2f2);
        iVar4 = -1;
        if (sVar1 != -1) {
          iVar4 = *(int *)(iVar5 + 0x2f8 + sVar1 * 4);
        }
        if (iVar4 == iVar7) {
          unit_drop_current_weapon(uVar2,local_4);
          return;
        }
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
