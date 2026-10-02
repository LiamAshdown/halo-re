// message_delta_locality_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4eab80, size 91 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eab80..0x4eabda: with the parameters protocol on registers
//   LOCALITY_BITS_PER_COMPONENT_FULL / _DELTA (ints), LOCALITY_DELTA_CUTOFF_DISTANCE and
//   LOCALITY_MINIMUM_MOVE_DISTANCE (reals); valid.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t message_delta_vector3d_absolute_bits_mode1; // 0x0069a2cc
extern uint32_t message_delta_vector3d_delta_bits;          // 0x0069a2d0
extern real message_delta_vector3d_delta_range;             // 0x0069a2d4
extern real message_delta_vector3d_delta_epsilon;           // 0x0069a2d8
extern uint32_t message_delta_vector3d_absolute_bits_mode0; // 0x0069a2dc
extern void message_delta_parameters_protocol_register(char *scope, char *name, int32_t type, void *value); // 0x4ebe00, EAX scope
extern uint8_t message_delta_parameters_enabled; // 0x0071cfa8

uint8_t message_delta_locality_initialize(message_delta_field_type *field_type)
{
    (void)field_type;
    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_protocol_register(0, (char *)"LOCALITY_BITS_PER_COMPONENT_FULL", 1,
            &message_delta_vector3d_absolute_bits_mode1);
        message_delta_parameters_protocol_register(0, (char *)"LOCALITY_BITS_PER_COMPONENT_DELTA", 1, &message_delta_vector3d_delta_bits);
        message_delta_parameters_protocol_register(0, (char *)"LOCALITY_DELTA_CUTOFF_DISTANCE", 0, &message_delta_vector3d_delta_range);
        message_delta_parameters_protocol_register(0, (char *)"LOCALITY_MINIMUM_MOVE_DISTANCE", 0,
            &message_delta_vector3d_delta_epsilon);
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
