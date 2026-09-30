// saved_game_find_by_name  (Ghidra: saved_game_find_by_name, already named)
// address 0x53d4a0, size 306 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: already named by Ghidra/CEA, both parameters fully resolved (char *name, short
// type). out/phase4/saved_games_functions.md summary "Searches the saved-game index for an entry
// with the given type and name (case-insensitive) and returns its handle, or -1 if not found."
// The stack layout (a 512-byte buffer, then a short, then a dword) matches
// saved_game_index_entry exactly (path[0x100] + display_name[0x80] = 512 bytes, then type at
// 0x200, index at 0x202); the savegame_slot_handle_pack (FUN_0053e630) call is given the same four separate arguments
// (index, type, builtin, checksum_valid) as the other call site this module already established
// in saved_game_enumerate_by_type.c, rather than Ghidra's single collapsed `local_7` dword
// (index and the two following bytes read together, another instance of the register/argument
// drops seen throughout this module).
// The manual byte-walk that computes pcVar2 (used only for its distance from name) is an inlined
// strlen(name); replaced with the CRT call for readability, not behavior.
// Phase 4 review: matched objdump 0x53d4a0..0x53d5d1; the handle is packed from the loop counter
// (EAX = EDI at 0x53d58c), not entry.index.
// register convention: name and type as ordinary stack arguments (already resolved by Ghidra).

#include "crt.h"
#include "win32.h"
#include <string.h>
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
extern file_reference_record savegame_index_file; // 0x00721330

extern int32_t savegame_index_get_slot_count(void); // 0x53e420, FUN_0053e420 (src/game): entry count

extern uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size); // 0x555a20, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module


// blam-cc: plain stack arguments (name, type)
// Under both module mutexes, opens the save-game index file and scans every entry for one whose
// type matches and whose path matches name (case-insensitive, up to strlen(name) characters),
// returning its packed handle. Returns -1 if not found or if either mutex wait, the entry count,
// or the index open fails.
int32_t saved_game_find_by_name(char *name, int16_t type)
{
    uint32_t name_length;
    uint32_t wait_result;
    int32_t entry_count;
    uint8_t opened;
    saved_game_index_entry entry;
    uint8_t read_ok;
    int32_t i;
    int32_t handle;
    int32_t result;

    result = -1;
    handle = -1;
    name_length = strlen(name);

    wait_result = WaitForSingleObject(saved_game_files_mutex->handle, 5000);
    if (wait_result == 0 || wait_result == 0x80) {
        wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);
        if (wait_result == 0 || wait_result == 0x80) {
            entry_count = savegame_index_get_slot_count();
            opened = savegame_index_file_exists();
            if (opened != 0) {
                for (i = 0; i < entry_count; i++) {
                    read_ok = file_reference_read(&savegame_index_file, &entry, sizeof(entry));
                    if (read_ok == 0) {
                        break;
                    }
                    if (entry.type == type &&
                        _strnicmp(name, entry.path, name_length) == 0) {
                        handle = savegame_slot_handle_pack(i, entry.type, entry.builtin, entry.checksum_valid);
                        break;
                    }
                }
                file_reference_close(&savegame_index_file);
                result = handle;
            }
            ReleaseMutex(savegame_index_mutex->handle);
        }
        ReleaseMutex(saved_game_files_mutex->handle);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x53d4a0):

undefined4 saved_game_find_by_name(char *param_1,short param_2)

{
  char cVar1;
  char *pcVar2;
  DWORD DVar3;
  int iVar4;
  int iVar5;
  undefined4 uVar6;
  int iVar7;
  undefined4 local_210;
  char local_20c [512];
  short local_c;
  undefined4 local_7;

  uVar6 = 0xffffffff;
  local_210 = 0xffffffff;
  pcVar2 = param_1;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  DVar3 = WaitForSingleObject((HANDLE)*DAT_0072143c,5000);
  if ((DVar3 == 0) || (DVar3 == 0x80)) {
    DVar3 = WaitForSingleObject((HANDLE)*DAT_00721440,5000);
    if ((DVar3 == 0) || (DVar3 == 0x80)) {
      iVar4 = FUN_0053e420();
      cVar1 = FUN_0053e060();
      if (cVar1 != '\0') {
        iVar7 = 0;
        if (0 < iVar4) {
          do {
            cVar1 = file_reference_read();
            if (cVar1 == '\0') break;
            if ((local_c == param_2) &&
               (iVar5 = __strnicmp(param_1,local_20c,(int)pcVar2 - (int)(param_1 + 1)), iVar5 == 0))
            {
              local_210 = FUN_0053e630(local_7);
              break;
            }
            iVar7 = iVar7 + 1;
          } while (iVar7 < iVar4);
        }
        file_reference_close();
        uVar6 = local_210;
      }
      ReleaseMutex((HANDLE)*DAT_00721440);
    }
    ReleaseMutex((HANDLE)*DAT_0072143c);
  }
  return uVar6;
}
#endif
