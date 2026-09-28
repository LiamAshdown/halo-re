p = "C:\\Users\\Liam-\\halo-re\\src\\units\\unit_scripting_set_or_drop_weapon.c"
t = open(p, encoding="utf-8").read()
cut = t.index("\n#if 0")
new = '''// unit_scripting_set_or_drop_weapon  (Ghidra: FUN_0056ddb0)
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

extern uint8_t *network_message_table; // 0x00687130, the pooled-node table (+0x28: key -> object index)

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
    keys = *(int32_t **)(network_message_table + 0x28);
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
    if (unit_get_weapon_object_index(unit_index, *(int16_t *)(unit + 0x2f2)) != weapon) {
        for (i = 0; i < 4; i++) {
            if (((datum_index *)(unit + 0x2f8))[i] == weapon) {
                *(int16_t *)(unit + 0x2f4) = (int16_t)i;
                unit_ready_desired_weapon(unit_index, 1);
                break;
            }
        }
    }
    current_index = *(int16_t *)(unit + 0x2f2);
    if (current_index != -1) {
        current = ((datum_index *)(unit + 0x2f8))[current_index];
    }
    if (current == weapon) {
        unit_drop_current_weapon(unit_index, (uint8_t)decoded.force);
    }
}
'''
open(p, "w", encoding="utf-8", newline="\n").write(new + t[cut:])
print("ok")
