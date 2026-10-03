// hs_rebuild_source  (Ghidra: hs_rebuild_source, already named)
// address 0x483e20, size 616 bytes (0x483e20..0x484087)
// name confidence: 0.7   rewrite confidence: 0.75
// NOTE ON SIZE: out/functions.json records size 288 because Ghidra cut the function at 0x483f40
// (the "add edi,0x10c" in the enumeration loop, which a backward branch made look like an entry
// point -- see the misattributed-functions note for 0x483f40). The real function runs to the
// "ret" at 0x484087. Everything below was re-derived instruction by instruction from the retail
// bytes, and the frame slots were resolved by tracking esp across the pushes:
//   F+0x003 found flag (byte)            F+0x004 loop counter
//   F+0x008/0x00c/0x010/0x014 the four out-pointers 0x556000 fills; F+0x008 is the extension
//   F+0x018 global_scripts reference     F+0x128 extension buffer [256]
//   F+0x228 scripts-directory reference  F+0x338 display-name buffer [256]
//   F+0x438 path buffer [256]            F+0x538 entries[8] (8 * 0x10c)
// register conventions recovered here (none of them were visible in the decompile):
//   path_append_component      @0x555ec0  ESI -> &reference->path, EBX -> component string
//   path_remove_last_component @0x555f80  EBX -> &reference->path
//   file_reference_exists      @0x555720  EAX -> reference
//   path_build     @0x5560d0  EAX -> &reference->path, EDX -> out buffer, CX -> reference+6
//   path_split     @0x556000  ESI -> path, EBX/EDI -> two out slots,
//                             stack (out_slot, out_extension, flags & 1)
// file_reference offsets proved here: +0x00 signature 0x66696c6f, +0x04 flags byte (bit 0 = "a
// component has been appended"), +0x06 uint16 seeded to 0xffff, +0x08 the path itself.
// UNSURE (open question for hook verification): file_reference_exists @0x555720 is called a
// second time on the SAME reference at 0x483ecf, and once more at 0x48405b, with the result
// discarded both times, immediately before the "found" flag is cleared. A pure existence
// predicate has no reason to be invoked twice on one argument, so the second site is most likely
// a different files-module routine -- the one that actually appends the file's contents to the
// combined source buffer, which is the step this function's name implies and which is otherwise
// absent -- folded onto the same address by /OPT:ICF. Preserved literally as the call the bytes
// make.
// UNSURE: the two "hsc" literals at 0x660f90 and 0x660fa8 are distinct addresses with identical
// contents; only 0x660f90 is used, by a 4-byte repe cmpsb, so the comparison includes the
// terminating NUL (an exact match on the extension, not a prefix test).
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "hs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FIXED 2026-09-28 (retail-independence loop): rewritten from objdump 0x483e20..0x484087 with the file helpers'
//   register arguments restored (path_append_component ESI/EBX, path_remove_last_component EBX, path_build_full
//   EAX/EDX/CX, path_split_components EBX/ESI/EDI + stack); the earlier version called them without arguments and
//   named the qsort comparator file_reference_compare_by_name, which bound to nothing -- it is
//   file_reference_compare_full_path (0x483d20).

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
typedef struct rebuild_file_reference {
    uint32_t signature; // 0x000 'filo'
    uint8_t flags;      // 0x004
    uint8_t unknown_005;
    int16_t location;   // 0x006
    char path[0x100];   // 0x008
    void *handle;       // 0x108
} rebuild_file_reference; // size 0x10c == k_hs_file_reference_size

extern uint8_t file_reference_exists(rebuild_file_reference *ref); // 0x555720, blam-cc: EAX
extern void file_enumerate_start(uint32_t flags, rebuild_file_reference *ref); // 0x555b90
extern uint8_t file_enumerate_find_next(rebuild_file_reference *out_entry, uint32_t *out_write_time); // 0x555c10
extern void path_append_component(char *destination, const char *component); // 0x555ec0, blam-cc: ESI destination, EBX component
extern void path_remove_last_component(char *path); // 0x555f80, blam-cc: EBX
extern void path_build_full(char *source, char *destination, int16_t location); // 0x5560d0, blam-cc: EAX, EDX, CX
extern void path_split_components(char **dir_start_out, char *path, char **ext_fallback_out,
    char **name_end_out, char **ext_start_out, uint8_t split_extension); // 0x556000, blam-cc: EBX, ESI, EDI, stack
extern int32_t file_reference_compare_full_path(const void *a, const void *b); // 0x483d20

extern datum_index global_scenario_index; // 0x0069e8d4
extern tag_instance *tag_instances;       // 0x0087bc14

// Looks for the HS source on disk: "data\global_scripts.hsc" and every *.hsc in "data\<scenario tag
// directory>\scripts" (at most 8 entries, sorted). Returns 1 when NOTHING was found -- each existing file clears it.
char hs_rebuild_source(void)
{
    char directory_path[0x100];
    char display_name[0x100];
    char extension[0x100];
    rebuild_file_reference global_scripts;
    rebuild_file_reference scripts_directory;
    rebuild_file_reference entries[8];
    char *dir_start;
    char *ext_fallback;
    char *name_end;
    char *ext_start;
    char nothing_found = 1;
    int16_t count;
    int16_t i;

    sprintf(directory_path, "data\\%s", tag_instances[(int16_t)global_scenario_index].path); // 0x00660fb4
    sprintf(strrchr(directory_path, '\\') + 1, "scripts"); // 0x00660fac

    memset(&global_scripts, 0, sizeof(global_scripts));
    global_scripts.signature = 0x66696c6f;
    global_scripts.location = -1;
    if ((global_scripts.flags & 1) != 0) {
        path_remove_last_component(global_scripts.path);
    }
    path_append_component(global_scripts.path, "data\\global_scripts.hsc"); // 0x00660f94
    global_scripts.flags |= 1;
    if (file_reference_exists(&global_scripts) != 0) {
        file_reference_exists(&global_scripts); // the binary calls it twice, the second result unused
        nothing_found = 0;
    }

    memset(&scripts_directory, 0, sizeof(scripts_directory));
    scripts_directory.signature = 0x66696c6f;
    scripts_directory.location = -1;
    path_append_component(scripts_directory.path, directory_path);
    file_enumerate_start(0, &scripts_directory);
    for (count = 0; count < 8; count++) {
        if (file_enumerate_find_next(&entries[count], 0) == 0) {
            break;
        }
    }
    qsort(entries, count, sizeof(rebuild_file_reference), (int (*)(const void *, const void *))file_reference_compare_full_path);

    for (i = 0; i < count; i++) {
        memset(display_name, 0, sizeof(display_name));
        path_build_full(entries[i].path, display_name, entries[i].location);
        path_split_components(&dir_start, display_name, &ext_fallback, &name_end, &ext_start, (uint8_t)(entries[i].flags & 1));
        extension[0] = 0;
        if (*ext_start != 0) {
            char *end = extension + strlen(extension);

            if (end != extension) { // never: the buffer was just emptied
                *end++ = '.';
                *end = 0;
            }
            strncpy(end, ext_start, 0xff - strlen(extension));
            extension[0xff] = 0;
        }
        if (strcmp(extension, "hsc") == 0) { // 0x00660f90, 4-byte compare including the NUL
            file_reference_exists(&entries[i]);
            nothing_found = 0;
        }
    }
    return nothing_found;
}

#if 0
Original Ghidra decompilation (0x483e20):

char __cdecl hs_rebuild_source(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  undefined4 *puVar4;
  bool bVar5;
  char *pcVar6;
  bool local_d9d;
  uint uStack_d9c;
  char *pcStack_d98;
  undefined4 local_d88;
  byte local_d84;
  undefined2 local_d82;
  char acStack_c78 [255];
  undefined1 uStack_b79;
  undefined4 local_b78;
  undefined2 local_b72;
  undefined1 uStack_a68;
  undefined4 uStack_a67;
  char local_968 [256];
  undefined1 local_868 [2148];

  _sprintf(local_968,"data\\%s");
  pcVar6 = "scripts";
  pcVar2 = _strrchr(local_968,0x5c);
  _sprintf(pcVar2 + 1,pcVar6);
  puVar4 = &local_d88;
  for (iVar3 = 0x43; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  local_d88 = 0x66696c6f;
  local_d82 = 0xffff;
  if ((local_d84 & 1) != 0) {
    path_remove_last_component();
  }
  path_append_component();
  local_d84 = local_d84 | 1;
  cVar1 = file_reference_exists();
  if (cVar1 != '\0') {
    file_reference_exists();
  }
  local_d9d = cVar1 == '\0';
  puVar4 = &local_b78;
  for (iVar3 = 0x43; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  local_b78 = 0x66696c6f;
  local_b72 = 0xffff;
  path_append_component();
  uStack_d9c = 0;
  FUN_00555b90();
  do {
    cVar1 = file_enumerate_find_next();
    if (cVar1 == '\0') break;
    uStack_d9c = uStack_d9c + 1;
  } while ((int)uStack_d9c < 8);
  _qsort(local_868,(int)(short)uStack_d9c,0x10c,FUN_00483d20);
  if (0 < (short)uStack_d9c) {
    uStack_d9c = uStack_d9c & 0xffff;
    do {
      uStack_a68 = 0;
      puVar4 = &uStack_a67;
      for (iVar3 = 0x3f; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar4 = 0;
        puVar4 = puVar4 + 1;
      }
      *(undefined2 *)puVar4 = 0;
      *(undefined1 *)((int)puVar4 + 2) = 0;
      FUN_005560d0();
      FUN_00556000();
      acStack_c78[0] = '\0';
      if (*pcStack_d98 != '\0') {
        pcVar2 = acStack_c78;
        do {
          cVar1 = *pcVar2;
          pcVar2 = pcVar2 + 1;
        } while (cVar1 != '\0');
        pcVar6 = &stack0xfffff250 + (int)(pcVar2 + (-0xd - (int)&stack0xfffff244));
        if (pcVar6 != acStack_c78) {
          *pcVar6 = '.';
          pcVar6 = &stack0xfffff251 + (int)(pcVar2 + (-0xd - (int)&stack0xfffff244));
          *pcVar6 = '\0';
        }
        pcVar2 = acStack_c78;
        do {
          cVar1 = *pcVar2;
          pcVar2 = pcVar2 + 1;
        } while (cVar1 != '\0');
        _strncpy(pcVar6,pcStack_d98,0xff - ((int)pcVar2 - (int)(acStack_c78 + 1)));
        uStack_b79 = 0;
      }
      iVar3 = 4;
      bVar5 = true;
      pcVar2 = acStack_c78;
      pcVar6 = "hsc";
      do {
        if (iVar3 == 0) break;
        iVar3 = iVar3 + -1;
        bVar5 = *pcVar2 == *pcVar6;
        pcVar2 = pcVar2 + 1;
        pcVar6 = pcVar6 + 1;
      } while (bVar5);
      if (bVar5) {
        file_reference_exists();
        local_d9d = false;
      }
      uStack_d9c = uStack_d9c - 1;
    } while (uStack_d9c != 0);
  }
  return local_d9d;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
