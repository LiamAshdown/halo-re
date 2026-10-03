// hs_evaluate_unit_get_shield  (not a Ghidra function; the evaluate handler of hs function 125 "unit_get_shield" (unit -> real))
// address 0x47c480, size 125 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47c480, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47c480..0x47c4fd: object_try_and_get(ECX unit, stack -1): none gives -1.0 (0x672ba8), a frozen object (+0x106 bit 2) 0.0
//   (0x672ac0), otherwise its shield vitality (+0xe4).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX, stack

void hs_evaluate_unit_get_shield(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t *object = (uint8_t *)object_try_and_get((datum_index)arguments[0], 0xffffffff);
    float result = -1.0f;

    if (object != 0) {
        result = (object[0x106] & 4) ? 0.0f : *(float *)(object + 0xe4);
    }
    hs_thread_return(*(int32_t *)&result, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
