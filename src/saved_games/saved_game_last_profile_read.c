// saved_game_last_profile_read  (Ghidra: saved_game_last_profile_read, already named)
// address 0x53d2b0, size 168 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: already named by Ghidra/CEA (__cdecl, one stack argument, already fully resolved).
// out/phase4/saved_games_functions.md summary "Reads the previously saved 'last used player
// profile' record from lastprof.txt into the caller's buffer." Same shape as
// saved_game_last_profile_clear.c but opening for read (mode 1) and always NUL-terminating
// out_data[0xff] before returning, on every path (confirmed by objdump 0x53d2b0..0x53d357: both
// the success and failure exits write that byte).
// register convention: __cdecl, one stack argument (out_data, the 0x100-byte destination).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char last_profile_path[0x100]; // 0x00721b49

extern file_reference_record *file_reference_init(file_reference_record *ref, const char *component, uint8_t is_file); // 0x5554c0, this module
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module
extern uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size); // 0x555a20, this module

// blam-cc: __cdecl, one stack argument (out_data)
// Opens lastprof.txt for read and reads 0x100 bytes into out_data. Always clears
// out_data[0xff] before returning (whether or not the read succeeded). Returns the read result
// (1 on success), or 0 if the file couldn't be opened.
uint8_t saved_game_last_profile_read(uint8_t *out_data)
{
    file_reference_record ref;
    uint8_t ok;
    uint8_t read_ok;

    file_reference_init(&ref, last_profile_path, 0);
    ok = file_reference_open(&ref, 1);
    if (ok != 0) {
        read_ok = file_reference_read(&ref, out_data, 0x100);
        file_reference_close(&ref);
        out_data[0xff] = 0;
        return read_ok;
    }
    out_data[0xff] = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x53d2b0):

char __cdecl saved_game_last_profile_read(int param_1)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_118;
  byte local_114;
  undefined2 local_112;

  puVar3 = &local_118;
  for (iVar2 = 0x43; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  local_118 = 0x66696c6f;
  local_112 = 2;
  if ((local_114 & 1) != 0) {
    path_remove_last_component();
  }
  path_append_component();
  local_114 = local_114 | 1;
  cVar1 = file_reference_open(1);
  if (cVar1 != '\0') {
    cVar1 = file_reference_read();
    file_reference_close();
    *(undefined1 *)(param_1 + 0xff) = 0;
    return cVar1;
  }
  *(undefined1 *)(param_1 + 0xff) = 0;
  return '\0';
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
