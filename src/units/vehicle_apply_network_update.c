// vehicle_apply_network_update  (reached only through a .data code pointer; no C existed)
// address 0x5726e0, size 846 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x5726e0..0x572a2d: the vehicle object type  +0x70 hook (0x69b628; the biped one
//   is 0x55b5f0): applies a received update. A vehicle already updated rejects (staged) a reliable update whose
//   sequence (+4) differs or whose delta counter (+5) is not ahead within 30. The 0x40-byte baseline decodes
//   (reliable: forced, from the current one at +0x528); on success the counter and flag 0x8000000 are taken, a full
//   update also the sequence and baseline. Forward/up are re-orthonormalized (two cross products, normalized) and
//   written with position and velocity to the network block (+0x1c.., +0x48, +0x2c, +0x38) and the object (+0x68,
//   +0x8c, +0x74, +0x80; flag 0x20 cleared unless set in the update); children follow the position (0x570cb0), and a
//   position more than 10 away, flag 0x20, or up misaligned beyond cos 45 snaps again. The message  latency (+0xad8
//   of the connection, else garbage from the record pointer as the binary does) above 10 records an extrapolation;
//   the baseline is kept at +0x56c.
// blam-cc: stack -> vehicle_index, message, connection

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
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
    ((unit_object *)vehicle)->base.flags |= 0x8000000;
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
        ((unit_object *)vehicle)->base.flags &= ~0x20u;
    }
    memcpy(vehicle + 0x68, &baseline.velocity, 12);
    memcpy(vehicle + 0x8c, &baseline.angular_velocity, 12);
    memcpy(vehicle + 0x74, &baseline.forward, 12);
    memcpy(vehicle + 0x80, &baseline.up, 12);
    unit_propagate_position_delta_to_children(&baseline.position, vehicle_index);
    dx = baseline.position.x - ((unit_object *)vehicle)->base.position.x;
    dy = baseline.position.y - ((unit_object *)vehicle)->base.position.y;
    dz = baseline.position.z - ((unit_object *)vehicle)->base.position.z;
    if ((real)sqrt(dx * dx + dy * dy + dz * dz) > 10.0f || (((unit_object *)vehicle)->base.flags & 0x20) != 0 ||
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
        *(int32_t *)&((unit_object *)vehicle)->base.network_timestamp = *(int32_t *)(record + 8) - latency_base;
    } else {
        vehicle[0x54] = 0;
    }
    vehicle[0x18] = 1;
    vehicle[0x44] = 1;
    vehicle[0x475] = 1;
    memcpy(vehicle + 0x56c, &baseline, sizeof(baseline));
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
