// game_sound_initialize  (Ghidra: game_sound_initialize, already named)
// address 0x543a30, size 88 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: types/sound.h globals (0x007461a0 game_looping_sound_data, 0x007461a4 game_sound_globals);
//   allocates the "object looping sounds" data array (0x400 x game_looping_sound, matching
//   k_maximum_game_looping_sounds) and registers a 0xc-byte game_sound_globals block, mirroring
//   the game_state_new/crc32_update pattern in src/ai/ai_initialize_for_new_map.c.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *game_looping_sound_data;  // 0x007461a0
extern game_sound_globals *game_sound_globals_ptr; // 0x007461a4

extern uint8_t *game_state_base;   // 0x006e2dc8
extern int32_t game_state_cursor;  // 0x006e2dcc
extern uint32_t game_state_crc;    // 0x006e2dd4

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count
    // blam-cc: EBX -> element_size, stack -> (name, maximum_count); see src/ai/actors_initialize.c

// Allocates the object-looping-sounds datum array and registers the game_sound_globals block
// (update_count / background_sound_index / last_update_time) at the current game state cursor.
void game_sound_initialize(void)
{
    int32_t size = sizeof(game_sound_globals);
    game_sound_globals *globals = (game_sound_globals *)(game_state_base + game_state_cursor);

    game_looping_sound_data = (data_array *)game_state_new((char *)"object looping sounds", k_maximum_game_looping_sounds, sizeof(game_looping_sound));

    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    game_sound_globals_ptr = globals;
}

#if 0
Original Ghidra decompilation (0x543a30):

void __cdecl game_sound_initialize(void)

{
  int iVar1;
  undefined4 local_4;

  DAT_007461a0 = game_state_new("object looping sounds",0x400);
  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0xc;
  local_4 = 0xc;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_007461a4 = iVar1;
  return;
}

Disassembly (0x543a30..0x543a88, capstone; phase-4 review):

0x543a30: push ecx
0x543a31: push ebx
0x543a32: push esi
0x543a33: push 0x400
0x543a38: push 0x671340
0x543a3d: mov ebx, 0x34
0x543a42: call 0x5380d0
0x543a47: mov ecx, dword ptr [0x6e2dc8]
0x543a4d: mov dword ptr [0x7461a0], eax
0x543a52: mov eax, dword ptr [0x6e2dcc]
0x543a57: push 4
0x543a59: lea edx, [esp + 0x14]
0x543a5d: lea esi, [eax + ecx]
0x543a60: push edx
0x543a61: add eax, 0xc
0x543a64: push 0x6e2dd4
0x543a69: mov dword ptr [esp + 0x1c], 0xc
0x543a71: mov dword ptr [0x6e2dcc], eax
0x543a76: call 0x4d02d0
0x543a7b: add esp, 0x14
0x543a7e: mov dword ptr [0x7461a4], esi
0x543a84: pop esi
0x543a85: pop ebx
0x543a86: pop ecx
0x543a87: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
