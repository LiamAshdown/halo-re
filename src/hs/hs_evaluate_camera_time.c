// hs_evaluate_camera_time  (not a Ghidra function; the evaluate handler of hs function 252 "camera_time" (no parameters -> short))
// address 0x47eff0, size 44 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47eff0, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47eff0..0x47f01c: the camera time left (0x006869d8, seconds) times 30.0 (0x672ac8) truncated by __ftol, as a word in a zeroed
//   dword.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"


extern float camera_script_time_remaining; // 0x006869d8

void hs_evaluate_camera_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return((int32_t)(uint16_t)(int16_t)(int32_t)(camera_script_time_remaining * 30.0f), thread_index);
}
