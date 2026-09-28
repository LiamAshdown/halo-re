// hs_evaluate_profile_unlock_solo_levels  (not a Ghidra function; the evaluate handler of hs function 431 "profile_unlock_solo_levels" (no parameters -> void))
// address 0x481400, size 58 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x481400, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x481400..0x48143a: ORs 0x0f into the ten level bytes at 0x00712ef6 and 4 into 0x00712ef4 of the profile globals (0x00712dd8), then
//   writes the profile when one is open (0x00714dd4 != -1: player_profile_write_data(stack: handle, block)); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t profile_globals_block[0x60a4]; // 0x00712dd8
extern int32_t saved_player_profile_slots_handle; // 0x00714dd4
extern void player_profile_write_data(int32_t handle, void *profile); // 0x53a950 (saved_player_profile *)

void hs_evaluate_profile_unlock_solo_levels(int16_t function_index, uint32_t thread_index, char first)
{
    int32_t level;

    for (level = 0; level < 10; level++) {
        profile_globals_block[0x11e + level] |= 0xf;
    }
    profile_globals_block[0x11c] |= 4;
    if (saved_player_profile_slots_handle != -1) {
        player_profile_write_data(saved_player_profile_slots_handle, profile_globals_block);
    }
    hs_thread_return(0, thread_index);
}
