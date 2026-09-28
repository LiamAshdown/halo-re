p = "C:\\Users\\Liam-\\halo-re\\src\\units\\unit_network_create_update_apply.c"
t = open(p, encoding="utf-8").read()
cut = t.index("\n#if 0")
head_comment = t[:t.index("#include")]
new = head_comment.rstrip("\n") + '''
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x55b110..0x55b3c8): the biped creation
// receiver (network action 0x1d), the counterpart of the vehicle one (0x572110). The message decodes (0x4ec590, EAX
// context, ECX destination; the original tests == 1) into a 0x8c-byte record: definition, network key, owner
// team, machine and creator keys, position, forward, up, a vector for placement +0x28, a 0x30-byte block for
// placement +0x58, then the unit flag 0x80000 (+0x204), the +0x344 scalar, the update sequence (+0x527), the grenade
// counts (+0x52c, an unaligned word at record +0x7d), body vitality (+0x530), shield vitality (+0x534) and the
// shield-stunned byte (+0x538). Forward/up are re-orthonormalized, the keys resolved (0x687130 / 0x687558), the
// biped created from the placement (role 1) and given the network key, and the network block and the live fields
// derived from it are written. The previous C decoded into nothing and wrote zeroed locals.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

extern data_array *object_data;                    // 0x008603b0
extern uint8_t *object_pooled_node_globals;        // 0x00687130, +0x28: network key -> object index
extern uint8_t *machine_table;                     // 0x00687558, +0x28: machine key -> index
extern uint8_t network_object_index_cache[];       // 0x006870d8

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern uint8_t network_index_cache_insert_if_free(uint8_t *container, int32_t slot, int32_t key); // 0x4e9cd0, EAX container, ECX key, stack slot

typedef struct biped_network_create_message {
    datum_index definition;        // 0x00
    int32_t network_key;           // 0x04
    int16_t owner_team;            // 0x08
    int16_t pad_0a;
    int32_t machine_key;           // 0x0c
    int32_t creator_key;           // 0x10
    real_point3d position;         // 0x14 -> placement +0x18
    real_vector3d forward;         // 0x20 -> placement +0x34
    real_vector3d up;              // 0x2c -> placement +0x40
    real_vector3d vector_38;       // 0x38 -> placement +0x28
    uint8_t block_44[0x30];        // 0x44 -> placement +0x58
    uint8_t flag_80000;            // 0x74 -> unit flags (+0x204) bit 0x80000
    uint8_t pad_75[3];
    uint32_t scalar_344;           // 0x78 -> +0x344
    uint8_t update_sequence;       // 0x7c -> +0x527
    uint8_t grenade_counts[2];     // 0x7d -> +0x52c (unaligned int16)
    uint8_t pad_7f;
    uint32_t body_vitality;        // 0x80 -> +0x530
    real shield_vitality;          // 0x84 -> +0x534
    uint8_t shield_stunned;        // 0x88 -> +0x538
    uint8_t pad_89[3];
} biped_network_create_message;    // 0x8c bytes

void unit_network_create_update_apply(void *incoming_record)
{
    biped_network_create_message message;
    real_vector3d side;
    uint8_t placement[0x88];
    int32_t creator = -1;
    int32_t machine = -1;
    datum_index biped_index;
    uint8_t *biped;

    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        message_delta_decode_compound_field_staged(incoming_record);
        return;
    }
    if (message_delta_decode_compound_field(incoming_record, &message) != 1) { // the original tests == 1
        return;
    }
    vector3d_cross_product(&side, &message.up, &message.forward);
    vector3d_cross_product(&message.up, &message.forward, &side);
    vector3d_normalize_with_length(&message.forward);
    vector3d_normalize_with_length(&message.up);
    if (message.creator_key != 0) {
        creator = (*(int32_t **)(object_pooled_node_globals + 0x28))[message.creator_key];
    }
    if (message.machine_key != 0) {
        machine = (*(int32_t **)(machine_table + 0x28))[message.machine_key];
    }
    memset(placement, 0, sizeof(placement));
    *(datum_index *)(placement + 0x00) = message.definition;
    *(int32_t *)(placement + 0x08) = machine;
    *(int32_t *)(placement + 0x0c) = creator;
    *(int16_t *)(placement + 0x14) = message.owner_team;
    memcpy(placement + 0x18, &message.position, 12);
    memcpy(placement + 0x28, &message.vector_38, 12);
    memcpy(placement + 0x34, &message.forward, 12);
    memcpy(placement + 0x40, &message.up, 12);
    memcpy(placement + 0x58, message.block_44, 0x30);
    biped_index = object_new_with_datum_role_control((object_placement_data *)placement, 1);
    if (biped_index == (datum_index)0xffffffff) {
        return;
    }
    network_index_cache_insert_if_free(network_object_index_cache, message.network_key, (int32_t)biped_index);
    biped = (uint8_t *)((object_header *)object_data->data)[biped_index & 0xffff].data;
    *(uint32_t *)(biped + 0x530) = message.body_vitality;
    *(real *)(biped + 0x534) = message.shield_vitality;
    biped[0x538] = message.shield_stunned;
    memcpy(biped + 0x52c, message.grenade_counts, 2);
    *(real *)(biped + 0xe4) = *(real *)(biped + 0x534) * 3.0f;   // shield vitality (0x672c3c = 3.0)
    biped[0x527] = message.update_sequence;
    *(uint32_t *)(biped + 0xe0) = *(uint32_t *)(biped + 0x530);  // body vitality
    *(int16_t *)(biped + 0x480) = (int16_t)*(int8_t *)(biped + 0x321);
    biped[0x526] = 1;
    biped[0x528] = 0;
    biped[0x475] = 1;
    *(int16_t *)(biped + 0x104) = biped[0x538] == 1;
    memcpy(biped + 0x4ac, biped + 0x494, 12);
    *(int16_t *)(biped + 0x31e) = *(int16_t *)(biped + 0x52c);
    if (message.flag_80000 != 0) {
        *(uint32_t *)(biped + 0x204) |= 0x80000;
    } else {
        *(uint32_t *)(biped + 0x204) &= ~0x80000u;
    }
    *(uint32_t *)(biped + 0x344) = message.scalar_344;
}
'''
open(p, "w", encoding="utf-8", newline="\n").write(new + t[cut:])
print("ok")
