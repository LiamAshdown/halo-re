// ai_initialize_for_new_map  (Ghidra: ai_initialize_for_new_map, already named)
// address 0x42a7c0, size 119 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: types/ai.h header note (this function allocates 0x8dc bytes for ai_globals and
//   crc32s that exact size, which is the primary evidence for k_ai_globals_size; the "prop"
//   data array name is the literal at 0x0065f008 read out of bin/halo.exe). Calls
//   actors_initialize (0x426710, already rewritten in this module), encounters_initialize
//   and ai_communication_initialize (both outside this rewrite's range but already present
//   in src/ai/, which is also where game_state_base/_cursor/_crc's established names come
//   from), and actor_avoidance_build_direction_tables (outside this rewrite's range, the direction-sample table
//   builder types/ai.h already attributes by address).
// register convention: plain __cdecl, no parameters.
//   // blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *prop_data;      // 0x008802c0

extern uint8_t *game_state_base;   // 0x006e2dc8
extern int32_t game_state_cursor;  // 0x006e2dcc
extern uint32_t game_state_crc;    // 0x006e2dd4

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count
extern void actors_initialize(void); // 0x426710
extern void encounters_initialize(void); // 0x435c00, not in this rewrite range
extern void ai_communication_initialize(void); // 0x42cf20, not in this rewrite range
extern void actor_avoidance_build_direction_tables(void); // 0x41a2d0, not in this rewrite range (builds the avoidance direction-sample tables)

extern char prop_array_name[]; // 0x0065f008, the literal "prop"

// Allocates and zero-initializes the main AI globals structure for a newly loaded map, then
// initializes the actor, prop, encounter, and communication subsystems.
void ai_initialize_for_new_map(void)
{
    ai_globals *globals = (ai_globals *)(game_state_base + game_state_cursor);
    int32_t size = k_ai_globals_size;

    game_state_cursor = game_state_cursor + k_ai_globals_size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    ai_globals_ptr = globals;
    memset(globals, 0, k_ai_globals_size);

    actors_initialize();
    prop_data = (data_array *)game_state_new(prop_array_name, k_prop_data_maximum_count, k_prop_size);
    encounters_initialize();
    ai_communication_initialize();
    actor_avoidance_build_direction_tables();
}

#if 0
Original Ghidra decompilation (0x42a7c0):

void ai_initialize_for_new_map(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_4;

  puVar2 = (undefined4 *)(DAT_006e2dcc + DAT_006e2dc8);
  DAT_006e2dcc = DAT_006e2dcc + 0x8dc;
  local_4 = 0x8dc;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_00880354 = puVar2;
  for (iVar1 = 0x237; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  actors_initialize();
  DAT_008802c0 = game_state_new(&DAT_0065f008,0x300);
  encounters_initialize();
  ai_communication_initialize();
  FUN_0041a2d0();
  return;
}
#endif
