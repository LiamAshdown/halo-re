// savegame_index_append_slot  (Ghidra: FUN_0053e300; renamed, no established name)
// address 0x53e300, size 276 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// evidence: shares the file_reference reset preamble with the sibling savegame_index_read_slot.c
//   / savegame_index_write_slot.c (this batch); the size check `size / 0x206 < 999` caps the
//   index at 999 records before appending a new one at the current end and reporting the new
//   slot count through the caller's out-parameter.
// register convention: `unused` (Ghidra's `param_1`, never read in the body) and `out_slot_count`
//   are both genuine stack parameters.
// UNSURE: `unused`'s real purpose (dead in this build); file_reference_seek/_write's argument
//   lists (elided by Ghidra, same as the whole family).
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
extern uint8_t file_reference_close(file_reference *reference); // 0x555890, ESI ref
extern uint8_t file_reference_seek(int32_t offset, file_reference *reference); // 0x5558f0, EAX offset, ECX ref
extern uint32_t file_reference_get_size(file_reference *reference); // 0x555950
extern uint8_t file_reference_write(file_reference *reference, const void *buffer, uint32_t size); // 0x555a90, EDX ref, ECX buffer, ESI size
extern void path_append_component(char *destination, const char *component); // 0x555ec0
extern void path_remove_last_component(uint8_t *path); // 0x555f80
extern uint32_t __stdcall WaitForSingleObject(void *handle, uint32_t timeout_ms); // Win32
extern uint32_t __stdcall ReleaseMutex(void *handle); // Win32
// CORRECTED by review: the four path/file_reference helpers above were declared argument-less
// because Ghidra elides their register arguments. savegame_index_file_exists.c's own objdump
// pass pins them for the whole family:
//   path_remove_last_component  EBX -> the file_reference's path field (reference + 8)
//   path_append_component       EBX -> component, ESI -> reference
//   file_reference_open         ESI -> reference, stack -> mode
//   file_reference_get_size     EAX -> reference
// The remaining helpers (file_reference_close / _seek / _read / _write) are still UNSURE.

// UNSURE: `unused` is read nowhere in this function's body (see header note).
// If the index file has fewer than 999 records, seeks to the end and writes a new record
// (source elided, same as savegame_index_write_slot), reporting the new record's slot number
// through `*out_slot_count`. Returns 1 on success, 0 otherwise.
// FIXED 2026-09-27 (static loop) from objdump 0x53e300..0x53e40d: the first stack argument ([esp+0x14] at 0x53e3be)
// is the 0x206-byte entry appended at record index size / 0x206 (must be < 999); the draft named it "unused" and
// called seek / write / close with no arguments, so creating a profile never registered it in the index.
uint8_t savegame_index_append_slot(const void *entry, uint32_t *out_slot_count)
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

    if (file_reference_open(&savegame_directory_file_reference, 2) != 0) {
        uint32_t size = file_reference_get_size(&savegame_directory_file_reference);
        if (size / 0x206 < 999) {
            if (file_reference_seek((int32_t)(size / 0x206) * 0x206, &savegame_directory_file_reference) != 0 &&
                file_reference_write(&savegame_directory_file_reference, entry, 0x206) != 0) {
                result = 1;
                *out_slot_count = size / 0x206;
            }
        }
        if (file_reference_close(&savegame_directory_file_reference) == 0) {
            result = 0;
        }
    }

    ReleaseMutex(savegame_index_mutex->handle);
    return result;
}

#if 0
Original Ghidra decompilation (0x53e300), from tools/pack.py 0x53e300:

undefined1 FUN_0053e300(undefined4 param_1,uint *param_2)

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
  cVar1 = file_reference_open(2);
  if (cVar1 == '\0') goto LAB_0053e3f1;
  uVar3 = file_reference_get_size();
  if (uVar3 / 0x206 < 999) {
    cVar1 = file_reference_seek();
    if (cVar1 != '\0') {
      cVar1 = file_reference_write();
      if (cVar1 != '\0') {
        local_1 = 1;
        *param_2 = uVar3 / 0x206;
        goto LAB_0053e3df;
      }
    }
    local_1 = 0;
  }
LAB_0053e3df:
  cVar1 = file_reference_close();
  if (cVar1 == '\0') {
    local_1 = 0;
  }
LAB_0053e3f1:
  ReleaseMutex((HANDLE)*DAT_00721440);
  return local_1;
}
#endif
