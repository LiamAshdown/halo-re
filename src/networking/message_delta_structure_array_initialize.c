// message_delta_structure_array_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4e90f0, size 52 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e90f0..0x4e9123: a positive count and element size and an element type that
//   initializes.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

uint8_t message_delta_structure_array_initialize(message_delta_field_type *field_type)
{
    message_delta_array_descriptor *descriptor = (message_delta_array_descriptor *)field_type->array_descriptor;

    if (descriptor->count <= 0 || descriptor->element_size <= 0 || descriptor->field_type == 0) {
        return 0;
    }
    return MESSAGE_DELTA_INITIALIZE(descriptor->field_type) == 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
