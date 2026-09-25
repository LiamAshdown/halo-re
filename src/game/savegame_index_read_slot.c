// savegame_index_read_slot  (Ghidra: FUN_0053e0e0; renamed, no established name)
// address 0x53e0e0, size 266 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/game_types_notes.md ("save games" section, record size 0x206 proved by
//   this very function's `slot * 0x206 + 0x206` size check); shares the file_reference reset
//   preamble with the sibling savegame_index_file_exists.c (this batch).
// register convention: a slot index in the low 16 bits of the incoming value (Ghidra's own
//   `ushort param_1`, a genuine stack parameter).
// UNSURE: file_reference_seek/_read's real argument lists (elided by Ghidra, same as the
//   whole family); the destination buffer for file_reference_read is never visible in this
//   function's own decompilation, so it is presumably an implicit register argument this
//   function's own (unrecovered) caller sets up -- modeled here as a bare call with no
//   parameter, exactly as Ghidra shows it.
// reconciled: R14 0x00721440 void* handle -> network_mutex_record *savegame_index_mutex; now waits on/releases ->handle (the binary loads [0x721440] then [eax])

// FIXED (objdump): path_append_component takes (destination = the file reference's path buffer at +8, in ESI;
//   component, in EBX); the draft passed them swapped, and read the component array as a pointer.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"
#include "networking.h"

extern char saved_game_root_path[]; // 0x006e3108 (an array: the original passes its address), the appended component
extern file_reference savegame_directory_file_reference; // 0x00721330
extern network_mutex_record *savegame_index_mutex; // 0x00721440, networking.h record; +0x00 is the HANDLE

extern uint8_t file_reference_open(file_reference *reference, int32_t mode); // 0x5557a0
extern uint8_t file_reference_close(void); // 0x555890
extern uint8_t file_reference_seek(void); // 0x5558f0, UNSURE args elided
extern uint32_t file_reference_get_size(file_reference *reference); // 0x555950
extern uint8_t file_reference_read(void); // 0x555a20, UNSURE args elided
extern void path_append_component(char *destination, const char *component); // 0x555ec0
extern void path_remove_last_component(uint8_t *path); // 0x555f80
extern uint32_t WaitForSingleObject(void *handle, uint32_t timeout_ms); // Win32
extern uint32_t ReleaseMutex(void *handle); // Win32
// CORRECTED by review: the four path/file_reference helpers above were declared argument-less
// because Ghidra elides their register arguments. savegame_index_file_exists.c's own objdump
// pass pins them for the whole family:
//   path_remove_last_component  EBX -> the file_reference's path field (reference + 8)
//   path_append_component       EBX -> component, ESI -> reference
//   file_reference_open         ESI -> reference, stack -> mode
//   file_reference_get_size     EAX -> reference
// The remaining helpers (file_reference_close / _seek / _read / _write) are still UNSURE.

// Locks the save-game directory mutex, resets the shared file_reference to the index path,
// opens it for reading, and -- if it is large enough to contain `slot` -- seeks to that slot's
// record and reads it. Returns 1 on full success, 0 otherwise; always releases the mutex before
// returning (unless the initial wait itself failed/timed out).
uint8_t savegame_index_read_slot(uint16_t slot)
{
    uint8_t result = 0;
    uint32_t wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);

    if (wait_result != 0 && wait_result != 0x80) {
        return 0;
    }

    {
        uint32_t *raw = (uint32_t *)&savegame_directory_file_reference;
        int32_t i;
        uint8_t *flags_byte = (uint8_t *)&savegame_directory_file_reference + 4;
        uint16_t *word_at_6 = (uint16_t *)((uint8_t *)&savegame_directory_file_reference + 6);

        for (i = 0; i < 0x43; i++) {
            raw[i] = 0;
        }
        raw[0] = 0x66696c6f;
        *word_at_6 = 2;
        if ((*flags_byte & 1) != 0) {
            path_remove_last_component((uint8_t *)&savegame_directory_file_reference + 8);
        }
        path_append_component((char *)&savegame_directory_file_reference + 8, saved_game_root_path);
        *flags_byte = *flags_byte | 1;
    }

    if (file_reference_open(&savegame_directory_file_reference, 1) != 0) {
        uint32_t size = file_reference_get_size(&savegame_directory_file_reference);
        if ((uint32_t)slot * 0x206 + 0x206 <= size) {
            if (file_reference_seek() != 0) {
                result = 1;
                if (file_reference_read() == 0) {
                    result = 0;
                }
            }
        }
        if (file_reference_close() == 0) {
            result = 0;
        }
    }

    ReleaseMutex(savegame_index_mutex->handle);
    return result;
}

#if 0
Original Ghidra decompilation (0x53e0e0), from tools/pack.py 0x53e0e0:

undefined1 FUN_0053e0e0(ushort param_1)

{
  char cVar1;
  DWORD DVar2;
  uint uVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined1 local_1;

  local_1 = 0;
  DVar2 = WaitForSingleObject((HANDLE)*DAT_00721440,5000);
  if ((DVar2 != 0) && (DVar2 != 0x80)) {
    return 0;
  }
  puVar5 = &DAT_00721330;
  for (iVar4 = 0x43; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  DAT_00721330 = 0x66696c6f;
  DAT_00721334._2_2_ = 2;
  if (((ushort)DAT_00721334 & 1) != 0) {
    path_remove_last_component();
  }
  path_append_component();
  DAT_00721334._0_2_ = (ushort)DAT_00721334 | 1;
  cVar1 = file_reference_open(1);
  if (cVar1 == '\0') goto LAB_0053e1cf;
  uVar3 = file_reference_get_size();
  if ((uint)param_1 * 0x206 + 0x206 <= uVar3) {
    cVar1 = file_reference_seek();
    if (cVar1 != '\0') {
      cVar1 = file_reference_read();
      local_1 = 1;
      if (cVar1 != '\0') goto LAB_0053e1bd;
    }
    local_1 = 0;
  }
LAB_0053e1bd:
  cVar1 = file_reference_close();
  if (cVar1 == '\0') {
    local_1 = 0;
  }
LAB_0053e1cf:
  ReleaseMutex((HANDLE)*DAT_00721440);
  return local_1;
}
#endif
