// savegame_index_get_slot_count  (Ghidra: FUN_0053e420; renamed, no established name)
// address 0x53e420, size 122 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: shares the file_reference reset preamble with the rest of this family (this
//   batch); file_reference_get_size_by_path (not in this batch) is a size-only query (no open mode argument, an
//   out-parameter for the size) reused verbatim from savegame_index_remove_slot.c's own use of
//   it; dividing by 0x206 (the record size, out/phase4/game_types_notes.md) gives the slot
//   count.
// register convention: none -- no stack parameters, no register arguments recovered.
// UNSURE: file_reference_get_size_by_path's real signature (elided by Ghidra); the file_reference reset here is
//   performed but never followed by an explicit file_reference_open, unlike the rest of the
//   family -- file_reference_get_size_by_path apparently opens (or stats) the path itself.

// FIXED (objdump): path_append_component takes (destination = the file reference's path buffer at +8, in ESI;
//   component, in EBX); the draft passed them swapped, and read the component array as a pointer.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char saved_game_root_path[]; // 0x006e3108 (an array: the original passes its address), the appended component
extern file_reference savegame_index_file; // 0x00721330

extern void path_append_component(char *destination, const char *component); // 0x555ec0
extern void path_remove_last_component(uint8_t *path); // 0x555f80
extern uint8_t file_reference_get_size_by_path(void *ref, uint32_t *out_size); // 0x555b00, ESI ref, stack out_size
// CORRECTED by review: the four path/file_reference helpers above were declared argument-less
// because Ghidra elides their register arguments. savegame_index_file_exists.c's own objdump
// pass pins them for the whole family:
//   path_remove_last_component  EBX -> the file_reference's path field (reference + 8)
//   path_append_component       EBX -> component, ESI -> reference
//   file_reference_open         ESI -> reference, stack -> mode
//   file_reference_get_size     EAX -> reference
// The remaining helpers (file_reference_close / _seek / _read / _write) are still UNSURE.

// Resets the shared file_reference to the index path and queries its size via file_reference_get_size_by_path,
// returning size / 0x206 (the number of save-slot records), or 0 if the query fails.
uint32_t savegame_index_get_slot_count(void)
{
    uint32_t *raw = (uint32_t *)&savegame_index_file;
    int32_t i;
    uint8_t *flags_byte = (uint8_t *)&savegame_index_file + 4;
    uint16_t *word_at_6 = (uint16_t *)((uint8_t *)&savegame_index_file + 6);
    uint32_t size;

    for (i = 0; i < 0x43; i++) {
        raw[i] = 0;
    }
    raw[0] = 0x66696c6f;
    *word_at_6 = 2;
    if ((*flags_byte & 1) != 0) {
        path_remove_last_component((uint8_t *)&savegame_index_file + 8);
    }
    path_append_component((char *)&savegame_index_file + 8, saved_game_root_path);
    *flags_byte = *flags_byte | 1;

    if (file_reference_get_size_by_path((void *)&savegame_index_file, &size) != 0) { // 0x53e473: ESI = 0x721330
        return size / 0x206;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x53e420), from tools/pack.py 0x53e420:

uint FUN_0053e420(void)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  uint local_4;

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
  cVar1 = FUN_00555b00(&local_4);
  if (cVar1 != '\0') {
    return local_4 / 0x206;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
