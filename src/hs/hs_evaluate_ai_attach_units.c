// hs_evaluate_ai_attach_units  (not a Ghidra function; the evaluate handler of the orphan hs function record 0x6589e0 "ai_attach_units" (object_list, ai -> void), not in hs_function_definitions)
// address 0x47d0a0, size 68 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47d0a0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47d0a0..0x47d0e3: spawns the ai into the object list (0x432a40); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern void ai_object_list_spawn_members(datum_index object_list_header_handle, uint32_t packed_reference); // 0x432a40, blam-cc: EAX, EDI

void hs_evaluate_ai_attach_units(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        ai_object_list_spawn_members((datum_index)arguments[0], (uint32_t)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}
