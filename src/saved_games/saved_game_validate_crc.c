// saved_game_validate_crc  (Ghidra: saved_game_validate_crc, already named)
// address 0x539570, size 408 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md
// register-convention note (confirmed in objdump per the notes) and the "0x14c game-state
// header" note (both checkpoint readers pass this the lea eax,[esp+0x14c]/[ecx+0x148] pointer,
// i.e. &header_buffer[k_game_state_header_size] == &header_buffer->file_checksum).
// Disassembly at 0x539570 confirms: ECX -> total_size, EDX -> header_size, EBX -> header_buffer
// (all three register args live unmodified into the ReadFile/crc32_update calls); expected_crc
// and corrupt_flag are the two stack parameters Ghidra recognized (EBP-cached and reloaded
// respectively); FUN_0053d080's call at the failure path (0x5396d1) is set up with the current
// profile handle in EAX (0x00714dd4) and a fresh local buffer in ESI, matching its own
// documented (EAX handle, ESI out buffer) convention; the crc32_update call whose args Ghidra
// dropped (0x53960b) is confirmed by the same disassembly to be
// crc32_update(&running_crc, header_buffer, header_size).
// register convention: total_size in ECX, header_size in EDX, header_buffer in EBX; expected_crc
// and corrupt_flag are the recognized stack parameters (param_1, param_2), in that order.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t saved_player_profile_slots_handle; // 0x00714dd4 (saved_player_profile_slots[0].handle)

extern void *game_state_open_persistent_storage(char *unused); // 0x5398e0
extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0
extern void sound_idle_update(void); // 0x549960, sound-clock service tick called between chunked reads

extern uint32_t SetFilePointer(void *file, int32_t distance, void *distance_high, uint32_t method);
extern int32_t ReadFile(void *file, void *buffer, uint32_t bytes_to_read, uint32_t *bytes_read, void *overlapped);
extern void CloseHandle(void *file);
extern int32_t DeleteFileA(const char *path);

// blam-cc: total_size in ECX, header_size in EDX, header_buffer in EBX, then the recognized
// stack parameters (expected_crc, corrupt_flag)
// Opens savegame.bin, reads header_size bytes into header_buffer, then crcs the whole
// total_size-byte image (header included) in up to 0x20000-byte chunks and compares the result
// against *expected_crc (clearing *expected_crc as it goes). If the header read itself fails,
// deletes the current profile's savegame.bin. Returns 1 only on a matching crc.
uint8_t saved_game_validate_crc(int32_t total_size, int32_t header_size, uint8_t *header_buffer,
    uint32_t *expected_crc, uint8_t *corrupt_flag)
{
    void *file;
    uint32_t bytes_read;
    int32_t remaining;
    int32_t chunk;
    uint32_t running_crc;
    uint32_t previous_crc;
    char profile_directory[512];

    file = game_state_open_persistent_storage(0);
    if (corrupt_flag != 0) {
        *corrupt_flag = 0;
    }
    if (file == (void *)0xffffffff) {
        return 0;
    }

    if (SetFilePointer(file, 0, 0, 0) == 0xffffffff ||
        ReadFile(file, header_buffer, header_size, &bytes_read, 0) == 0 ||
        bytes_read != (uint32_t)header_size) {
        if (saved_game_get_directory_by_handle(saved_player_profile_slots_handle, profile_directory) != 0) {
            DeleteFileA(profile_directory);
        }
    } else {
        previous_crc = *expected_crc;
        running_crc = 0xffffffff;
        *expected_crc = 0;
        crc32_update(&running_crc, header_buffer, header_size);

        remaining = total_size - header_size;
        while (0 < remaining) {
            uint8_t chunk_buffer[0x20000];

            chunk = remaining;
            if (0x1ffff < remaining) {
                chunk = 0x20000;
            }
            if (ReadFile(file, chunk_buffer, chunk, &bytes_read, 0) != 0 && bytes_read == (uint32_t)chunk) {
                crc32_update(&running_crc, chunk_buffer, chunk);
            }
            sound_idle_update();
            remaining = remaining - chunk;
        }

        if (running_crc == previous_crc) {
            CloseHandle(file);
            return 1;
        }
        if (corrupt_flag != 0 && previous_crc != 0) {
            *corrupt_flag = 1;
            CloseHandle(file);
            return 0;
        }
    }
    CloseHandle(file);
    return 0;
}

#if 0
Original Ghidra decompilation (0x539570):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 saved_game_validate_crc(int *param_1,undefined1 *param_2)

{
  char cVar1;
  HANDLE hFile;
  DWORD DVar2;
  BOOL BVar3;
  int in_ECX;
  DWORD in_EDX;
  LPVOID unaff_EBX;
  uint uVar4;
  uint nNumberOfBytesToRead;
  int iStack_2010c;
  DWORD DStack_20108;
  int iStack_20104;
  CHAR aCStack_20100 [256];
  undefined1 auStack_20000 [131072];

  hFile = game_state_open_persistent_storage((char *)0x0);
  if (param_2 != (undefined1 *)0x0) {
    *param_2 = 0;
  }
  if (hFile == (HANDLE)0xffffffff) {
    return 0;
  }
  DVar2 = SetFilePointer(hFile,0,(PLONG)0x0,0);
  if (((DVar2 == 0xffffffff) ||
      (BVar3 = ReadFile(hFile,unaff_EBX,in_EDX,&DStack_20108,(LPOVERLAPPED)0x0), BVar3 == 0)) ||
     (DStack_20108 != in_EDX)) {
    cVar1 = FUN_0053d080();
    if (cVar1 != '\0') {
      DeleteFileA(aCStack_20100);
    }
  }
  else {
    iStack_20104 = *param_1;
    iStack_2010c = -1;
    *param_1 = 0;
    crc32_update(&iStack_2010c);
    for (uVar4 = in_ECX - in_EDX; 0 < (int)uVar4; uVar4 = uVar4 - nNumberOfBytesToRead) {
      nNumberOfBytesToRead = uVar4;
      if (0x1ffff < uVar4) {
        nNumberOfBytesToRead = 0x20000;
      }
      BVar3 = ReadFile(hFile,auStack_20000,nNumberOfBytesToRead,&DStack_20108,(LPOVERLAPPED)0x0);
      if ((BVar3 != 0) && (DStack_20108 == nNumberOfBytesToRead)) {
        crc32_update(&iStack_2010c,auStack_20000,nNumberOfBytesToRead);
      }
      FUN_00549960();
    }
    if (iStack_2010c == iStack_20104) {
      CloseHandle(hFile);
      return 1;
    }
    if ((param_2 != (undefined1 *)0x0) && (iStack_20104 != 0)) {
      *param_2 = 1;
      CloseHandle(hFile);
      return 0;
    }
  }
  CloseHandle(hFile);
  return 0;
}
#endif
