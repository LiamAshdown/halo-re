// hs_evaluate_sound_enable  (not a Ghidra function; the evaluate handler of hs function 337 "sound_enable" (boolean -> void))
// address 0x480030, size 64 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x480030, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x480030..0x480070: sound_enabled (0x00725201) = the boolean, returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


extern uint8_t sound_enabled; // 0x00725201

void hs_evaluate_sound_enable(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    sound_enabled = *(uint8_t *)&arguments[0];
    hs_thread_return(0, thread_index);
    }
}
