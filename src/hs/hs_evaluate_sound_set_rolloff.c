// hs_evaluate_sound_set_rolloff  (not a Ghidra function; the evaluate handler of hs function 443 "sound_set_rolloff" (real -> void))
// address 0x481750, size 78 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x481750, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x481750..0x48179e: stores the factor at 0x00746134 and hands it to the DirectSound 3D listener (*0x00746114):
//   SetRolloffFactor (vtable +0x3c, stdcall: listener, factor, 0 = DS3D_IMMEDIATE); returns 0.
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
extern void *directsound_listener; // 0x00746114
extern float sound_listener_rolloff_factor; // 0x00746134

void hs_evaluate_sound_set_rolloff(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    float factor = *(float *)&arguments[0];
    void **vtable = *(void ***)directsound_listener;

    sound_listener_rolloff_factor = factor;
    ((int32_t (__stdcall *)(void *, float, uint32_t))vtable[0x3c / 4])(directsound_listener, factor, 0);
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
