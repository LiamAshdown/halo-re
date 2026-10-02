// hs_evaluate_recording_time  (not a Ghidra function; the evaluate handler of hs function 70 "recording_time" (unit -> short))
// address 0x47b0c0, size 103 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47b0c0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47b0c0..0x47b127: the word at +0x08 of the unit's recording when its +0x04 is that unit, else 0, in a zeroed dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"
#include "units.h"
#include "cutscene.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern recorded_animation *recorded_animation_find_by_object(datum_index unit_index, datum_index *out_index);
    // 0x44ad20, blam-cc: EBX, stack

void hs_evaluate_recording_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];
    uint8_t *recording = (uint8_t *)recorded_animation_find_by_object(unit, 0);
    uint16_t ticks = 0;

    if (recording != 0 && *(datum_index *)(recording + 4) == unit) {
        ticks = *(uint16_t *)(recording + 8);
    }
    hs_thread_return((int32_t)ticks, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
