// hs_evaluate_game_safe_to_save  (not a Ghidra function; the evaluate handler of hs "game_safe_to_save" (-> boolean))
// address 0x47fa40, size 31 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47fa40, only reachable through that pointer.
//   Campaign track: 1 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47fa40: the byte result of 0x45ba50 zero-extended.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t game_safe_to_save(void); // 0x45ba50

void hs_evaluate_game_safe_to_save(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    uint8_t safe = game_safe_to_save();
    hs_thread_return((int32_t)safe, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
