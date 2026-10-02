// hs_evaluate_unit_get_total_grenade_count  (not a Ghidra function; the evaluate handler of hs function 126 "unit_get_total_grenade_count" (unit -> short))
// address 0x47c500, size 116 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47c500, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47c500..0x47c574: the sum of the two signed grenade count bytes (+0x31e, +0x31f) of a unit (object_try_and_get(ECX, 3)), else 0,
//   as a word in a zeroed dword.
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

void hs_evaluate_unit_get_total_grenade_count(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t *unit = (uint8_t *)object_try_and_get((datum_index)arguments[0], 3);
    int16_t total = 0;

    if (unit != 0) {
        total = (int16_t)((int8_t)unit[0x31e] + (int8_t)unit[0x31f]);
    }
    hs_thread_return((int32_t)(uint16_t)total, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
