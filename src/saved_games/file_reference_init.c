// file_reference_init  (Ghidra: FUN_005554c0, renamed)
// address 0x5554c0, size 96 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_functions.md summary "Initializes a generic file-reference
// record (magic, type, and name) used by the saved-games file I/O helpers."; out/phase4/
// saved_games_types_notes.md "file_reference_record (0x10c)": zeroes 0x43 dwords, stores the
// signature, location 2 (absolute), appends the path and sets the is-file bit -- exactly the
// pattern several other constructors in this module repeat inline (saved_game_open_file_by_handle,
// the last*.txt helpers, the default-file writers).
// Ghidra's decompiled body never uses its second parameter, but objdump 0x5554c0..0x55551a shows
// it IS used: `mov ebx,[esp+0x18]` (the second stack argument) loads path_append_component's EBX
// argument at every call site, even though Ghidra rendered those calls with no visible arguments.
// register convention: three plain stack arguments -- reference record, component string, is_file
// flag (confirmed by objdump: [esp+0xc]/[esp+0x18]/[esp+0x1c] relative to the prologue's four
// pushes are the first, second and third stack slots respectively). Returns the reference record
// pointer in EAX.

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

extern void path_append_component(char *destination, const char *component); // 0x555ec0, this module
extern void path_remove_last_component(char *path); // 0x555f80, this module

// blam-cc: plain stack arguments (ref, component, is_file)
// Zeroes the record, sets its signature and location (always _file_location_absolute), then
// either appends component as the whole path (is_file != 0: the caller is naming a single
// file, no prior path to trim) or, only if flags already has the is-file bit set from a prior
// call, removes the previous trailing component first, then appends component and sets the
// is-file bit. Returns ref.
file_reference_record *file_reference_init(file_reference_record *ref, const char *component, uint8_t is_file)
{
    uint8_t *zero_cursor;
    int32_t i;

    zero_cursor = (uint8_t *)ref;
    for (i = 0x43; i != 0; i--) {
        *(uint32_t *)zero_cursor = 0;
        zero_cursor += 4;
    }
    ref->signature = k_file_reference_signature;
    ref->location = _file_location_absolute;
    if (is_file != 0) {
        path_append_component(ref->path, component);
        return ref;
    }
    if ((ref->flags & _file_reference_is_file_bit) != 0) {
        path_remove_last_component(ref->path);
    }
    path_append_component(ref->path, component);
    ref->flags = ref->flags | _file_reference_is_file_bit;
    return ref;
}

#if 0
Original Ghidra decompilation (0x5554c0):

undefined4 * FUN_005554c0(undefined4 *param_1,undefined4 param_2,char param_3)

{
  int iVar1;
  undefined4 *puVar2;

  puVar2 = param_1;
  for (iVar1 = 0x43; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *param_1 = 0x66696c6f;
  *(undefined2 *)((int)param_1 + 6) = 2;
  if (param_3 != '\0') {
    path_append_component();
    return param_1;
  }
  if ((*(byte *)(param_1 + 1) & 1) != 0) {
    path_remove_last_component();
  }
  path_append_component();
  *(byte *)(param_1 + 1) = *(byte *)(param_1 + 1) | 1;
  return param_1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
