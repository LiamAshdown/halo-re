// saved_game_enumerate_by_type  (Ghidra: saved_game_enumerate_by_type, already named)
// address 0x53c4e0, size 278 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: already named by Ghidra/CEA. Confirmed against objdump 0x53c4e0..0x53c5f0: three
// plain stack arguments (type, out_handles, builtin_only) plus a fourth argument in EBX -- a
// pointer to a word that is BOTH the input capacity of out_handles and, on return, the number
// of handles written (`cmp word[ebx],bp` gates the scan loop, `mov [ebx],di` writes the final
// count at every exit path, including the two early-return paths where the mutex wait fails).
// The index entries are read directly with file_reference_read against the fixed
// savegame_index_file reference (0x00721330), 0x206 bytes at a time, into a local
// saved_game_index_entry-shaped buffer. savegame_slot_handle_pack (FUN_0053e630) (the handle packer, outside this batch) is
// called with the loop index in EAX, the entry's type in ECX, its builtin byte in DL and its
// checksum_valid byte (read as a raw dword, matching the binary) pushed on the stack; this
// matches types/saved_games.h's own evidence note for 0x53e630's call at 0x53c588..0x53c5a7.
// Phase 4 review: matched objdump 0x53c4e0..0x53c5f5 (EBX capacity, stack type / out_handles /
// builtin_only).
// register convention: capacity/count in/out pointer in EBX; type, out_handles and builtin_only
// as ordinary stack arguments in that order.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_game.h"

extern network_mutex_record *saved_game_files_mutex; // 0x0072143c
extern network_mutex_record *savegame_index_mutex; // 0x00721440
extern uint8_t savegame_index_dirty; // 0x00721447
extern file_reference_record savegame_index_file; // 0x00721330 (types/hs.h keeps this opaque as
                                                    // file_reference; same bytes, field layout here)

extern void saved_game_list_rebuild_index(void); // 0x53d720, this module
extern int32_t savegame_index_get_slot_count(void); // 0x53e420, FUN_0053e420 (src/game): current index entry count


extern uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size); // 0x555a20, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module


// blam-cc: EBX -> capacity_and_count, stack -> type, out_handles, builtin_only
// Rebuilds the saved-game index first if it is marked dirty, then scans up to *capacity_and_count
// index entries (rebuilding the count from savegame_index_get_slot_count (FUN_0053e420) as the scan bound) for ones matching
// type, writing their packed handles into out_handles until either the scan bound or the
// caller's capacity is reached. builtin_only selects whether non-builtin entries are skipped.
// *capacity_and_count is always overwritten with the number of handles actually written (0 if
// either mutex wait fails).
// (parameters ordered as the callers declare them; EBX is bound by name)
void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only,
    uint16_t *capacity_and_count)
{
    uint32_t wait_result;
    int32_t entry_count;
    int32_t written;
    uint8_t opened;
    int32_t i;
    saved_game_index_entry entry;
    uint8_t read_ok;
    int32_t handle;

    written = 0;
    wait_result = WaitForSingleObject(saved_game_files_mutex->handle, 5000);
    if (wait_result == 0 || wait_result == 0x80) {
        if (savegame_index_dirty != 0) {
            saved_game_list_rebuild_index();
        }
        entry_count = savegame_index_get_slot_count();
        written = 0;
        wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);
        if (wait_result == 0 || wait_result == 0x80) {
            opened = savegame_index_file_exists();
            if (opened != 0) {
                i = 0;
                if (*capacity_and_count != 0) {
                    do {
                        if (entry_count <= i) {
                            break;
                        }
                        read_ok = file_reference_read(&savegame_index_file, &entry, sizeof(entry));
                        if (read_ok == 0) {
                            break;
                        }
                        if (entry.type == (int16_t)type &&
                            (builtin_only == 1 || entry.builtin == 0)) {
                            handle = savegame_slot_handle_pack(i, entry.type, entry.builtin, entry.checksum_valid);
                            out_handles[written] = handle;
                            written++;
                        }
                        i++;
                    } while (written < (int32_t)(uint32_t)*capacity_and_count);
                }
                file_reference_close(&savegame_index_file);
            }
            ReleaseMutex(savegame_index_mutex->handle);
        }
        ReleaseMutex(saved_game_files_mutex->handle);
    }
    *capacity_and_count = (uint16_t)written;
    return;
}

#if 0
Original Ghidra decompilation (0x53c4e0):

void saved_game_enumerate_by_type(ushort param_1,int param_2,char param_3)

{
  char cVar1;
  DWORD DVar2;
  int iVar3;
  undefined4 uVar4;
  ushort *unaff_EBX;
  int iVar5;
  ushort uVar6;
  int iVar7;
  undefined2 local_c;
  undefined1 local_8;
  undefined4 local_7;

  uVar6 = 0;
  DVar2 = WaitForSingleObject((HANDLE)*DAT_0072143c,5000);
  if ((DVar2 == 0) || (DVar2 == 0x80)) {
    if (DAT_00721447 != '\0') {
      saved_game_list_rebuild_index();
    }
    iVar3 = FUN_0053e420();
    iVar7 = 0;
    DVar2 = WaitForSingleObject((HANDLE)*DAT_00721440,5000);
    if ((DVar2 == 0) || (DVar2 == 0x80)) {
      cVar1 = FUN_0053e060();
      if (cVar1 != '\0') {
        iVar5 = 0;
        if (*unaff_EBX != 0) {
          do {
            if (iVar3 <= iVar5) break;
            cVar1 = file_reference_read();
            if (cVar1 == '\0') break;
            if (((int)local_c == (uint)param_1) && ((param_3 == '\x01' || (local_8 == '\0')))) {
              uVar4 = FUN_0053e630(local_7);
              *(undefined4 *)(param_2 + iVar7 * 4) = uVar4;
              iVar7 = iVar7 + 1;
            }
            iVar5 = iVar5 + 1;
          } while (iVar7 < (int)(uint)*unaff_EBX);
        }
        file_reference_close();
      }
      ReleaseMutex((HANDLE)*DAT_00721440);
    }
    uVar6 = (ushort)iVar7;
    ReleaseMutex((HANDLE)*DAT_0072143c);
  }
  *unaff_EBX = uVar6;
  return;
}
#endif
