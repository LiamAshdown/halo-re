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
// REWRITTEN (objdump 0x53e4a0..0x53e62f): every file_reference call had its register arguments elided. Verified:
//   0x53e52a get_size_by_path(ESI ref, stack &size)   0x53e556 open(ESI ref, stack 3)
//   0x53e568 seek(EAX slot*0x206, ECX ref)            0x53e580 seek(EAX read offset, ECX ref)
//   0x53e590 read(EDX ref, ECX record, ESI 0x206)     0x53e5a7 seek(EAX write offset, ECX ref)
//   0x53e5b7 write(EDX ref, ECX record, ESI 0x206)    0x53e5e2 set_length(EAX size-0x206, ESI ref)
//   0x53e5f6 close(ESI ref). The mutex wait failing returns 0 without releasing it (0x53e625).
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char saved_game_root_path[]; // 0x006e3108 (an array: the original passes its address), the appended component
extern file_reference_record savegame_index_file; // 0x00721330
extern network_mutex_record *savegame_index_mutex; // 0x00721440, networking.h record; +0x00 is the HANDLE

extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, ESI ref, stack mode
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, ESI ref
extern uint8_t file_reference_seek(int32_t offset, file_reference_record *ref); // 0x5558f0, EAX offset, ECX ref
extern uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size); // 0x555a20, EDX ref, ECX buffer, ESI size
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, EDX ref, ECX buffer, ESI size
extern uint8_t file_reference_set_length(int32_t offset, file_reference_record *ref); // 0x5559b0, EAX offset, ESI ref
extern void path_append_component(char *destination, const char *component); // 0x555ec0, ESI destination, EBX component
extern void path_remove_last_component(char *path); // 0x555f80, EBX path
extern uint8_t file_reference_get_size_by_path(file_reference_record *ref, uint32_t *out_size); // 0x555b00, ESI ref, stack out_size

// Removes save-slot `slot` from the index file: every 0x206-byte record after it is read and written back one record
// earlier, then the file is truncated by one record. Returns 1 on success, 0 otherwise.
uint8_t savegame_index_remove_slot(uint16_t slot)
{
    file_reference_record *ref = &savegame_index_file;
    uint8_t record[0x206];
    uint32_t size;
    uint32_t read_offset, write_offset;
    uint8_t result = 0;
    uint32_t wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);

    if (wait_result != 0 && wait_result != 0x80) {
        return 0;
    }

    {
        uint32_t *raw = (uint32_t *)ref;
        int32_t i;
        for (i = 0; i < 0x43; i++) {
            raw[i] = 0;
        }
        raw[0] = 0x66696c6f;
        *(uint16_t *)((uint8_t *)ref + 6) = 2;
        if ((*((uint8_t *)ref + 4) & 1) != 0) {
            path_remove_last_component((char *)ref + 8);
        }
        path_append_component((char *)ref + 8, saved_game_root_path);
        *((uint8_t *)ref + 4) |= 1;
    }

    if (file_reference_get_size_by_path(ref, &size) != 0) {
        write_offset = (uint32_t)slot * 0x206;
        read_offset = write_offset + 0x206;
        if (read_offset <= size && file_reference_open(ref, 3) != 0) {
            result = file_reference_seek((int32_t)write_offset, ref);
            if (result == 1) {
                for (; read_offset < size; read_offset += 0x206, write_offset += 0x206) {
                    if (file_reference_seek((int32_t)read_offset, ref) == 0 ||
                        file_reference_read(ref, record, 0x206) == 0 ||
                        file_reference_seek((int32_t)write_offset, ref) == 0 ||
                        file_reference_write(ref, record, 0x206) == 0) {
                        result = 0;
                        goto close_file;
                    }
                }
                result = file_reference_set_length((int32_t)(size - 0x206), ref);
            } else if (result != 0) {
                result = file_reference_set_length((int32_t)(size - 0x206), ref);
            }
        close_file:
            if (file_reference_close(ref) == 0) {
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
