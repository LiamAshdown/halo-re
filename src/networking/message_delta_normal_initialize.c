// message_delta_normal_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4ea6c0, size 92 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea6c0..0x4ea71b: with the parameters protocol on, registers the phi and theta
//   bit widths under the type  name ("bits_theta_internet" -> +0, "bits_phi_internet" -> +4); all four widths
//   positive.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void message_delta_parameters_protocol_register(char *scope, char *name, int32_t type, void *value); // 0x4ebe00, EAX scope
extern uint8_t message_delta_parameters_enabled; // 0x0071cfa8

uint8_t message_delta_normal_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_protocol_register(field_type->name, (char *)"bits_theta_internet", 1, descriptor);
        message_delta_parameters_protocol_register(field_type->name, (char *)"bits_phi_internet", 1, descriptor + 1);
    }
    return descriptor[0] > 0 && descriptor[1] > 0 && descriptor[2] > 0 && descriptor[3] > 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
