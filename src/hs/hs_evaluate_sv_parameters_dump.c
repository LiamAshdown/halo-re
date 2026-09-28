// hs_evaluate_sv_parameters_dump  (not a Ghidra function; the evaluate handler of hs function 501 "sv_parameters_dump" ( -> void))
// address 0x482510, size 70 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_482510 trapped.
// WRITTEN 2026-09-28 from objdump 0x482510..0x482555: when message delta parameters are enabled with exactly 1:
//   writes the config text buffer (0x00860b40, used as the format) to "parameters.cfg" opened "wb"; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include <stdio.h>

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern uint8_t message_delta_parameters_enabled; // 0x0071cfa8
extern char message_delta_config_text_buffer[]; // 0x00860b40

void hs_evaluate_sv_parameters_dump(int16_t function_index, uint32_t thread_index, char first)
{
    if (message_delta_parameters_enabled != 0 && message_delta_parameters_enabled == 1) {
        FILE *file = fopen("parameters.cfg", "wb"); // 0x0066e664, mode 0x0066e674

        if (file != 0) {
            fprintf(file, message_delta_config_text_buffer);
            fclose(file);
        }
    }
    hs_thread_return(0, thread_index);
}
