// message_delta_compute_size_2  (reached only through a .data code pointer; no C existed)
// address 0x4eb2c0, size 6 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb2c0..0x4eb2c5: 2 bits (grenade index).
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t message_delta_compute_size_2(message_delta_field_type *field_type)
{
    (void)field_type;
    return 2;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
