// game_engine_variant_add_to_history  (Ghidra: game_engine_variant_add_to_history, already named)
// address 0x463980, size 397 bytes
// name confidence: 0.5   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Adds a named custom game variant (with either
// supplied or default option data) to the dynamically-grown recent/custom variant cache");
// types/game.h game_variant_history_entry (path +0x00, name +0x04, options +0x0c -- the header
// itself flags path/name as "UNSURE which of the two is which", not resolved further here),
// game_variant_history/_count/_capacity (0x00687b0c/0x10/0x14); this batch's
// game_engine_get_variant_by_name (0x4622d0) and game_engine_is_map_and_variant_valid (0x463920).
// register convention: an optional path/source string in EAX (in_EAX); param_1 (the variant
// name) and param_2 (an optional pre-built game_variant*, NULL to look up defaults by name) are
// this function's own stack parameters.
//   // blam-cc: EAX -> path (optional), stack -> name, options
// UNSURE: game_engine_is_map_and_variant_valid is called here with zero visible arguments; its
// real (map_path, variant_name) inputs could not be recovered from this decompilation and are
// passed as NULL. wcslen and string_convert_unicode_to_ascii (the "no path given" branch's length/format
// helpers) are outside this batch's evidence and are modeled minimally.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_variant_history_entry *game_variant_history; // 0x00687b0c
extern uint32_t game_variant_history_count;              // 0x00687b10
extern uint32_t game_variant_history_capacity;            // 0x00687b14


extern uint32_t game_engine_is_map_and_variant_valid(const char *map_path, const char *variant_name); // 0x463920, this batch
extern uint8_t game_engine_get_variant_by_name(const char *name, game_variant *out);
    // 0x4622d0, this module; blam-cc: ECX -> name, stack -> out. A NULL `out` only tests
    // whether the name is recognized.
extern char *string_convert_unicode_to_ascii(uint32_t size); // 0x557950, not in this batch; UNSURE

// blam-cc: EAX -> path (optional), stack -> name, options
uint32_t game_engine_variant_add_to_history(char *name, game_variant *options, char *path)
{
    game_variant temp;
    void *name_copy;

    if (game_engine_is_map_and_variant_valid(0, 0) == 0) { // UNSURE: real args elided by Ghidra
        return 0;
    }

    if (path == 0) {
        int32_t length = (int32_t)wcslen((const wchar_t *)options); // 0x625b7a == wcslen;
        // the variant's UTF-16 name is its first field. NOTE: the byte count below is
        // `length + 1`, i.e. the binary sizes a narrow buffer from a wide length.
        name_copy = GlobalAlloc(0, (uint32_t)length + 1);
        path = string_convert_unicode_to_ascii((uint32_t)length + 1); // UNSURE: real relationship between this and name_copy
        ((char *)name_copy)[length] = 0;
    } else {
        size_t length = strlen(path);
        name_copy = GlobalAlloc(0, (uint32_t)(length + 1));
        memcpy(name_copy, path, length + 1);
    }

    if (options == 0) {
        if (game_engine_get_variant_by_name(name, &temp) == 0) {
            GlobalFree(name_copy);
            return 0;
        }
    } else {
        memcpy(&temp, options, sizeof(game_variant));
    }

    if (name_copy != 0) {
        game_variant_history_entry *entry;

        if (game_variant_history_capacity <= game_variant_history_count) {
            uint32_t bytes;
            game_variant_history_capacity = game_variant_history_capacity + 4;
            bytes = game_variant_history_capacity * sizeof(game_variant_history_entry);
            if (game_variant_history == 0) {
                game_variant_history = (game_variant_history_entry *)GlobalAlloc(0, bytes);
            } else if (bytes == 0) {
                GlobalFree(game_variant_history);
                game_variant_history = 0;
            } else {
                game_variant_history = (game_variant_history_entry *)GlobalReAlloc(game_variant_history, bytes, 2);
            }
        }

        entry = &game_variant_history[game_variant_history_count];
        game_variant_history_count = game_variant_history_count + 1;

        entry->options = temp;
        entry->name = (char *)name_copy;

        {
            size_t name_length = strlen(name);
            char *path_copy = (char *)GlobalAlloc(0, (uint32_t)(name_length + 1));
            entry->path = path_copy;
            memcpy(path_copy, name, name_length + 1);
        }
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x463980), from tools/pack.py 0x463980:

uint game_engine_variant_add_to_history(char *param_1,undefined4 *param_2)

{
  char cVar1;
  char *in_EAX;
  uint uVar2;
  char *pcVar3;
  HGLOBAL hMem;
  SIZE_T dwBytes;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  undefined4 local_98 [38];

  uVar2 = FUN_00463920();
  if ((char)uVar2 == '\0') {
    return uVar2 & 0xffffff00;
  }
  if (in_EAX == (char *)0x0) {
    iVar4 = FUN_00625b7a(param_2);
    hMem = GlobalAlloc(0,iVar4 + 1U);
    in_EAX = (char *)FUN_00557950(iVar4 + 1U);
    *(undefined1 *)(iVar4 + (int)hMem) = 0;
  }
  else {
    pcVar3 = in_EAX;
    do {
      cVar1 = *pcVar3;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
    hMem = GlobalAlloc(0,(SIZE_T)(pcVar3 + (1 - (int)(in_EAX + 1))));
    iVar4 = (int)hMem - (int)in_EAX;
    do {
      cVar1 = *in_EAX;
      in_EAX[iVar4] = cVar1;
      in_EAX = in_EAX + 1;
    } while (cVar1 != '\0');
  }
  if (param_2 == (undefined4 *)0x0) {
    in_EAX = (char *)game_engine_get_variant_by_name(local_98);
    if ((char)in_EAX == '\0') {
      in_EAX = GlobalFree(hMem);
      goto LAB_00463a74;
    }
  }
  else {
    puVar6 = local_98;
    for (iVar4 = 0x26; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = *param_2;
      param_2 = param_2 + 1;
      puVar6 = puVar6 + 1;
    }
  }
  if (hMem != (HGLOBAL)0x0) {
    if (DAT_00687b14 <= DAT_00687b10) {
      DAT_00687b14 = DAT_00687b14 + 4;
      dwBytes = DAT_00687b14 * 0xa4;
      if (DAT_00687b0c == (HGLOBAL)0x0) {
        DAT_00687b0c = GlobalAlloc(0,dwBytes);
      }
      else if (dwBytes == 0) {
        GlobalFree(DAT_00687b0c);
        DAT_00687b0c = (HGLOBAL)0x0;
      }
      else {
        DAT_00687b0c = GlobalReAlloc(DAT_00687b0c,dwBytes,2);
      }
    }
    puVar5 = (undefined4 *)(DAT_00687b10 * 0xa4 + (int)DAT_00687b0c);
    DAT_00687b10 = DAT_00687b10 + 1;
    puVar6 = local_98;
    puVar7 = puVar5 + 3;
    for (iVar4 = 0x26; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar7 = puVar7 + 1;
    }
    puVar5[1] = hMem;
    pcVar3 = param_1;
    do {
      cVar1 = *pcVar3;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
    pcVar3 = GlobalAlloc(0,(SIZE_T)(pcVar3 + (1 - (int)(param_1 + 1))));
    *puVar5 = pcVar3;
    do {
      cVar1 = *param_1;
      param_1 = param_1 + 1;
      *pcVar3 = cVar1;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
    return CONCAT31((int3)((uint)pcVar3 >> 8),1);
  }
LAB_00463a74:
  return (uint)in_EAX & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
