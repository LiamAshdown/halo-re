exec(open(r'C:\Users\Liam-\halo-re\scratchpad\md_lib.py').read())
HDR = '#include "tags.h"\n#include "memory.h"\n#include "math.h"\n#include "cache.h"\n#include "objects.h"\n#include "units.h"\n#include <string.h>\n'

emit(0x571f20, 495, 'vehicle_encode_network_create', 'the vehicle object type  +0x64 hook (0x69b61c; the biped one is 0x55aed0): the creation message (type 0x1c) -- definition tag, the network key (a new index-cache slot 0x6870d8 when unknown), the owner team, the machine and creator keys (0 when unknown), four seat keys (vehicle +0x2f8; 0 for none, -1 unknown), five vectors (+0x52c, +0x550, +0x55c, +0x538, +0x544) and the +0x526 byte -- encoded with message_delta_encode_message into (buffer, bit budget).', '''
extern data_array *object_data;                    // 0x008603b0
extern uint8_t *object_pooled_node_globals;        // 0x00687130, network-id hash_table at +0x0c
extern uint8_t *machine_table;                     // 0x00687558, hash_table at +0x0c
extern uint8_t network_object_index_cache[];       // 0x006870d8
extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, ESI table, ECX key
extern int32_t network_index_cache_find_or_allocate_slot(uint8_t *container, int32_t key); // 0x4e9c20, EAX container
extern int32_t message_delta_encode_message(int32_t buffer, int32_t bit_budget, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX, EDX

typedef struct vehicle_network_create_record {
    datum_index definition;        // 0x00
    int32_t network_key;           // 0x04
    int16_t owner_team;            // 0x08
    int16_t pad_0a;
    int32_t machine_key;           // 0x0c
    int32_t creator_key;           // 0x10
    int32_t seat_keys[4];          // 0x14
    uint8_t vectors[5][12];        // 0x24
    uint8_t unknown_526;           // 0x60
    uint8_t pad_61[3];
} vehicle_network_create_record;   // size 0x64

int32_t vehicle_encode_network_create(datum_index vehicle_index, int32_t buffer, int32_t bit_budget)
{
    uint8_t *vehicle = (uint8_t *)((object_header *)object_data->data)[vehicle_index & 0xffff].data;
    hash_table *keys = (hash_table *)(object_pooled_node_globals + 0xc);
    vehicle_network_create_record record;
    void *item = &record;
    int32_t key = 0;
    int32_t creator = 0;
    int32_t machine = 0;
    int32_t i;

    if (vehicle_index != (datum_index)0xffffffff) {
        key = hash_table_get(keys, (int32_t)vehicle_index);
    }
    if (*(int32_t *)(vehicle + 0xc4) != -1) {
        creator = hash_table_get(keys, *(int32_t *)(vehicle + 0xc4));
        if (creator == -1) {
            creator = 0;
        }
    }
    if (*(int32_t *)(vehicle + 0xc0) != -1) {
        machine = hash_table_get((hash_table *)(machine_table + 0xc), *(int32_t *)(vehicle + 0xc0));
        if (machine == -1) {
            machine = 0;
        }
    }
    if (key == -1) {
        key = network_index_cache_find_or_allocate_slot(network_object_index_cache, (int32_t)vehicle_index);
    }
    record.definition = *(datum_index *)vehicle;
    record.network_key = key;
    record.owner_team = *(int16_t *)(vehicle + 0xb8);
    record.creator_key = creator;
    record.machine_key = machine;
    for (i = 0; i < 4; i++) {
        int32_t seat = ((int32_t *)(vehicle + 0x2f8))[i];

        record.seat_keys[i] = seat == -1 ? 0 : hash_table_get(keys, seat);
    }
    record.unknown_526 = vehicle[0x526];
    memcpy(record.vectors[0], vehicle + 0x52c, 12);
    memcpy(record.vectors[1], vehicle + 0x550, 12);
    memcpy(record.vectors[2], vehicle + 0x55c, 12);
    memcpy(record.vectors[3], vehicle + 0x538, 12);
    memcpy(record.vectors[4], vehicle + 0x544, 12);
    return message_delta_encode_message(buffer, bit_budget, 0, 0x1c, 0, &item, 0, 1, 0);
}
''', cc='stack -> vehicle_index, buffer, bit_budget', module='units', header=HDR)

emit(0x5726e0, 846, 'vehicle_apply_network_update', 'the vehicle object type  +0x70 hook (0x69b628; the biped one is 0x55b5f0): applies a received update. A vehicle already updated rejects (staged) a reliable update whose sequence (+4) differs or whose delta counter (+5) is not ahead within 30. The 0x40-byte baseline decodes (reliable: forced, from the current one at +0x528); on success the counter and flag 0x8000000 are taken, a full update also the sequence and baseline. Forward/up are re-orthonormalized (two cross products, normalized) and written with position and velocity to the network block (+0x1c.., +0x48, +0x2c, +0x38) and the object (+0x68, +0x8c, +0x74, +0x80; flag 0x20 cleared unless set in the update); children follow the position (0x570cb0), and a position more than 10 away, flag 0x20, or up misaligned beyond cos 45 snaps again. The message  latency (+0xad8 of the connection, else garbage from the record pointer as the binary does) above 10 records an extrapolation; the baseline is kept at +0x56c.', '''
extern data_array *object_data;                    // 0x008603b0
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX index
extern uint8_t message_delta_decode_compound_field_forced(void **context, void *destination, int32_t changed_offset,
    int32_t force); // 0x4ec600, EAX context, ECX destination, EDX
extern uint8_t message_delta_decode_compound_field(void **context, void *destination); // 0x4ec590, EAX, ECX
extern uint8_t message_delta_decode_compound_field_staged(void **context); // 0x4ec670, EAX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void unit_propagate_position_delta_to_children(real_point3d *new_position, uint32_t unit_index); // 0x570cb0
extern double sqrt(double x);

typedef struct vehicle_network_baseline {
    uint8_t object_flag_5;         // 0x00
    uint8_t pad_01[3];
    real_point3d position;         // 0x04
    real_vector3d velocity;        // 0x10
    real_vector3d angular_velocity;// 0x1c
    real_vector3d forward;         // 0x28
    real_vector3d up;              // 0x34
} vehicle_network_baseline;        // size 0x40

void vehicle_apply_network_update(datum_index vehicle_index, void **message, uint8_t *connection)
{
    uint8_t *vehicle = (uint8_t *)object_try_and_get(vehicle_index, 2);
    uint8_t *record;
    uint8_t *guard;
    vehicle_network_baseline baseline;
    real_vector3d side;
    uint8_t accepted;
    real dx;
    real dy;
    real dz;
    int32_t latency_base;
    int32_t latency;
    int32_t *timing;

    if (vehicle == 0) {
        message_delta_decode_compound_field_staged(message);
        return;
    }
    record = (uint8_t *)message[0x11];
    guard = (uint8_t *)((object_header *)object_data->data)[vehicle_index & 0xffff].data;
    if ((*(uint32_t *)(guard + 0x10) & 0x8000000) != 0 && **(int32_t **)message == 1) {
        int32_t incoming = record[5];
        int32_t current = vehicle[0x527];

        if (record[4] != vehicle[0x526] || (incoming <= current && incoming - current + 0xff >= 0x1e)) {
            message_delta_decode_compound_field_staged(message);
            return;
        }
    }
    if (**(int32_t **)message == 1) {
        memcpy(&baseline, vehicle + 0x528, sizeof(baseline));
        accepted = message_delta_decode_compound_field_forced(message, &baseline, (int32_t)(vehicle + 0x528), 0);
    } else {
        accepted = message_delta_decode_compound_field(message, &baseline);
    }
    if (!accepted) {
        return;
    }
    vehicle[0x527] = record[5];
    *(uint32_t *)(vehicle + 0x10) |= 0x8000000;
    if (record[6] != 0) {
        vehicle[0x526] = record[4];
        memcpy(vehicle + 0x528, &baseline, sizeof(baseline));
    }
    vector3d_cross_product(&side, &baseline.up, &baseline.forward);
    vector3d_cross_product(&baseline.up, &baseline.forward, &side);
    vector3d_normalize_with_length(&baseline.forward);
    vector3d_normalize_with_length(&baseline.up);
    memcpy(vehicle + 0x1c, &baseline.position, 12);
    memcpy(vehicle + 0x48, &baseline.velocity, 12);
    memcpy(vehicle + 0x2c, &baseline.forward, 12);
    memcpy(vehicle + 0x38, &baseline.up, 12);
    vehicle[0x18] = 1;
    vehicle[0x44] = 1;
    vehicle[0x28] = 1;
    if (baseline.object_flag_5 == 0) {
        *(uint32_t *)(vehicle + 0x10) &= ~0x20u;
    }
    memcpy(vehicle + 0x68, &baseline.velocity, 12);
    memcpy(vehicle + 0x8c, &baseline.angular_velocity, 12);
    memcpy(vehicle + 0x74, &baseline.forward, 12);
    memcpy(vehicle + 0x80, &baseline.up, 12);
    unit_propagate_position_delta_to_children(&baseline.position, vehicle_index);
    dx = baseline.position.x - *(real *)(vehicle + 0x5c);
    dy = baseline.position.y - *(real *)(vehicle + 0x60);
    dz = baseline.position.z - *(real *)(vehicle + 0x64);
    if ((real)sqrt(dx * dx + dy * dy + dz * dz) > 10.0f || (*(uint32_t *)(vehicle + 0x10) & 0x20) != 0 ||
        baseline.up.j * ((real *)(vehicle + 0x80))[1] + baseline.up.k * ((real *)(vehicle + 0x80))[2] +
                baseline.up.i * ((real *)(vehicle + 0x80))[0] < 0.70710677f) {
        memcpy(vehicle + 0x1c, &baseline.position, 12);
        memcpy(vehicle + 0x48, &baseline.velocity, 12);
        memcpy(vehicle + 0x74, &baseline.forward, 12);
        memcpy(vehicle + 0x80, &baseline.up, 12);
        unit_propagate_position_delta_to_children(&baseline.position, vehicle_index);
    }
    timing = *(int32_t **)(connection + 0xad8);
    latency_base = timing != 0 ? timing[0] : (int32_t)record;
    latency = timing != 0 ? timing[1] : (int32_t)record;
    if (latency > 10) {
        vehicle[0x54] = 1;
        *(int32_t *)(vehicle + 0x58) = *(int32_t *)(record + 8) - latency_base;
    } else {
        vehicle[0x54] = 0;
    }
    vehicle[0x18] = 1;
    vehicle[0x44] = 1;
    vehicle[0x475] = 1;
    memcpy(vehicle + 0x56c, &baseline, sizeof(baseline));
}
''', cc='stack -> vehicle_index, message, connection', module='units', header=HDR)
print('ok')
