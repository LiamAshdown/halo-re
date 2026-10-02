// saved_game_last_mp_variant_read  (Ghidra: FUN_0053d3f0, renamed)
// address 0x53d3f0, size 168 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md summary "Reads the previously saved 'last used
// multiplayer variant' record from lastmpvr.txt." Byte-identical shape to
// saved_game_last_profile_read.c (0x53d2b0) apart from the target path global
// (last_game_variant_path, 0x00721c49).
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

extern char last_game_variant_path[0x100]; // 0x00721c49

extern file_reference_record *file_reference_init(file_reference_record *ref, const char *component, uint8_t is_file); // 0x5554c0, this module
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module
extern uint8_t file_reference_read(file_reference_record *ref, void *buffer, uint32_t size); // 0x555a20, this module

// blam-cc: __cdecl, one stack argument (out_data)
// Opens lastmpvr.txt for read and reads 0x100 bytes into out_data. Always clears
// out_data[0xff] before returning. Returns the read result (1 on success), or 0 if the file
// couldn't be opened.
uint8_t saved_game_last_mp_variant_read(uint8_t *out_data)
{
    file_reference_record ref;
    uint8_t ok;
    uint8_t read_ok;

    file_reference_init(&ref, last_game_variant_path, 0);
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
Original Ghidra decompilation (0x53d3f0):

undefined1 FUN_0053d3f0(int param_1)

{
  char cVar1;
  undefined1 uVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 local_118;
  byte local_114;
  undefined2 local_112;

  puVar4 = &local_118;
  for (iVar3 = 0x43; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
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
    uVar2 = file_reference_read();
    file_reference_close();
    *(undefined1 *)(param_1 + 0xff) = 0;
    return uVar2;
  }
  *(undefined1 *)(param_1 + 0xff) = 0;
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
