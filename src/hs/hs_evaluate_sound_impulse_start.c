// hs_evaluate_sound_impulse_start  (not a Ghidra function; the evaluate handler of hs function 326 "sound_impulse_start" (sound, object, real -> void))
// address 0x47fce0, size 75 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fce0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fce0..0x47fd2b: sound_impulse_start(EAX object, ECX sound, stack scale), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void sound_impulse_start(datum_index object_index, datum_index definition_index, float scale); // 0x543e10, EAX, ECX, stack

void hs_evaluate_sound_impulse_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    sound_impulse_start((datum_index)arguments[1], (datum_index)arguments[0], *(float *)&arguments[2]);
    hs_thread_return(0, thread_index);
    }
}
