// message_delta_first_dword_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4ea4d0, size 10 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea4d0..0x4ea4d9: the descriptor  first dword (the flag count for kind 17, the
//   bit width for kind 20).
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_first_dword_compute_size(message_delta_field_type *field_type)
{
    return *(int32_t *)field_type->array_descriptor;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
