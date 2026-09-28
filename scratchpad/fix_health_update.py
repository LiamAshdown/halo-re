p = "C:\\Users\\Liam-\\halo-re\\src\\units\\unit_apply_network_health_update.c"
t = open(p, encoding="utf-8").read()
inc = t.index("#include")
cut = t.index("\n#if 0")
head = t[:inc].rstrip("\n") + '''
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x55b5f0..0x55b77b): the object is
// object_try_and_get(object_index, 1) (the C looked up -1). The 16-byte network block +0x52c (grenade counts, body
// vitality, shield vitality, stunned byte) is copied into a local and the message decodes INTO that local
// (0x4ec590, or 0x4ec600 with the block as baseline for a reliable message); the previous C decoded into nothing
// and applied the stale copy. Staleness guard, sequence bytes (+0x527/+0x528 against record +4/+5), the full-update
// write-back (record +6), shield x3 (0x672c3c) applied when record +7 is 1, the baseline copy at +0x540 and the
// flags are as the original.
'''
if "rewrite confidence: 0.85" not in head:
    head = head.replace("rewrite confidence: ", "rewrite confidence: 0.85 (REWRITTEN; was ", 1)
    lines = head.split("\n")
    for i, l in enumerate(lines):
        if "(REWRITTEN; was " in l:
            lines[i] = l + ")"
            break
    head = "\n".join(lines)
body = '''
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

extern data_array *object_data; // 0x008603b0

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, index in ECX, mask on the stack
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_forced(void *decode_context, void *destination,
    int32_t changed_offset, uint8_t force); // 0x4ec600, EAX context, ECX destination, EDX baseline, stack force
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message

typedef struct biped_network_health_block {
    uint32_t grenade_counts;       // 0x00 (+0x52c), low word used
    uint32_t body_vitality;        // 0x04 (+0x530)
    real shield_vitality;          // 0x08 (+0x534)
    uint32_t shield_stunned;       // 0x0c (+0x538), low byte used
} biped_network_health_block;

void unit_apply_network_health_update(uint32_t object_index, void *message)
{
    uint8_t *unit = (uint8_t *)object_try_and_get((datum_index)object_index, 1);
    uint8_t *guard;
    uint8_t *record;
    int32_t reliable;
    biped_network_health_block block;
    uint8_t accepted;
    real shield;

    if (unit == 0) {
        message_delta_decode_compound_field_staged(message);
        return;
    }
    guard = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    record = (uint8_t *)((void **)message)[0x11];
    reliable = **(int32_t **)message == 1;
    if ((*(uint32_t *)(guard + 0x10) & 0x8000000) != 0 && reliable) {
        int32_t incoming = record[5];
        int32_t current = unit[0x528];

        if (record[4] != unit[0x527] || (incoming <= current && incoming - current + 0xff >= 0x1e)) {
            message_delta_decode_compound_field_staged(message);
            return;
        }
    }
    memcpy(&block, unit + 0x52c, sizeof(block));
    if (reliable) {
        accepted = message_delta_decode_compound_field_forced(message, &block, (int32_t)(unit + 0x52c), 0);
    } else {
        accepted = message_delta_decode_compound_field(message, &block);
    }
    if (!accepted) {
        return;
    }
    unit[0x528] = record[5];
    *(uint32_t *)(unit + 0x10) |= 0x8000000;
    if (record[6] != 0) {
        unit[0x527] = record[4];
        memcpy(unit + 0x52c, &block, sizeof(block));
    }
    shield = block.shield_vitality * 3.0f;
    *(int16_t *)(unit + 0x31e) = (int16_t)block.grenade_counts;
    *(uint32_t *)(unit + 0xe0) = block.body_vitality;
    if (record[7] == 1) {
        *(real *)(unit + 0xe4) = shield;
    }
    *(uint32_t *)(unit + 0x540) = block.grenade_counts;
    *(uint32_t *)(unit + 0x544) = block.body_vitality;
    *(real *)(unit + 0x548) = shield;
    *(uint32_t *)(unit + 0x54c) = block.shield_stunned;
    *(int16_t *)(unit + 0x104) = (uint8_t)block.shield_stunned == 1;
    unit[0x475] = 1;
    unit[0x53c] = 1;
}
'''
open(p, "w", encoding="utf-8", newline="\n").write(head + body + t[cut:])
print("ok")
