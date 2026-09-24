// game_state_write_persistent_storage  (Ghidra: game_state_write_persistent_storage, already named)
// address 0x539710, size 319 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: out/phase4/saved_games_functions.md; Ghidra recognized all three stack parameters
// (buffer, header_size, total_size) correctly here; EAX (crc_slot) confirmed by the caller
// game_state_save_thread_proc's &buffer[0x148] setup (disassembly 0x538a41), matching
// out/phase4/saved_games_types_notes.md's "+0x148 is lea eax,[ecx+0x148]... in EAX" note. The
// two-phase write (whole image with the header zeroed, then seek back and patch in the real
// header) is a crash-safety commit pattern: a write interrupted after the first WriteFile
// leaves the on-disk header zeroed (and so failing crc validation) rather than half-updated.
// register convention: crc_slot in EAX; buffer, header_size and total_size are the recognized
// stack parameters (Ghidra's own param_1/param_2/param_3).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t saved_player_profile_slots_handle; // 0x00714dd4

extern void *game_state_open_persistent_storage(char *name); // 0x5398e0
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0
extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70
extern uint32_t SetFilePointer(void *file, int32_t distance, void *distance_high, uint32_t method); // Win32
extern uint32_t WriteFile(void *file, const void *buffer, uint32_t bytes_to_write, uint32_t *bytes_written, void *overlapped); // Win32
extern uint32_t DeleteFileA(const char *path); // Win32
extern uint32_t CloseHandle(void *handle); // Win32
extern void *memcpy(void *dest, const void *src, uint32_t count);
extern void *memset(void *dest, int32_t value, uint32_t count);

// blam-cc: crc_slot in EAX, then the recognized stack parameters (buffer, header_size, total_size)
// Crcs the whole total_size-byte image into *crc_slot (a field inside the header at the start
// of buffer, zeroed first so it doesn't crc its own old value), then writes the file in two
// passes: the full image with the header portion zeroed, then a second, header-only write that
// patches the real header back in -- so a write interrupted after the first pass leaves an
// on-disk header that still fails validation rather than a half-written one. Restores buffer's
// header bytes in memory afterward regardless of success. Raises a fatal error dialog and
// attempts to delete the (default profile) save file if either write fails.
void game_state_write_persistent_storage(uint32_t *crc_slot, uint8_t *buffer, int32_t header_size, int32_t total_size)
{
    void *file;
    uint32_t running_crc;
    uint8_t header_backup[2052];
    uint32_t bytes_written;
    char directory[264];

    file = game_state_open_persistent_storage(0);
    if (file == (void *)0xffffffff) {
        return;
    }

    *crc_slot = 0;
    running_crc = 0xffffffff;
    crc32_update(&running_crc, buffer, total_size);
    *crc_slot = running_crc;

    memcpy(header_backup, buffer, header_size);
    memset(buffer, 0, header_size);

    if (SetFilePointer(file, 0, 0, 0) == 0xffffffff ||
        WriteFile(file, buffer, total_size, &bytes_written, 0) == 0 ||
        bytes_written != (uint32_t)total_size ||
        SetFilePointer(file, 0, 0, 0) == 0xffffffff ||
        WriteFile(file, header_backup, header_size, &bytes_written, 0) == 0 ||
        bytes_written != (uint32_t)header_size) {
        shell_display_fatal_error_dialog(0x8b, 0x8c, 1);
        if (saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory) != 0) {
            DeleteFileA(directory);
        }
    }

    memcpy(buffer, header_backup, header_size);
    CloseHandle(file);
}

#if 0
Original Ghidra decompilation (0x539710):

void game_state_write_persistent_storage(undefined4 *param_1,uint param_2,DWORD param_3)

{
  char cVar1;
  uint *in_EAX;
  HANDLE hFile;
  DWORD DVar2;
  BOOL BVar3;
  uint uVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  DWORD local_90c;
  CHAR local_908 [256];
  undefined4 local_808 [513];

  hFile = game_state_open_persistent_storage((char *)0x0);
  if (hFile != (HANDLE)0xffffffff) {
    *in_EAX = 0;
    local_90c = 0xffffffff;
    crc32_update(&local_90c,param_1,0x440000);
    *in_EAX = local_90c;
    puVar5 = param_1;
    puVar6 = local_808;
    for (uVar4 = param_2 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    for (uVar4 = param_2 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *(undefined1 *)puVar6 = *(undefined1 *)puVar5;
      puVar5 = (undefined4 *)((int)puVar5 + 1);
      puVar6 = (undefined4 *)((int)puVar6 + 1);
    }
    puVar5 = param_1;
    for (uVar4 = param_2 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    for (uVar4 = param_2 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *(undefined1 *)puVar5 = 0;
      puVar5 = (undefined4 *)((int)puVar5 + 1);
    }
    DVar2 = SetFilePointer(hFile,0,(PLONG)0x0,0);
    if ((((DVar2 == 0xffffffff) ||
         (BVar3 = WriteFile(hFile,param_1,param_3,&local_90c,(LPOVERLAPPED)0x0), BVar3 == 0)) ||
        (local_90c != param_3)) ||
       (((DVar2 = SetFilePointer(hFile,0,(PLONG)0x0,0), DVar2 == 0xffffffff ||
         (BVar3 = WriteFile(hFile,local_808,param_2,&local_90c,(LPOVERLAPPED)0x0), BVar3 == 0)) ||
        (local_90c != param_2)))) {
      shell_display_fatal_error_dialog(0x8b,0x8c,1);
      cVar1 = FUN_0053d080();
      if (cVar1 != '\0') {
        DeleteFileA(local_908);
      }
    }
    puVar5 = local_808;
    for (uVar4 = param_2 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
      *param_1 = *puVar5;
      puVar5 = puVar5 + 1;
      param_1 = param_1 + 1;
    }
    for (param_2 = param_2 & 3; param_2 != 0; param_2 = param_2 - 1) {
      *(undefined1 *)param_1 = *(undefined1 *)puVar5;
      puVar5 = (undefined4 *)((int)puVar5 + 1);
      param_1 = (undefined4 *)((int)param_1 + 1);
    }
    CloseHandle(hFile);
  }
  return;
}
#endif
