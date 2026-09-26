// hs_evaluate_sound_looping_start  (not a Ghidra function; the evaluate handler of hs function 330 "sound_looping_start" (looping_sound, object, real -> void))
// address 0x47fe60, size 74 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fe60, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fe60..0x47feaa: sound_looping_start(EAX looping sound, stack: object, scale), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void sound_looping_start(datum_index definition_index, datum_index object_index, float scale); // 0x544090, EAX, stack

void hs_evaluate_sound_looping_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    sound_looping_start((datum_index)arguments[0], (datum_index)arguments[1], *(float *)&arguments[2]);
    hs_thread_return(0, thread_index);
    }
}
