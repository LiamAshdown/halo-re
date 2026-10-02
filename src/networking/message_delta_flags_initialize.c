// message_delta_flags_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4ea260, size 67 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea260..0x4ea2a2: 1..32 flags, each mask byte 0 or 1.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

uint8_t message_delta_flags_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint8_t *mask = (uint8_t *)(descriptor + 1);
    int32_t i;

    if (descriptor[0] <= 0 || descriptor[0] > 0x20) {
        return 0;
    }
    for (i = 0; i < descriptor[0]; i++) {
        if (mask[i] != 1 && mask[i] != 0) {
            return 0;
        }
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
