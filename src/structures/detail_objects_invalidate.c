// detail_objects_invalidate  (not a Ghidra function; game state after-load proc 8)
// address 0x5522c0, size 13 bytes
// name confidence: 0.4  rewrite confidence: 0.95
// evidence: game_state_after_load_procs[8] (0x0069e7d4) holds 0x5522c0; detail_object_globals.valid (+0x520e,
//   types/structures.h) is the "0 forces a rebuild" byte.
// objdump 0x5522c0: mov eax,ds:0x72277c / mov BYTE PTR [eax+0x520e],0x0 / ret
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"

extern uint8_t *detail_objects; // 0x0072277c

void detail_objects_invalidate(void)
{
    detail_objects[0x520e] = 0;
}
