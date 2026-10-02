// hs_evaluate_ignore_arguments  (not a Ghidra function; the evaluate handler shared by 9 hs functions, listed below)
// address 0x47fc20, size 56 bytes
// shared by: 167 "ai_select" (ai -> void); 319 "core_save_name" (string -> void);
//   322 "core_load_name" (string -> void); 323 "core_load_name_at_startup" (string -> void);
//   325 "sound_impulse_predict" (sound, boolean -> void); 393 "player_effect_set_max_vibrate" (real, real -> void);
//   452 "config_one_control" (string -> void); 478 "set_gamepad_yaw_scale" (short, real -> void);
//   480 "set_gamepad_pitch_scale" (short, real -> void);
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fc20, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fc20..0x47fc58: evaluates the arguments and returns 0: the evaluator of functions retail compiled to nothing (core_save_name,
//   core_load_name, ai_select, sound_impulse_predict, set_gamepad_*_scale, ...).
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

void hs_evaluate_ignore_arguments(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return(0, thread_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
