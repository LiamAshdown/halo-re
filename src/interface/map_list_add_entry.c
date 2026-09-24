// map_list_add_entry  (Ghidra: chimera__load_multiplayer_maps, renamed per types/interface.h)
// address 0x4950c0, size 415 bytes (0x4950c0..0x49525e; Ghidra stopped at 208 bytes and printed the
//   tail as the bogus "functions" 0x495190 first_person_weapons_update and 0x4951f0
//   first_person_weapon_render_update -- both are part of this function, covered below; orphan
//   pass 4 re-checked the tail against objdump 0x495160..0x49525f and fixed the file name passed
//   to cache_file_exists, which is strrchr(path, '\\') + 1, or the whole path when there is none)
// name confidence: 0.55   rewrite confidence: 0.4
// evidence: types/interface.h map_list_entry struct comment lists this address as
// map_list_add_entry, and documents the growth-by-0x13 GlobalReAlloc sizing this function
// performs; out/phase4/interface_functions.md "Appends one map-file entry (path, id,
// cache-exists flag) to the growable multiplayer map list, allocating/reallocating storage as
// needed."; src/cache/cache_file_exists.c's confirmed (name in EAX, header_out in ESI) contract.
// register convention: map path in EAX (in_EAX), map id as the recognized stack parameter.
// blam-cc: EAX -> path, stack -> map_id
// UNSURE: the substring search for ".map" (FUN_00625430 against DAT_00669928) infers the needle
// from types/interface.h's "extension stripped" note; the literal bytes at 0x00669928 were not
// independently read.
// UNSURE: cache_file_exists needs a cache_file_header* in ESI that this call site never
// materializes (Ghidra drops it entirely); a local scratch header is supplied here since the
// caller must own that buffer and this function has no other candidate storage for it.
// UNSURE: several redundant re-computations the original performs (a second strlen-based
// re-termination of the path that always lands on the byte already written, and a stale iVar8
// carried from the first strlen when no extension is found) are true no-ops and are not
// reproduced; see the #if 0 block for the literal sequence.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#include <ctype.h>

extern map_list_entry *map_list;     // 0x00712dcc
extern int32_t map_list_count;       // 0x00712dd0
extern int32_t map_list_capacity;    // 0x00712dd4

extern void *GlobalAlloc(uint32_t flags, uint32_t bytes);
extern void *GlobalReAlloc(void *mem, uint32_t bytes, uint32_t flags);
extern void *GlobalFree(void *mem);
extern char *_strstr(char *haystack, const char *needle);
extern char *_strrchr(const char *s, int32_t c);
extern uint8_t cache_file_exists(char *name, cache_file_header *header_out); // 0x442bb0

// blam-cc: EAX -> path, stack -> map_id
// Appends one entry to the growable map_list: grows the array by 0x13 entries first if it is
// full, copies `path` into a fresh GlobalAlloc buffer, strips a trailing ".map" extension if
// present, lowercases the result, and records whether the corresponding cache file (looked up by
// the filename after the last backslash) exists.
void map_list_add_entry(char *path, int32_t map_id)
{
    map_list_entry *entry;
    uint32_t path_length;
    char *extension;
    char *filename;
    char *cursor;
    cache_file_header header;

    if (map_list_capacity <= map_list_count) {
        map_list_capacity = map_list_capacity + 0x13;
        if (map_list == (map_list_entry *)0) {
            map_list = (map_list_entry *)GlobalAlloc(0, map_list_capacity * sizeof(map_list_entry));
        } else if (map_list_capacity * sizeof(map_list_entry) == 0) {
            GlobalFree(map_list);
            map_list = (map_list_entry *)0;
        } else {
            map_list = (map_list_entry *)GlobalReAlloc(
                map_list, map_list_capacity * sizeof(map_list_entry), 2);
        }
    }

    entry = &map_list[map_list_count];
    entry->path = (char *)0;
    entry->map_id = map_id;
    entry->cache_file_exists = 0;

    path_length = strlen(path);
    entry->path = (char *)GlobalAlloc(0, path_length + 1);
    strcpy(entry->path, path);

    extension = _strstr(entry->path, ".map"); // UNSURE: needle inferred, see header
    if (extension != (char *)0) {
        *extension = '\0';
    }

    for (cursor = entry->path; *cursor != '\0'; cursor++) {
        *cursor = (char)tolower((uint8_t)*cursor);
    }

    // 0x495220..0x49522f: the name after the last backslash, or the whole path when there is none
    filename = _strrchr(entry->path, '\\');
    filename = (filename != (char *)0) ? filename + 1 : entry->path;
    entry->cache_file_exists = cache_file_exists(filename, &header);
    map_list_count = map_list_count + 1;
}

#if 0
Original Ghidra decompilation (0x4950c0):

void chimera__load_multiplayer_maps(undefined4 param_1)

{
  byte *pbVar1;
  undefined4 *puVar2;
  char cVar3;
  byte bVar4;
  undefined1 uVar5;
  char *in_EAX;
  SIZE_T SVar6;
  char *pcVar7;
  int iVar8;
  HGLOBAL pvVar9;
  int iVar10;
  undefined1 *puVar11;
  char *pcVar12;
  byte *pbVar13;

  if (DAT_00712dd4 <= DAT_00712dd0) {
    DAT_00712dd4 = DAT_00712dd4 + 0x13;
    SVar6 = DAT_00712dd4 * 0xc;
    if (DAT_00712dcc == (HGLOBAL)0x0) {
      DAT_00712dcc = GlobalAlloc(0,SVar6);
    }
    else if (SVar6 == 0) {
      GlobalFree(DAT_00712dcc);
      DAT_00712dcc = (HGLOBAL)0x0;
    }
    else {
      DAT_00712dcc = GlobalReAlloc(DAT_00712dcc,SVar6,2);
    }
  }
  puVar2 = (undefined4 *)((int)DAT_00712dcc + DAT_00712dd0 * 0xc);
  *puVar2 = 0;
  puVar2[1] = param_1;
  *(undefined1 *)(puVar2 + 2) = 0;
  pcVar7 = in_EAX;
  do {
    cVar3 = *pcVar7;
    pcVar7 = pcVar7 + 1;
  } while (cVar3 != '\0');
  pvVar9 = (HGLOBAL)*puVar2;
  iVar8 = (int)pcVar7 - (int)(in_EAX + 1);
  SVar6 = iVar8 + 1;
  if (pvVar9 == (HGLOBAL)0x0) {
    pvVar9 = GlobalAlloc(0,SVar6);
  }
  else if (SVar6 == 0) {
    GlobalFree(pvVar9);
    pvVar9 = (HGLOBAL)0x0;
  }
  else {
    pvVar9 = GlobalReAlloc(pvVar9,SVar6,2);
  }
  puVar2 = (undefined4 *)((int)DAT_00712dcc + DAT_00712dd0 * 0xc);
  *puVar2 = pvVar9;
  iVar10 = (int)pvVar9 - (int)in_EAX;
  do {
    cVar3 = *in_EAX;
    in_EAX[iVar10] = cVar3;
    in_EAX = in_EAX + 1;
  } while (cVar3 != '\0');
  puVar11 = (undefined1 *)FUN_00625430(*puVar2,&DAT_00669928);
  iVar10 = DAT_00712dd0;
  pvVar9 = DAT_00712dcc;
  if (puVar11 != (undefined1 *)0x0) {
    *puVar11 = 0;
    pcVar12 = *(char **)((int)pvVar9 + iVar10 * 0xc);
    pcVar7 = pcVar12 + 1;
    do {
      cVar3 = *pcVar12;
      pcVar12 = pcVar12 + 1;
    } while (cVar3 != '\0');
    iVar8 = (int)pcVar12 - (int)pcVar7;
  }
  *(undefined1 *)(iVar8 + *(int *)((int)pvVar9 + iVar10 * 0xc)) = 0;
  pbVar13 = *(byte **)((int)pvVar9 + iVar10 * 0xc);
  bVar4 = *pbVar13;
  while (bVar4 != 0) {
    iVar8 = _tolower((uint)*pbVar13);
    *pbVar13 = (byte)iVar8;
    pbVar1 = pbVar13 + 1;
    pbVar13 = pbVar13 + 1;
    pvVar9 = DAT_00712dcc;
    iVar10 = DAT_00712dd0;
    bVar4 = *pbVar1;
  }
  _strrchr(*(char **)((int)pvVar9 + iVar10 * 0xc),0x5c);
  uVar5 = cache_file_exists();
  *(undefined1 *)((int)DAT_00712dcc + DAT_00712dd0 * 0xc + 8) = uVar5;
  DAT_00712dd0 = DAT_00712dd0 + 1;
  return;
}
#endif
