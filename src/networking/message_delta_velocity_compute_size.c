// message_delta_velocity_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4eb520, size 56 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb520..0x4eb557: the index prefix (count + 1) plus three components at the
//   wider of the two widths.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t message_delta_velocity_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t mode0 = descriptor[4] * 3;
    int32_t mode1 = descriptor[2] * 3;

    if (descriptor[6] + mode0 + 1 > mode1 + descriptor[6] + 1) {
        return descriptor[6] + mode0 + 1;
    }
    return descriptor[6] + mode1 + 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
