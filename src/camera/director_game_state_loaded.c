// director_game_state_loaded  (no Ghidra function; new entry)
// address 0x445560, size 22 bytes (0x445560..0x445575)
// name confidence: 0.4   rewrite confidence: 0.95
// evidence: entry 13 (the last) of game_state_after_load_procs (.data 0x0069e7e0,
//   types/saved_games.h 0x0069e7b4[13]). After a saved game is loaded it resets the director
//   (camera_initialize) and re-applies the hs camera_control value that was saved with the game
//   state (the byte behind 0x0087bc0c), so a cutscene camera survives the load.
// register convention: none; cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"
#include "fn_camera.h"

extern uint8_t *hs_camera_control_pointer; // 0x0087bc0c, hs module


void director_game_state_loaded(void)
{
    camera_initialize();
    camera_control(*hs_camera_control_pointer);
}

#if 0
No Ghidra function exists at 0x445560 (Ghidra folds it into the padding after
camera_track_compute_pov). objdump -d -M intel:

  445560: call 0x445580
  445565: mov eax,ds:0x87bc0c
  44556a: xor ecx,ecx
  44556c: mov cl,BYTE PTR [eax]
  44556e: push ecx
  44556f: call 0x445cc0
  445574: pop ecx
  445575: ret
#endif
