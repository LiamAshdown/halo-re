// hs_evaluate_sound_looping_set_alternate  (not a Ghidra function; the evaluate handler of hs function 333 "sound_looping_set_alternate" (looping_sound, boolean -> void))
// address 0x47ff40, size 72 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47ff40, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47ff40..0x47ff88: sound_looping_set_alternate(EAX looping sound, stack: the boolean zero-extended), returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void sound_looping_set_alternate(datum_index looping_definition, uint8_t alternate); // 0x544200, EAX, stack

void hs_evaluate_sound_looping_set_alternate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    sound_looping_set_alternate((datum_index)arguments[0], *(uint8_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
