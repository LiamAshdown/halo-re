// encounters_initialize  (Ghidra: encounters_initialize, already named)
// address 0x435c00, size 170 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: cea-pdb name match via the "encounter" and "ai pursuit" literals it hands to
//   game_state_new; types/ai.h header note ("encounters_initialize @0x435c00 'encounter'
//   0x80 stride 0x6c, 'ai pursuit' 0x100 stride 0x28", and the two flat tables of 0x8000 and
//   0x1000 bytes it reserves out of the game state). game_state_new's real signature
//   (element size in EBX, name and maximum_count on the stack) is already established at
//   src/effects/contrails_initialize.c and src/ai/actors_initialize.c.
// register convention: no arguments.
//
// UNSURE: Ghidra computes both raw game-state blocks before storing either global; the two
// stores are interleaved with the crc32_update calls in the original. crc32_update cannot
// touch encounter_squad_states / encounter_platoon_states, so the order is reproduced in the
// readable order here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *encounter_data;                            // 0x008802c8
extern encounter_squad_state *encounter_squad_states;         // 0x008802cc
extern encounter_platoon_state *encounter_platoon_states;     // 0x008802c4
extern data_array *ai_pursuit_data;                           // 0x008802d0
extern uint8_t *game_state_base;   // 0x006e2dc8
extern int32_t game_state_cursor;  // 0x006e2dcc
extern uint32_t game_state_crc;    // 0x006e2dd4

extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count
    // blam-cc: EBX -> element_size, stack -> (name, maximum_count)
extern void crc32_update(uint32_t *crc, void *data, int32_t length); // 0x4d02d0

// Creates the encounter and "ai pursuit" data arrays and reserves the two flat sub-record
// tables (1024 encounter_squad_state records, 256 encounter_platoon_state records) straight
// out of the game-state bump allocator, folding each reservation size into the game-state
// crc the same way game_state_new does.
void encounters_initialize(void)
{
    uint32_t reserved_size;

    encounter_data = (data_array *)game_state_new((char *)"encounter", k_encounter_data_maximum_count, k_encounter_size);

    encounter_squad_states = (encounter_squad_state *)(game_state_base + game_state_cursor);
    game_state_cursor = game_state_cursor + 0x8000;
    reserved_size = 0x8000;
    crc32_update(&game_state_crc, &reserved_size, 4);

    encounter_platoon_states = (encounter_platoon_state *)(game_state_base + game_state_cursor);
    game_state_cursor = game_state_cursor + 0x1000;
    reserved_size = 0x1000;
    crc32_update(&game_state_crc, &reserved_size, 4);

    ai_pursuit_data = (data_array *)game_state_new((char *)"ai pursuit", k_ai_pursuit_data_maximum_count, k_ai_pursuit_size);
}

#if 0
Original Ghidra decompilation (0x435c00):

void __cdecl encounters_initialize(void)

{
  int iVar1;
  int iVar2;
  undefined4 local_4;

  DAT_008802c8 = game_state_new("encounter",0x80);
  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x8000;
  local_4 = 0x8000;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  iVar2 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x1000;
  local_4 = 0x1000;
  DAT_008802cc = iVar1;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_008802c4 = iVar2;
  DAT_008802d0 = game_state_new("ai pursuit",0x100);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
