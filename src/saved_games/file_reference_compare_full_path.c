// file_reference_compare_full_path  (orphan pass 4: FUN_00483d20, no Ghidra name)
// address 0x483d20, size 245 bytes
// name confidence: 0.4 (src/hs/README.md, "Misattributed functions" #2: "the qsort comparator
//   hs_rebuild_source passes. It builds a 0x100-byte path out of a file_reference via 0x5560d0
//   and compares; the record and both helpers belong to the files module.")
// rewrite confidence: 0.4 (the two build-and-split-and-append sequences and the final
//   comparison are confirmed against the decompilation and match the established
//   path_build_full/path_split_components/path_append_component signatures already used
//   throughout src/saved_games; the intermediate dir/ext out-pointers this function passes to
//   path_split_components and discards are not independently re-derived, matching that
//   function's own "UNSURE" note about the exact role of each out-parameter)
// evidence: types/hs.h "0x00483d20 ... referenced here only as an extern" (from the hs
//   module's own README). types/saved_games.h file_reference_record. This pass places the
//   function in src/saved_games, the module that already owns file_reference_record,
//   path_build_full, path_split_components and path_append_component.
// register convention: two stack arguments (both file_reference_record *, confirmed by
//   Ghidra's own recognized param_1/param_2), the qsort comparator signature.
// blam-cc: file_reference_compare_full_path(const file_reference_record *a, const file_reference_record *b)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern void path_build_full(char *source, char *destination, int16_t location); // 0x5560d0, this module
extern void path_split_components(char **dir_start_out, char *path, char **ext_fallback_out,
    char **name_end_out, char **ext_start_out, uint8_t split_extension); // 0x556000, this module
extern void path_append_component(char *destination, const char *component); // 0x555ec0, this module
extern int32_t __stricmp(const char *a, const char *b);

int32_t file_reference_compare_full_path(const file_reference_record *a, const file_reference_record *b)
{
    char scratch[0x100];
    char *dir_start, *ext_fallback, *name_end, *ext_start;
    char name_a[0x104];
    char name_b[0x104];

    scratch[0] = 0;
    // UNSURE: the original zeroes the whole 0x100-byte scratch buffer before every
    // path_build_full call; matched here with an explicit zero-fill rather than transliterating
    // the dword loop, since only scratch[0] (the empty-string terminator) is ever observed.
    {
        int32_t i;
        for (i = 0; i < 0x100; i++) scratch[i] = 0;
    }
    path_build_full((char *)a->path, scratch, a->location);
    path_split_components(&dir_start, scratch, &ext_fallback, &name_end, &ext_start, a->flags & 1);
    name_a[0] = 0;
    path_append_component(name_a, scratch);

    {
        int32_t i;
        for (i = 0; i < 0x100; i++) scratch[i] = 0;
    }
    path_build_full((char *)b->path, scratch, b->location);
    path_split_components(&dir_start, scratch, &ext_fallback, &name_end, &ext_start, b->flags & 1);
    name_b[0] = 0;
    path_append_component(name_b, scratch);

    return __stricmp(name_a, name_b);
}

#if 0
Original Ghidra decompilation (0x483d20):

void FUN_00483d20(int param_1,int param_2)

{
  int iVar1;
  undefined4 *puVar2;
  undefined1 local_314 [4];
  undefined1 local_310 [4];
  undefined1 local_30c [4];
  undefined1 local_308;
  undefined4 local_307;
  char local_208 [256];
  char local_108 [260];

  local_308 = 0;
  puVar2 = &local_307;
  for (iVar1 = 0x3f; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)puVar2 = 0;
  *(undefined1 *)((int)puVar2 + 2) = 0;
  path_build_full();
  path_split_components(local_310,local_30c,*(byte *)(param_1 + 4) & 1);
  local_108[0] = '\0';
  path_append_component();
  local_308 = 0;
  puVar2 = &local_307;
  for (iVar1 = 0x3f; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)puVar2 = 0;
  *(undefined1 *)((int)puVar2 + 2) = 0;
  path_build_full();
  path_split_components(local_310,local_314,*(byte *)(param_2 + 4) & 1);
  local_208[0] = '\0';
  path_append_component();
  __stricmp(local_108,local_208);
  return;
}
#endif
