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

// file_reference: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

extern char file_reference_exists(file_reference *reference); // blam-cc: EAX -> reference; 0x555720
extern void file_enumerate_start(int32_t flags, file_reference *directory); // files, 0x555b90
extern char file_enumerate_find_next(file_reference *out_entry, int32_t param_2); // files, 0x555c10
extern void path_append_component(void);
    // blam-cc: ESI -> &reference->path, EBX -> component; files module, 0x555ec0
extern void path_remove_last_component(void);
    // blam-cc: EBX -> &reference->path; files module, 0x555f80
extern void path_split(char **out_slot, char **out_extension, int32_t flags);
    // blam-cc: ESI -> path, EBX -> out slot, EDI -> out slot; files module, 0x556000
extern void path_build(void);
    // blam-cc: EAX -> &reference->path, EDX -> out buffer, CX -> reference+6; files, 0x5560d0
extern int32_t file_reference_compare_by_name(const void *a, const void *b);
    // the qsort comparator at 0x483d20; belongs to files, not hs -- see the misattribution note

extern datum_index global_scenario_index; // 0x0069e8d4
extern tag_instance *tag_instances;       // 0x0087bc14

// Rebuilds the combined HS source from disk: "data\<scenario tag directory>\scripts\*.hsc", plus
// "data\global_scripts.hsc". Returns true when NOTHING was found -- the flag starts at 1 and is
// cleared by global_scripts.hsc existing, and again by every .hsc file in the scripts directory.
char hs_rebuild_source(void)
{
    char path[256];
    char *last_separator;
    file_reference global_scripts;
    file_reference scripts_directory;
    file_reference entries[8];
    char display_name[256];
    char extension[256];
    char *extension_part;
    char *split_slot;
    char nothing_found;
    int16_t entry_count;
    int16_t i;
    char *end;
    size_t used;

    // "data\<scenario tag path>", then replace the tag's own last component with "scripts".
    sprintf(path, "data\\%s", tag_instances[global_scenario_index & 0xffff].path);
    last_separator = strrchr(path, '\\');
    sprintf(last_separator + 1, "scripts");

    // "data\global_scripts.hsc"
    memset(&global_scripts, 0, sizeof(global_scripts));
    *(uint32_t *)&global_scripts.opaque[0] = 0x66696c6f;
    *(uint16_t *)&global_scripts.opaque[6] = 0xffff;
    if ((global_scripts.opaque[4] & 1) != 0) { // dead: the memset above just cleared this byte
        path_remove_last_component(); // EBX = &global_scripts.opaque[8]
    }
    path_append_component(); // ESI = &global_scripts.opaque[8], EBX = "data\\global_scripts.hsc"
    global_scripts.opaque[4] |= 1;
    nothing_found = 1;
    if (file_reference_exists(&global_scripts) != 0) {
        file_reference_exists(&global_scripts); // result discarded -- see the UNSURE note above
        nothing_found = 0;
    }

    // "data\<scenario tag directory>\scripts" as a directory reference, then enumerate it.
    memset(&scripts_directory, 0, sizeof(scripts_directory));
    *(uint32_t *)&scripts_directory.opaque[0] = 0x66696c6f;
    *(uint16_t *)&scripts_directory.opaque[6] = 0xffff;
    path_append_component(); // ESI = &scripts_directory.opaque[8], EBX = path

    entry_count = 0;
    file_enumerate_start(0, &scripts_directory);
    do {
        if (file_enumerate_find_next(&entries[entry_count], 0) == 0) {
            break;
        }
        entry_count = entry_count + 1;
    } while (entry_count < k_hs_maximum_source_files);

    qsort(entries, (size_t)entry_count, k_hs_file_reference_size,
          (int (*)(const void *, const void *))file_reference_compare_by_name);

    for (i = 0; i < entry_count; i = i + 1) {
        memset(display_name, 0, sizeof(display_name));
        path_build(); // EAX = &entries[i].opaque[8], EDX = display_name,
                      // CX = *(uint16 *)&entries[i].opaque[6]
        extension_part = 0;
        split_slot = 0;
        path_split(&split_slot, &extension_part, entries[i].opaque[4] & 1);
            // ESI = display_name, EBX and EDI = two further out slots this function never reads

        extension[0] = '\0';
        if (*extension_part != '\0') {
            // Append ".<ext>" to `extension`. `extension` was just emptied, so the '.' branch
            // never actually runs; the retail code emits it unconditionally anyway.
            end = extension;
            while (*end != '\0') {
                end = end + 1;
            }
            if (end != extension) {
                *end = '.';
                end = end + 1;
                *end = '\0';
            }
            used = strlen(extension);
            strncpy(end, extension_part, (size_t)(0xff - used));
            extension[0xff] = '\0';
        }

        // 4-byte repe cmpsb, so the NUL is included: `extension` must be exactly "hsc"
        if (memcmp(extension, "hsc", 4) == 0) {
            file_reference_exists(&entries[i]); // result discarded -- see the UNSURE note above
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
