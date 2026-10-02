// hs_evaluate_unit_solo_player_integrated_night_vision_is_active  (not a Ghidra function; the evaluate handler of hs "unit_solo_player_integrated_night_vision_is_active" (-> boolean))
// address 0x47c730, size 31 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47c730, only reachable through that pointer.
//   Campaign track: 2 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47c730: the byte result of 0x565b00 zero-extended.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t unit_local_player_weapon_flag_check(void); // 0x565b00

void hs_evaluate_unit_solo_player_integrated_night_vision_is_active(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    uint8_t active = unit_local_player_weapon_flag_check();
    hs_thread_return((int32_t)active, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
