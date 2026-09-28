// hs_evaluate_cheat_all_powerups  (not a Ghidra function; the evaluate procedure of hs function "cheat_all_powerups"; no C existed, so running it
//   from the console trapped as unlisted_47ce70)
// address 0x47ce70, size 53 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x47ce70: spawns the Globals tag's powerup list (+0x158 count as int16, +0x15c elements, NULL when empty) near the camera (0x45a800). Returns 0 through hs_thread_return (EAX 0, ECX thread).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, EAX value, ECX thread

extern uint8_t *global_globals; // 0x00746fa0, the Globals tag data
extern void cheat_spawn_objects_near_camera(TagDependency *tag_array, int16_t count); // 0x45a800

void hs_evaluate_cheat_all_powerups(int16_t function_index, uint32_t thread_index, char first)
{
    TagDependency *list = *(int32_t *)(global_globals + 0x158) != 0 ? *(TagDependency **)(global_globals + 0x15c) : 0;

    cheat_spawn_objects_near_camera(list, *(int16_t *)(global_globals + 0x158));
    hs_thread_return(0, thread_index);
}
