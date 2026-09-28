// hs_evaluate_cinematic_skip_stop_internal  (not a Ghidra function; the evaluate handler of hs function 299 "cinematic_skip_stop_internal" (no parameters -> void))
// address 0x47f860, size 66 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47f860, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47f860..0x47f8a2: a saved music gain (0x00686b60) other than -1.0 (x87 fucompp: NaN restores too) is restored with
//   sound_set_music_gain and reset to -1.0; cinematic globals +0x0a = 0; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "units.h"
#include "cutscene.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern float cinematic_saved_music_gain; // 0x00686b60
extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c
extern void sound_set_music_gain(float gain); // 0x548680

void hs_evaluate_cinematic_skip_stop_internal(int16_t function_index, uint32_t thread_index, char first)
{
    if (!(cinematic_saved_music_gain == -1.0f)) {
        sound_set_music_gain(cinematic_saved_music_gain);
        cinematic_saved_music_gain = -1.0f;
    }
    cinematic_globals_ptr->skip_in_progress = 0;
    hs_thread_return(0, thread_index);
}
