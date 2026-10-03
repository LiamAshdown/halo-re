// observer_initialize  (no Ghidra function; new entry)
// address 0x447870, size 10 bytes (0x447870..0x447879)
// name confidence: 0.45   rewrite confidence: 0.95
// evidence: out/phase4/camera_types_notes.md "0x447870: observer initialize thunk
//   (mov edx,0x6ac65c; jmp 0x447740)". It is entry 5 of game_state_after_load_procs
//   (.data 0x0069e7c0, types/saved_games.h 0x0069e7b4[13]), so a loaded saved game starts with a
//   freshly constructed observer for local player 0.
// register convention: none; cdecl, no arguments (tail call into observer_new with EDX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern observer observers[1]; // 0x006ac65c

// blam-cc: EDX -> self
extern void observer_new(observer *self); // 0x447740, this module

void observer_initialize(void)
{
    observer_new(&observers[0]);
}

#if 0
No Ghidra function exists at 0x447870. objdump -d -M intel:

  447870: mov edx,0x6ac65c
  447875: jmp 0x447740
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
