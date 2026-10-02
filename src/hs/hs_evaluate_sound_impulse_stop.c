// hs_evaluate_sound_impulse_stop  (not a Ghidra function; the evaluate handler of hs function 328 "sound_impulse_stop" (sound -> void))
// address 0x47fda0, size 119 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fda0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fda0..0x47fe17: a sound tag whose playing impulse (+0x94) is set gets sound_impulse_fade_out(ECX impulse), then +0x94 and +0x90
//   = -1; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern tag_instance *tag_instances; // 0x0087bc14
extern void sound_impulse_fade_out(datum_index sound_index); // 0x549ee0, blam-cc: ECX

void hs_evaluate_sound_impulse_stop(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index sound = (datum_index)arguments[0];

    if (sound != k_datum_index_none) {
        uint8_t *definition = (uint8_t *)tag_instances[sound & 0xffff].data;

        if (*(datum_index *)(definition + 0x94) != k_datum_index_none) {
            sound_impulse_fade_out(*(datum_index *)(definition + 0x94));
            *(datum_index *)(definition + 0x94) = k_datum_index_none;
            *(datum_index *)(definition + 0x90) = k_datum_index_none;
        }
    }
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
