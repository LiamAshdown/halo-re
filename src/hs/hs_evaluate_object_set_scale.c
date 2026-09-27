// hs_evaluate_object_set_scale  (not a Ghidra function; the evaluate handler of hs function 75 "object_set_scale" (object, real, short -> void))
// address 0x47b2c0, size 77 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b2c0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b2c0..0x47b30d: object_set_scale_and_refresh_nodes(EAX object, stack: scale, the tick word zero-extended), returns 0.
//   STABLE-DIVERGENCE: the retail callee forwards that second stack argument (EDX at 0x4f96dc) to 0x4f6b70; the
//   stable C of object_set_scale_and_refresh_nodes takes only the scale, so the ticks are dropped here.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void object_set_scale_and_refresh_nodes(uint32_t object_index, float scale, int16_t ticks); // 0x4f96a0, EAX, stack

void hs_evaluate_object_set_scale(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    object_set_scale_and_refresh_nodes((uint32_t)arguments[0], *(float *)&arguments[1],
        (int16_t)*(uint16_t *)&arguments[2]); // 0x47b2ef: the tick word, zero-extended
    hs_thread_return(0, thread_index);
    }
}
