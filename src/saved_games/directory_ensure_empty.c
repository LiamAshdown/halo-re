// directory_ensure_empty  (Ghidra: FUN_00555520, renamed)
// address 0x555520, size 144 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/saved_games_functions.md summary "Ensures a directory exists and is
// empty, creating it if missing or deleting all of its contents if it already exists."
// Confirmed against objdump 0x555520..0x5555af: ESI is overwritten locally (it is not an input);
// EBX is never assigned before path_append_component's call, so it is a genuine incoming
// register argument forwarded straight through, exactly like directory_path here.
// register convention: directory path (component appended to a fresh, location=-1 reference)
// in EBX. No stack arguments.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern void path_append_component(char *destination, const char *component); // 0x555ec0, this module
extern uint8_t file_reference_exists(file_reference_record *ref); // 0x555720, this module
extern uint8_t file_reference_create(file_reference_record *ref); // 0x5555b0, this module
extern void file_enumerate_start(uint32_t flags, file_reference_record *ref); // 0x555b90, this module
extern uint8_t file_enumerate_find_next(file_reference_record *out_entry, uint32_t *out_write_time); // 0x555c10, this module
extern uint8_t file_reference_delete(file_reference_record *ref); // 0x555670, this module

// blam-cc: directory path in EBX
// Builds a fresh, relative (_file_location_relative) file_reference_record naming
// directory_path. If it doesn't exist yet, creates it. If it does, enumerates every entry
// directly inside it and deletes each one (the directory itself is left in place, now empty).
void directory_ensure_empty(const char *directory_path)
{
    file_reference_record dir_ref;
    file_reference_record entry;
    uint8_t exists;
    uint8_t found;

    memset(&dir_ref, 0, sizeof(dir_ref));
    dir_ref.signature = k_file_reference_signature;
    dir_ref.location = _file_location_relative;
    path_append_component(dir_ref.path, directory_path);

    exists = file_reference_exists(&dir_ref);
    if (!exists) {
        file_reference_create(&dir_ref);
    } else {
        file_enumerate_start(0, &dir_ref);
        found = file_enumerate_find_next(&entry, 0);
        if (found != 0) {
            do {
                file_reference_delete(&entry);
                found = file_enumerate_find_next(&entry, 0);
            } while (found != 0);
            return;
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x555520):

void FUN_00555520(void)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_228;
  undefined2 local_222;
  undefined1 local_118 [276];

  puVar3 = &local_228;
  for (iVar2 = 0x43; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  local_228 = 0x66696c6f;
  local_222 = 0xffff;
  path_append_component();
  cVar1 = file_reference_exists();
  if (cVar1 == '\0') {
    file_reference_create();
  }
  else {
    FUN_00555b90(0,&local_228);
    cVar1 = file_enumerate_find_next(local_118,0);
    if (cVar1 != '\0') {
      do {
        file_reference_delete();
        cVar1 = file_enumerate_find_next(local_118,0);
      } while (cVar1 != '\0');
      return;
    }
  }
  return;
}
#endif
