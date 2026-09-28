// hs_evaluate_cheat_all_vehicles  (not a Ghidra function; the evaluate procedure of hs function "cheat_all_vehicles"; no C existed, so running it
//   from the console trapped as unlisted_4828b0)
// address 0x4828b0, size 51 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4828b0: when the Globals tag's +0x164 block is non-empty, spawns element 0's vehicle list (+0x24 elements, +0x20 count as a word) near the camera (0x45a800). Returns 0 through hs_thread_return (EAX 0, ECX thread).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, EAX value, ECX thread

extern uint8_t *global_globals; // 0x00746fa0, the Globals tag data
extern void cheat_spawn_objects_near_camera(TagDependency *tag_array, int16_t count); // 0x45a800

void hs_evaluate_cheat_all_vehicles(int16_t function_index, uint32_t thread_index, char first)
{
    if (*(int32_t *)(global_globals + 0x164) != 0) {
        uint8_t *element = *(uint8_t **)(global_globals + 0x168);

        cheat_spawn_objects_near_camera(*(TagDependency **)(element + 0x24), (int16_t)*(uint16_t *)(element + 0x20));
    }
    hs_thread_return(0, thread_index);
}
