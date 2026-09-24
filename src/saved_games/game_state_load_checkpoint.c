// game_state_load_checkpoint  (Ghidra: game_state_load_checkpoint, already named)
// address 0x538280, size 149 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_functions.md; disassembly at 0x538280 confirms a local
// 0x14c-byte game_state_header-shaped stack buffer, passed to saved_game_validate_crc as
// (ECX=k_game_state_size, EDX=k_game_state_header_size, EBX=&buffer, stack: &buffer.file_checksum,
// NULL) and then to saved_game_verify_version_and_checksum(&buffer, 0); on success it applies
// the checkpoint's difficulty to cache_file_slot_table+0xe (0x006b0b80, matching
// out/phase4/saved_games_types_notes.md's game_state_header::difficulty note) before running the
// revert callback, reading the persistent-storage block and dispatching load callbacks.
// register convention: no parameters, no return value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "cache.h"

extern uint16_t game_state_persistent_storage_created_or_similar_007196d8; // 0x007196d8, UNSURE: guards this whole path
extern int16_t pending_difficulty; // 0x00696564
extern uint8_t *game_state_base; // 0x006e2dc8
extern uint8_t *cache_file_slot_table; // 0x006b0b80, opaque here; +0x0e is difficulty
extern game_state_proc game_state_revert_proc; // 0x0069e7b0

extern uint8_t saved_game_validate_crc(int32_t total_size, int32_t header_size, uint8_t *header_buffer,
    uint32_t *expected_crc, uint8_t *corrupt_flag); // 0x539570
extern uint8_t saved_game_verify_version_and_checksum(game_state_header *header, uint8_t report_error); // 0x538430
extern void game_state_read_persistent_storage_block(int32_t size, void *buffer); // 0x539850, blam-cc: EAX size
extern void game_state_dispatch_load_callbacks(void); // 0x537f70
extern void game_state_perform_save(uint8_t is_checkpoint); // 0x5381c0

void game_state_load_checkpoint(void)
{
    game_state_header header;

    if (game_state_persistent_storage_created_or_similar_007196d8 != 0) {
        return;
    }
    if (saved_game_validate_crc(k_game_state_size, k_game_state_header_size, (uint8_t *)&header,
            &header.file_checksum, 0) == 0) {
        return;
    }
    if (saved_game_verify_version_and_checksum(&header, 0) == 0) {
        return;
    }
    if (pending_difficulty != header.difficulty) {
        return;
    }

    game_state_revert_proc();
    game_state_read_persistent_storage_block(k_game_state_size, game_state_base);
    *(int16_t *)(cache_file_slot_table + 0xe) = pending_difficulty;
    game_state_dispatch_load_callbacks();
    game_state_perform_save(0);
}

#if 0
Original Ghidra decompilation (0x538280):

void game_state_load_checkpoint(void)

{
  char cVar1;
  uint uVar2;
  int local_14c [73];
  short local_26;
  undefined1 local_4 [4];

  if (DAT_007196d8 == 0) {
    cVar1 = saved_game_validate_crc(local_4,0);
    if (cVar1 != '\0') {
      uVar2 = saved_game_verify_version_and_checksum(local_14c,'\0');
      if (((char)uVar2 != '\0') && (DAT_00696564 == local_26)) {
        (*(code *)PTR_chimera__revert_0069e7b0)();
        game_state_read_persistent_storage_block(DAT_006e2dc8);
        *(short *)(DAT_006b0b80 + 0xe) = DAT_00696564;
        game_state_dispatch_load_callbacks();
        FUN_005381c0(0);
      }
    }
  }
  return;
}
#endif
