// savegame_index_remove_slot  (Ghidra: FUN_0053e4a0; renamed, no established name)
// address 0x53e4a0, size 400 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: shares the file_reference reset preamble with the rest of this family (this
//   batch); record size 0x206 per out/phase4/game_types_notes.md; the loop shifts every record
//   after `slot` down by one (read then write one record size earlier) before truncating via
//   file_reference_set_length (not in this batch, a file-truncate-to-current-position helper judging by its
//   call position and lack of arguments).
// register convention: a slot index in the low 16 bits of the incoming value (Ghidra's own
//   `ushort param_1`, a genuine stack parameter).
// UNSURE: file_reference_seek/_read/_write's real argument lists (elided by Ghidra, same as the
//   whole family); file_reference_set_length's exact behavior (presumed file truncate).
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
extern uint8_t file_reference_seek(void); // 0x5558f0
extern uint8_t file_reference_read(void); // 0x555a20
extern uint8_t file_reference_write(void); // 0x555a90
extern void path_append_component(char *destination, const char *component); // 0x555ec0
extern void path_remove_last_component(uint8_t *path); // 0x555f80
extern uint8_t file_reference_get_size_by_path(uint32_t *out_size); // 0x555b00, not in this batch
extern uint8_t file_reference_set_length(void); // 0x555559b0... 0x5559b0, not in this batch; UNSURE: presumed truncate
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

// Removes save-slot `slot` by shifting every following record down by one position (read each,
// seek back, write it one record earlier) and then truncating the file, if the index is large
// enough to contain `slot` in the first place. Returns 1 on success, 0 otherwise.
uint8_t savegame_index_remove_slot(uint16_t slot)
{
    uint8_t result = 0;
    uint32_t wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);
    uint32_t size[131]; // UNSURE: Ghidra's local_20c[131], only [0] is used (file_reference_get_size_by_path's
                        // out-size parameter); kept at its original size for fidelity

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

    if (file_reference_get_size_by_path(size) != 0) {
        uint32_t offset = (uint32_t)slot * 0x206 + 0x206;
        if (offset <= size[0] && file_reference_open(&savegame_directory_file_reference, 3) != 0) {
            result = file_reference_seek();
            if (result == 1) {
                for (; offset < size[0]; offset = offset + 0x206) {
                    if (file_reference_seek() == 0 || file_reference_read() == 0 ||
                        file_reference_seek() == 0 || file_reference_write() == 0) {
                        result = 0;
                        goto close_file;
                    }
                }
                result = file_reference_set_length();
            } else if (result != 0) {
                // UNSURE: dead in practice if file_reference_seek only ever returns 0/1
                // (preserved literally -- see original's own separate `== 1` / `!= 0` tests)
                result = file_reference_set_length();
            }
        close_file:
            if (file_reference_close() == 0) {
                result = 0;
            }
        }
    }

    ReleaseMutex(savegame_index_mutex->handle);
    return result;
}

#if 0
Original Ghidra decompilation (0x53e4a0), from tools/pack.py 0x53e4a0:

char FUN_0053e4a0(ushort param_1)

{
  char cVar1;
  DWORD DVar2;
  int iVar3;
  undefined4 *puVar4;
  uint uVar5;
  char local_20d;
  uint local_20c [131];

  local_20d = '\0';
  DVar2 = WaitForSingleObject((HANDLE)*DAT_00721440,5000);
  if ((DVar2 != 0) && (DVar2 != 0x80)) {
    return '\0';
  }
  puVar4 = &DAT_00721330;
  for (iVar3 = 0x43; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  DAT_00721330 = 0x66696c6f;
  DAT_00721334._2_2_ = 2;
  if (((byte)DAT_00721334 & 1) != 0) {
    path_remove_last_component();
  }
  path_append_component();
  DAT_00721334._0_1_ = (byte)DAT_00721334 | 1;
  cVar1 = FUN_00555b00(local_20c);
  if (((cVar1 == '\0') || (uVar5 = (uint)param_1 * 0x206 + 0x206, local_20c[0] < uVar5)) ||
     (cVar1 = file_reference_open(3), cVar1 == '\0')) goto LAB_0053e608;
  local_20d = file_reference_seek();
  if (local_20d == '\x01') {
    for (; uVar5 < local_20c[0]; uVar5 = uVar5 + 0x206) {
      cVar1 = file_reference_seek();
      if (((cVar1 == '\0') || (cVar1 = file_reference_read(), cVar1 == '\0')) ||
         ((cVar1 = file_reference_seek(), cVar1 == '\0' ||
          (cVar1 = file_reference_write(), cVar1 == '\0')))) {
        local_20d = '\0';
        goto LAB_0053e5f6;
      }
    }
LAB_0053e5e2:
    local_20d = FUN_005559b0();
  }
  else if (local_20d != '\0') goto LAB_0053e5e2;
LAB_0053e5f6:
  cVar1 = file_reference_close();
  if (cVar1 == '\0') {
    local_20d = '\0';
  }
LAB_0053e608:
  ReleaseMutex((HANDLE)*DAT_00721440);
  return local_20d;
}
#endif
