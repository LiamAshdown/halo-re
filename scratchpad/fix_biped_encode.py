p = "C:\\Users\\Liam-\\halo-re\\src\\units\\unit_build_network_update.c"
t = open(p, encoding="utf-8").read()
inc = t.index("#include")
cut = t.index("\n#if 0")
head = t[:inc].rstrip("\n") + '''
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly 0x55aed0..0x55b103: the biped creation encoder
// (biped type +0x64 hook; network action 0x1d), the counterpart of vehicle_encode_network_create 0x571f20 and the
// exact inverse of unit_network_create_update_apply 0x55b110. Arguments (object index, buffer, bit budget); the 0x8c
// byte record -- definition, network key (a new index-cache slot 0x6870d8 when unknown), owner team, machine key
// (0x687558 table), creator key (0 when unknown), position +0x5c, forward +0x74, up +0x80, velocity +0x68, the
// 0x30 bytes at +0x188, unit flag 0x80000, +0x344, update sequence +0x527, grenade counts +0x52c (at record +0x7d),
// body / shield vitality +0x530 / +0x534, stunned +0x538 -- is encoded with message_delta_encode_message (EAX buffer,
// EDX budget; type 0x1d, one item) and its bit count returned. The previous C packed an invented layout and passed
// the encoder neither buffer nor budget.
'''
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

extern data_array *object_data;                    // 0x008603b0
extern uint8_t *object_pooled_node_globals;        // 0x00687130, network-id hash_table at +0x0c
extern uint8_t *machine_table;                     // 0x00687558, hash_table at +0x0c
extern uint8_t network_object_index_cache[];       // 0x006870d8
extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, ESI table, ECX key
extern int32_t network_index_cache_find_or_allocate_slot(uint8_t *container, int32_t key); // 0x4e9c20, EAX container
extern int32_t message_delta_encode_message(int32_t buffer, int32_t bit_budget, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX, EDX

typedef struct biped_network_create_record {
    datum_index definition;        // 0x00
    int32_t network_key;           // 0x04
    int16_t owner_team;            // 0x08
    int16_t pad_0a;
    int32_t machine_key;           // 0x0c
    int32_t creator_key;           // 0x10
    uint8_t position[12];          // 0x14 <- +0x5c
    uint8_t forward[12];           // 0x20 <- +0x74
    uint8_t up[12];                // 0x2c <- +0x80
    uint8_t velocity[12];          // 0x38 <- +0x68
    uint8_t block_188[0x30];       // 0x44 <- +0x188
    uint8_t flag_80000;            // 0x74
    uint8_t pad_75[3];
    uint32_t scalar_344;           // 0x78
    uint8_t update_sequence;       // 0x7c
    uint8_t grenade_counts[2];     // 0x7d
    uint8_t pad_7f;
    uint32_t body_vitality;        // 0x80
    uint32_t shield_vitality;      // 0x84
    uint8_t shield_stunned;        // 0x88
    uint8_t pad_89[3];
    uint32_t zero_8c;              // 0x8c, cleared before the encode (0x55b0e5)
} biped_network_create_record;

int32_t unit_build_network_update(uint32_t object_index, int32_t buffer, int32_t bit_budget)
{
    uint8_t *biped = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    hash_table *keys = (hash_table *)(object_pooled_node_globals + 0xc);
    biped_network_create_record record;
    void *item = &record;
    int32_t key = 0;
    int32_t creator = 0;
    int32_t machine = 0;

    if (object_index != 0xffffffff) {
        key = hash_table_get(keys, (int32_t)object_index);
    }
    if (*(int32_t *)(biped + 0xc4) != -1) {
        creator = hash_table_get(keys, *(int32_t *)(biped + 0xc4));
        if (creator == -1) {
            creator = 0;
        }
    }
    if (*(int32_t *)(biped + 0xc0) != -1) {
        machine = hash_table_get((hash_table *)(machine_table + 0xc), *(int32_t *)(biped + 0xc0));
        if (machine == -1) {
            machine = 0;
        }
    }
    if (key == -1) {
        key = network_index_cache_find_or_allocate_slot(network_object_index_cache, (int32_t)object_index);
    }
    record.definition = *(datum_index *)biped;
    record.network_key = key;
    record.owner_team = *(int16_t *)(biped + 0xb8);
    record.machine_key = machine;
    record.creator_key = creator;
    memcpy(record.forward, biped + 0x74, 12);
    memcpy(record.up, biped + 0x80, 12);
    memcpy(record.position, biped + 0x5c, 12);
    memcpy(record.velocity, biped + 0x68, 12);
    memcpy(record.block_188, biped + 0x188, 0x30);
    record.flag_80000 = (uint8_t)((*(uint32_t *)(biped + 0x204) >> 0x13) & 1);
    record.scalar_344 = *(uint32_t *)(biped + 0x344);
    record.update_sequence = biped[0x527];
    record.body_vitality = *(uint32_t *)(biped + 0x530);
    record.shield_vitality = *(uint32_t *)(biped + 0x534);
    record.shield_stunned = biped[0x538];
    memcpy(record.grenade_counts, biped + 0x52c, 2);
    record.zero_8c = 0;
    return message_delta_encode_message(buffer, bit_budget, 0, 0x1d, 0, &item, 0, 1, 0);
}
'''
open(p, "w", encoding="utf-8", newline="\n").write(head + body + t[cut:])
print("ok")
