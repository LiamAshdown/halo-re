// savegame_index_file_exists  (Ghidra: FUN_0053e060; renamed, no established name)
// address 0x53e060, size 115 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/game_types_notes.md ("save games" section): the shared 0x43-dword reset
//   of the file_reference at 0x00721330 (types/hs.h file_reference, 0x10c bytes == 0x43 dwords)
//   is common to this whole family (savegame_index_read_slot / _write_slot / _append_slot /
//   _remove_slot, this batch); file_reference_open's mode-1 argument here matches the
//   read-mode opens in the sibling read functions.
// register convention: none -- no stack parameters, no register arguments recovered.
// CORRECTED by review (objdump 0x53e060..0x53e0d3):
//   - the return type. Only AL is written ("mov al,1" at 0x53e0cd / "xor al,al" at 0x53e0d0),
//     and the surrounding EAX still holds 0x00721330 from the file_reference_get_size call, so
//     the full EAX is NOT 1 on success. Declared uint8_t.
//   - the four elided callee arguments, all visible as live registers at each call:
//       0x53e08d  mov ebx,0x721338 ; call 0x555f80   -> EBX = &ref->path
//       0x53e097  mov ebx,0x6e3108 ; mov esi,0x721330 ; call 0x555ec0
//                                                    -> EBX = the component, ESI = the ref
//       0x53e0af  mov esi,0x721330 ; push 1 ; call 0x5557a0  -> ESI = ref, stack = mode
//       0x53e0c3  mov eax,0x721330 ; call 0x555950   -> EAX = ref
// UNSURE: DAT_00721334's field roles (kept as raw byte/word offsets into the opaque
//   file_reference); the implicit path string is DAT_006e3108, built once by
//   saved_game_files_initialize, outside this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"

extern file_reference savegame_directory_file_reference; // 0x00721330

extern uint8_t *saved_game_root_path; // 0x006e3108, the path component appended below
extern uint8_t file_reference_open(file_reference *reference, int32_t mode); // 0x5557a0;
    // blam-cc: ESI -> reference, stack -> mode
extern uint32_t file_reference_get_size(file_reference *reference); // 0x555950; blam-cc: EAX -> reference
extern void path_append_component(uint8_t *component, file_reference *reference); // 0x555ec0;
    // blam-cc: EBX -> component, ESI -> reference
extern void path_remove_last_component(uint8_t *path); // 0x555f80; blam-cc: EBX -> path
    // (called with file_reference + 8, i.e. the path field itself)

// Resets the shared save-game-directory file_reference to the "saved games index" path and
// tries to open it for reading, purely to confirm the file exists (and to warm its cached
// size); returns 1 on success, 0 otherwise.
uint8_t savegame_index_file_exists(void)
{
    uint32_t *raw = (uint32_t *)&savegame_directory_file_reference;
    int32_t i;
    uint8_t *flags_byte = (uint8_t *)&savegame_directory_file_reference + 4;
    uint16_t *word_at_6 = (uint16_t *)((uint8_t *)&savegame_directory_file_reference + 6);

    for (i = 0; i < 0x43; i++) {
        raw[i] = 0;
    }
    raw[0] = 0x66696c6f; // UNSURE: field identity, see header note
    *word_at_6 = 2;
    if ((*flags_byte & 1) != 0) {
        path_remove_last_component((uint8_t *)&savegame_directory_file_reference + 8);
    }
    path_append_component(saved_game_root_path, &savegame_directory_file_reference);
    *flags_byte = *flags_byte | 1;

    if (file_reference_open(&savegame_directory_file_reference, 1) != 0) {
        file_reference_get_size(&savegame_directory_file_reference);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x53e060), from tools/pack.py 0x53e060:

undefined4 FUN_0053e060(void)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;

  puVar3 = &DAT_00721330;
  for (iVar2 = 0x43; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  DAT_00721330 = 0x66696c6f;
  DAT_00721334._2_2_ = 2;
  if (((byte)DAT_00721334 & 1) != 0) {
    path_remove_last_component();
  }
  path_append_component();
  DAT_00721334._0_1_ = (byte)DAT_00721334 | 1;
  cVar1 = file_reference_open(1);
  if (cVar1 != '\0') {
    file_reference_get_size();
    return 1;
  }
  return 0;
}
#endif
