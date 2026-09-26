// object_update_export_functions  (not a Ghidra function; the base object type's +0x38 callback)
// address 0x4f80d0, size 427 bytes
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: object_type_definition object (0x0069b360) field +0x38; the object type dispatch calls it cdecl with the
//   object handle for every object. Only reachable through that table; first-boot track: needed while placing the
//   UI map's objects.
//   objdump 0x4f80d0..0x4f827a (switch: byte table 0x004f829c, jump table 0x004f827c): each of the four export
//   function sources (int16 at object tag +0x108 + 2i; 0 leaves output i alone) writes output i (float at object
//   +0x124 + 4i):
//     1 -> +0xe0            2 -> +0xe4, at most 1     3 -> +0xec            4 -> +0xe8
//     5 -> random_real() when the output is exactly 1.0, else 0
//     18 -> 0 when object byte +0x106 has bit 2, else 1
//     19 -> compass: while the root forward vector (at the object's +0x1f2 node-matrix offset, +4) is not near
//           vertical (|k| < 0.995), angle_delta_wrapped(scenario +0x4c, atan2(i, j)) / 2pi + 0.5 clamped to 0..1;
//           near vertical the output is kept
//     anything else -> object byte +0x178 + (source - 10) / 255
// blam-cc: stack -> object_index (cdecl)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *global_scenario; // 0x00746f8c
extern real random_real(void); // 0x4019f0
extern float angle_delta_wrapped(float from, float to); // 0x470d10
extern double fpatan(double y, double x); // x87 FPATAN: atan(y / x) with the quadrant of (x, y)
extern double fabs(double x);

static float clamp_to_one(float value)
{
    return value <= 1.0f ? value : 1.0f; // fcom 1.0 / test ah,0x41: NaN also becomes 1
}

void object_update_export_functions(datum_index object_index)
{
    uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
    uint8_t *definition = (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
    int32_t i;

    for (i = 0; i < 4; i++) {
        int16_t source = *(int16_t *)(definition + 0x108 + i * 2);
        float *output = (float *)(object + 0x124 + i * 4);
        float value = 0.0f;

        if (source == 0) {
            continue;
        }
        switch (source) {
        case 1: value = *(float *)(object + 0xe0); break;
        case 2: value = clamp_to_one(*(float *)(object + 0xe4)); break;
        case 3: value = *(float *)(object + 0xec); break;
        case 4: value = *(float *)(object + 0xe8); break;
        case 5:
            if (*(uint32_t *)output == 0x3f800000) {
                value = random_real();
            }
            break;
        case 18: value = (object[0x106] & 4) != 0 ? 0.0f : 1.0f; break;
        case 19: {
            float *forward = (float *)(object + *(int16_t *)(object + 0x1f2) + 4);

            if (!(fabs(forward[2]) < 0.995)) {
                value = *output; // near vertical (or NaN): unchanged
            } else {
                float yaw = (float)fpatan(forward[0], forward[1]);
                value = angle_delta_wrapped(*(float *)(global_scenario + 0x4c), yaw) * 0.15915494f + 0.5f;
                value = value >= 0.0f ? clamp_to_one(value) : 0.0f;
            }
            break;
        }
        default:
            value = (float)object[0x178 + (source - 10)] * 0.0039215689f;
            break;
        }
        *output = value;
    }
}
