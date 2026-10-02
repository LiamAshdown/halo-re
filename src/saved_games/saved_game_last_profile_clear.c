// saved_game_last_profile_clear  (Ghidra: saved_game_last_profile_clear, already named)
// address 0x53d220, size 142 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: already named by Ghidra/CEA, but Ghidra's `void saved_game_last_profile_clear(void)`
// signature is wrong: objdump 0x53d220..0x53d2ad shows `mov ecx,[ebp+0x8]` feeding
// file_reference_write's ECX=buffer argument, i.e. this function takes one stack argument (the
// 0x100-byte record to write) that Ghidra simply failed to attribute. EBX is set to the fixed
// global last_profile_path (0x00721b49) right before path_append_component, confirming the
// created/truncated file is always lastprof.txt regardless of caller.
// register convention: __cdecl, one stack argument (data, the 0x100 bytes written to
// lastprof.txt).

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
extern uint8_t file_reference_create(file_reference_record *ref); // 0x5555b0, this module
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, this module

// blam-cc: __cdecl, one stack argument (data)
// Creates (or truncates) lastprof.txt and writes 0x100 bytes from data into it.
void saved_game_last_profile_clear(const void *data)
{
    file_reference_record ref;
    uint8_t ok;

    file_reference_init(&ref, last_profile_path, 0);
    ok = file_reference_create(&ref);
    if (ok != 0) {
        ok = file_reference_open(&ref, 2);
        if (ok != 0) {
            file_reference_write(&ref, data, 0x100);
            file_reference_close(&ref);
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x53d220):

void __cdecl saved_game_last_profile_clear(void)

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
  cVar1 = file_reference_create();
  if (cVar1 != '\0') {
    cVar1 = file_reference_open(2);
    if (cVar1 != '\0') {
      file_reference_write();
      file_reference_close();
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
