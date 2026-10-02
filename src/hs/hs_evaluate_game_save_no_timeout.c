// hs_evaluate_game_save_no_timeout  (not a Ghidra function; the evaluate handler of hs function 313 "game_save_no_timeout" (no parameters -> void))
// address 0x47fb40, size 66 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47fb40, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47fb40..0x47fb82: as game_save but leaving 0x0071973e clear (it is always cleared); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t network_join_error_reason; // 0x0071973c
extern uint8_t main_globals_byte_0071973d; // 0x0071973d
extern uint8_t main_globals_byte_0071973e; // 0x0071973e
extern int32_t main_globals_dword_00719740; // 0x00719740
extern int32_t main_globals_dword_00719744; // 0x00719744
extern int16_t main_globals_word_0071974c; // 0x0071974c

void hs_evaluate_game_save_no_timeout(int16_t function_index, uint32_t thread_index, char first)
{
    if (network_join_error_reason == 0 || main_globals_byte_0071973e != 0) {
        network_join_error_reason = 1;
        main_globals_byte_0071973d = 1;
        main_globals_dword_00719740 = 0;
        main_globals_dword_00719744 = 0;
        main_globals_word_0071974c = 0;
    }
    main_globals_byte_0071973e = 0;
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
