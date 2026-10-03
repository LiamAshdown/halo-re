// hs_evaluate_sv_parameters_reload  (not a Ghidra function; the evaluate handler of hs function 500 "sv_parameters_reload" ( -> void))
// address 0x4824e0, size 35 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_4824e0 trapped.
// WRITTEN 2026-09-28 from objdump 0x4824e0..0x482502: while message delta parameters are enabled (0x0071cfa8):
//   reloads them from the config file, reapplies the field bindings and sends an update; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t message_delta_parameters_enabled; // 0x0071cfa8
extern void message_delta_parameters_protocol_reload_from_config_file(void); // 0x4ebda0
extern void message_delta_definitions_invoke_field_bindings(void); // 0x4ec390
extern void message_delta_parameters_protocol_send_update(void); // 0x4ebf50

void hs_evaluate_sv_parameters_reload(int16_t function_index, uint32_t thread_index, char first)
{
    if (message_delta_parameters_enabled != 0) {
        message_delta_parameters_protocol_reload_from_config_file();
        message_delta_definitions_invoke_field_bindings();
        message_delta_parameters_protocol_send_update();
    }
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
