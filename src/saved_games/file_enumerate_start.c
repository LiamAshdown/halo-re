// file_enumerate_start  (Ghidra: FUN_00555b90, renamed)
// address 0x555b90, size 125 bytes
// name confidence: 0.45   rewrite confidence: 0.65
// evidence: out/phase4/saved_games_functions.md summary "Initializes the shared (possibly
// recursive) directory-enumeration state from a path/flags record, closing any stale find
// handles first." out/phase4/saved_games_types_notes.md file_reference_record: "0x555b90 ...
// (location +0x06 copied to the enumeration state, path +0x08)". Ghidra's decompiled body
// already resolves both parameters cleanly (flags, ref), matching every caller
// (directory_ensure_empty passes (0, &dir_ref)).
// register convention: plain stack arguments (flags, ref); Ghidra fully resolved them.
// UNSURE: file_enumeration_handles is documented as 8 entries (only 8 are seeded to -1
// elsewhere); this function trusts the depth value it is given without bounds-checking it,
// exactly as the binary does.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern file_enumeration_position file_enumeration_pos; // 0x0069fa5c
extern uint32_t file_enumeration_flags_value; // 0x0069fa58
extern void *file_enumeration_handles[8]; // 0x0069fb60
extern char file_enumeration_path[0x100]; // 0x0069fa60


// blam-cc: plain stack arguments (flags, ref)
// Closes any find handles still open from a previous (possibly recursive) enumeration up to and
// including the current depth, then starts a fresh one: stores flags, resets the depth to 0 and
// the location to ref->location, and copies ref->path into the shared enumeration path buffer.
void file_enumerate_start(uint32_t flags, file_reference_record *ref)
{
    int32_t depth;
    int32_t handle_count;
    char *dest;
    const char *src;

    if (file_enumeration_pos.depth >= 0) {
        depth = file_enumeration_pos.depth;
        handle_count = (uint16_t)(file_enumeration_pos.depth + 1);
        do {
            if (file_enumeration_handles[depth] != (void *)-1) {
                FindClose(file_enumeration_handles[depth]);
                file_enumeration_handles[depth] = (void *)-1;
            }
            depth--;
            handle_count--;
        } while (handle_count != 0);
    }
    file_enumeration_flags_value = flags;
    file_enumeration_pos.depth = 0;
    file_enumeration_pos.location = ref->location;
    dest = file_enumeration_path;
    src = ref->path;
    do {
        *dest = *src;
        dest++;
    } while (*src++ != '\0');
    return;
}

#if 0
Original Ghidra decompilation (0x555b90):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00555b90(undefined4 param_1,int param_2)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  uint uVar4;
  undefined4 *puVar5;

  if (-1 < (short)DAT_0069fa5c) {
    puVar5 = (undefined4 *)(&DAT_0069fb60 + (short)DAT_0069fa5c * 4);
    uVar4 = DAT_0069fa5c + 1U & 0xffff;
    do {
      if ((HANDLE)*puVar5 != (HANDLE)0xffffffff) {
        FindClose((HANDLE)*puVar5);
        *puVar5 = 0xffffffff;
      }
      puVar5 = puVar5 + -1;
      uVar4 = uVar4 - 1;
    } while (uVar4 != 0);
  }
  _DAT_0069fa58 = param_1;
  DAT_0069fa5c = (uint)*(ushort *)(param_2 + 6) << 0x10;
  pcVar2 = (char *)(param_2 + 8);
  iVar3 = (int)&DAT_0069fa60 - (int)pcVar2;
  do {
    cVar1 = *pcVar2;
    pcVar2[iVar3] = cVar1;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  return;
}
#endif
