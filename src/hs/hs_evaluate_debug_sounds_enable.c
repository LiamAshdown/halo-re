// hs_evaluate_debug_sounds_enable  (not a Ghidra function; the evaluate handler of hs function 334 "debug_sounds_enable" (string, boolean -> void))
// address 0x47ff90, size 72 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47ff90, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47ff90..0x47ffd8: sound_class_set_muted_by_name(BL boolean, stack name), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_sound.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58


void hs_evaluate_debug_sounds_enable(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    sound_class_set_muted_by_name(*(uint8_t *)&arguments[1], (char *)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}
