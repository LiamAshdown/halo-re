// savegame_find_next  (Ghidra: savegame_find_next, already named)
// address 0x551d30, size 251 bytes
// name confidence: 0.55   rewrite confidence: 0.3
// evidence: shares the "scratch buffer at find_data + 0x140" reconstruction with the sibling
//   savegame_find_first.c (this batch); user_save_path_lookup (this batch, 0x5516a0) is keyed by
//   the same find-handle-as-user-id convention savegame_find_first establishes. Ghidra's own
//   `hFindFile` local is read by FindNextFileA without ever being assigned anywhere in this
//   function's decompilation; the only value that could plausibly reach it is `handle` itself
//   (the same value passed to user_save_path_lookup), so this rewrite uses `handle` directly for
//   that call, flagged UNSURE.
// register convention: the caller-supplied WIN32_FIND_DATAA (plus scratch, see
//   savegame_find_first.c) in EAX (in_EAX), the find handle in ECX (in_ECX).
//   // blam-cc: EAX -> find_data, ECX -> handle
// UNSURE: `hFindFile` (see above); string_convert_ascii_to_unicode's real effect (elided argument, same as
//   savegame_find_first.c).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// win32_find_dataa is types/game.h's (the Win32 WIN32_FIND_DATAA layout, 0x140 bytes).

extern char *user_save_path_default; // 0x00721f28
extern char *user_save_path_lookup(uint32_t user_id); // this batch, 0x5516a0
extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source); // 0x557990, EAX dst, EDI capacity, EBX source

// blam-cc: EAX -> find_data, ECX -> handle
// If `handle` is registered (its root_path is not the default sentinel) and FindNextFileA
// yields another entry, returns 1; if that entry is a non-dotfile subdirectory, also rebuilds
// "root_path\<subdir>\" in the scratch area past `find_data`, exactly like savegame_find_first.
// Returns 0 if `handle`/`find_data` are invalid, the handle isn't registered, or the
// enumeration is exhausted.
uint32_t savegame_find_next(win32_find_dataa *find_data, uint32_t handle)
{
    uint32_t result = 0;

    if (handle != 0 && find_data != 0) {
        char *root_path = user_save_path_lookup(handle);
        if (root_path != user_save_path_default &&
            FindNextFileA((void *)handle, (LPWIN32_FIND_DATAA)find_data) != 0) { // UNSURE: see header note on `handle`
            result = 1;
            if (find_data->cFileName[0] != '.' && (find_data->dwFileAttributes & 0x10) != 0) {
                char *scratch = (char *)find_data + 0x140;
                int32_t end;

                {
                    int32_t i = 0;
                    do {
                        scratch[i] = find_data->cFileName[i];
                        i = i + 1;
                    } while (find_data->cFileName[i - 1] != '\0');
                }
                // 0x551daa..0x551db5: as in savegame_find_first -- the name into the wide buffer at find_data + 0x244
                string_convert_ascii_to_unicode((uint16_t *)((uint8_t *)find_data + 0x244), 0x80, find_data->cFileName);

                {
                    int32_t i = 0;
                    do {
                        scratch[i] = root_path[i];
                        i = i + 1;
                    } while (root_path[i - 1] != '\0');
                }
                end = 0;
                while (scratch[end] != '\0') {
                    end = end + 1;
                }
                scratch[end] = '\\';
                end = end + 1;
                {
                    int32_t i = 0;
                    do {
                        scratch[end + i] = find_data->cFileName[i];
                        i = i + 1;
                    } while (find_data->cFileName[i - 1] != '\0');
                    end = end + i - 1;
                }
                scratch[end] = '\\';
                scratch[end + 1] = '\0';
            }
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x551d30), from tools/pack.py 0x551d30:

undefined4 savegame_find_next(void)

{
  CHAR *pCVar1;
  char cVar2;
  int iVar3;
  LPWIN32_FIND_DATAA in_EAX;
  char *pcVar4;
  BOOL BVar5;
  CHAR *pCVar6;
  int in_ECX;
  HANDLE hFindFile;
  uint uVar7;
  CHAR *pCVar8;
  undefined2 *puVar9;
  CHAR *pCVar10;
  undefined4 local_4;

  local_4 = 0;
  if ((in_ECX != 0) && (in_EAX != (LPWIN32_FIND_DATAA)0x0)) {
    pcVar4 = (char *)user_save_path_lookup();
    if ((pcVar4 != DAT_00721f28) && (BVar5 = FindNextFileA(hFindFile,in_EAX), BVar5 != 0)) {
      pCVar1 = in_EAX->cFileName;
      local_4 = 1;
      if ((in_EAX->cFileName[0] != '.') && ((in_EAX->dwFileAttributes & 0x10) != 0)) {
        pCVar6 = pCVar1;
        do {
          cVar2 = *pCVar6;
          pCVar6[(int)in_EAX + (0x140 - (int)pCVar1)] = cVar2;
          pCVar6 = pCVar6 + 1;
        } while (cVar2 != '\0');
        FUN_00557990();
        iVar3 = 0x140 - (int)pcVar4;
        do {
          cVar2 = *pcVar4;
          pcVar4[(int)in_EAX + iVar3] = cVar2;
          pcVar4 = pcVar4 + 1;
        } while (cVar2 != '\0');
        puVar9 = (undefined2 *)&in_EAX->field_0x13f;
        do {
          pcVar4 = (char *)((int)puVar9 + 1);
          puVar9 = (undefined2 *)((int)puVar9 + 1);
        } while (*pcVar4 != '\0');
        *puVar9 = 0x5c;
        pCVar6 = pCVar1;
        do {
          cVar2 = *pCVar6;
          pCVar6 = pCVar6 + 1;
        } while (cVar2 != '\0');
        pCVar10 = &in_EAX->field_0x13f;
        do {
          pcVar4 = pCVar10 + 1;
          pCVar10 = pCVar10 + 1;
        } while (*pcVar4 != '\0');
        pCVar8 = pCVar1;
        for (uVar7 = (uint)((int)pCVar6 - (int)pCVar1) >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
          *(undefined4 *)pCVar10 = *(undefined4 *)pCVar8;
          pCVar8 = pCVar8 + 4;
          pCVar10 = pCVar10 + 4;
        }
        puVar9 = (undefined2 *)&in_EAX->field_0x13f;
        for (uVar7 = (int)pCVar6 - (int)pCVar1 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
          *pCVar10 = *pCVar8;
          pCVar8 = pCVar8 + 1;
          pCVar10 = pCVar10 + 1;
        }
        do {
          pcVar4 = (char *)((int)puVar9 + 1);
          puVar9 = (undefined2 *)((int)puVar9 + 1);
        } while (*pcVar4 != '\0');
        *puVar9 = 0x5c;
      }
    }
    return local_4;
  }
  return 0;
}
#endif
