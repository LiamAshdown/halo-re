// hs_evaluate_sound_impulse_time  (not a Ghidra function; the evaluate handler of hs function 327 "sound_impulse_time" (sound -> long))
// address 0x47fd30, size 111 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fd30, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fd30..0x47fd9f: the ticks until the sound tag's impulse ends (+0x90 minus the game time, 0x006f1d6c +0x0c), clamped at 0; 0 for
//   a -1 sound or no impulse.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "cache.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

void hs_evaluate_sound_impulse_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index sound = (datum_index)arguments[0];
    int32_t ticks = 0;

    if (sound != k_datum_index_none) {
        int32_t end_time = *(int32_t *)((uint8_t *)tag_instances[sound & 0xffff].data + 0x90);

        if (end_time != -1) {
            ticks = end_time - game_time->game_time;
            if (ticks <= 0) {
                ticks = 0;
            }
        }
    }
    hs_thread_return(ticks, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
