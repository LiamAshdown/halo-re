// hs_evaluate_vehicle_hover  (not a Ghidra function; the evaluate handler of hs function "vehicle_hover"
//   (vehicle, boolean -> void); no C existed, so a10's scripts hit the unlisted_4801c0 trap)
// address 0x4801c0, size 131 bytes
// name confidence: 0.9  rewrite confidence: 0.85
// evidence: hs function table record at 0x65a1d4 (name 0x662934 "vehicle_hover", parse 0x487440, evaluate +0xc
//   0x4801c0, parameters vehicle / boolean), only reachable through that pointer.
// WRITTEN 2026-09-28 from objdump 0x4801c0..0x480242: for a valid vehicle, hovering on records the vehicle's
//   current position at vehicle +0x4fc (object_get_position, EAX out / ECX vehicle) and sets bit 0x02 of the
//   byte at +0x4cc; hovering off clears that bit. Returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern data_array *object_data; // 0x008603b0
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX out, ECX object

void hs_evaluate_vehicle_hover(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        datum_index vehicle = (datum_index)arguments[0];
        uint8_t hover = *(uint8_t *)&arguments[1];

        if (vehicle != k_datum_index_none) {
            uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[vehicle & 0xffff].data;

            if (hover != 0) {
                object_get_position((real_point3d *)(obj + 0x4fc), vehicle);
                obj[0x4cc] |= 2;
            } else {
                obj[0x4cc] &= 0xfd;
            }
        }
        hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
