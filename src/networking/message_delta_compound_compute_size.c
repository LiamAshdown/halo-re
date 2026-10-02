// message_delta_compound_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4e9530, size 76 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9530..0x4e957b: each binding  type size (cached in it) summed; one flag per
//   binding (reserved bits = count).
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_compound_compute_size(message_delta_field_type *field_type)
{
    message_delta_array_field_list *list = (message_delta_array_field_list *)field_type->array_descriptor;
    int32_t total = 0;
    int32_t i;

    for (i = 0; i < list->count; i++) {
        int32_t bits = MESSAGE_DELTA_COMPUTE_SIZE(list->fields[i].field_type);

        total += bits;
        list->fields[i].field_type->size_bits = bits;
    }
    field_type->reserved_bits = list->count;
    return list->count + total;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
