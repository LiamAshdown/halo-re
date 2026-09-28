// hs_evaluate_magic_melee_attack  (not a Ghidra function; the evaluate handler of hs function 120 "magic_melee_attack" ( -> void))
// address 0x47c2d0, size 36 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: the function record's evaluate slot (+0x0c); only reachable through it, so no C meant
//   unlisted_47c2d0 trapped.
// WRITTEN 2026-09-28 from objdump 0x47c2d0..0x47c2f3: makes the first player's unit (player +0x34) ready its weapon
//   (0x569a20, not forced, no direction); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread
extern data_array *player_data; // 0x0087a480
extern uint8_t unit_try_ready_weapon(uint32_t unit_index, uint8_t forced, const void *direction); // 0x569a20, blam-cc: EDI unit

void hs_evaluate_magic_melee_attack(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t *player = (uint8_t *)player_data->data;

    unit_try_ready_weapon(*(uint32_t *)(player + 0x34), 0, 0);
    hs_thread_return(0, thread_index);
}
