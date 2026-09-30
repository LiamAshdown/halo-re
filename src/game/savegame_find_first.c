// savegame_find_first  (Ghidra: savegame_find_first, already named)
// address 0x551bc0, size 356 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/game_types_notes.md ("user_save_path_table comes from
//   user_save_path_register / _lookup / _remove"); this batch's user_save_path_register.c
//   (called here with the fresh FindFirstFileA handle AS the user id, i.e. this table doubles
//   as a handle -> root_path map for the enumerator); the sibling XDeleteSaveGame.c's identical
//   "scan forward from one byte before a scratch string for its NUL, then append a byte"
//   pointer idiom, which is what this rewrite recognizes and collapses into plain strcpy/strcat
//   on a scratch buffer living at caller_find_data + 0x140 (one byte past the real
//   WIN32_FIND_DATAA, whose own real size is 0x140). Ghidra's raw pointer arithmetic (self-
//   referential array indexing such as `pCVar7[(int)in_EAX + (0x140 - (int)pCVar1)]`) algebraically
//   reduces to `*((char*)in_EAX + 0x140 + k)` for the k-th source byte, i.e. a strcpy into that
//   fixed offset; this rewrite is written in that reduced form rather than the literal
//   index expression.
// register convention: the caller-supplied WIN32_FIND_DATAA (plus 0x140-byte trailing scratch)
//   in EAX (in_EAX); `root_path` is this function's own recognized stack parameter.
//   // blam-cc: EAX -> find_data, stack -> root_path
// UNSURE: the algebraic reduction above is derived, not independently confirmed by disassembly;
//   string_convert_ascii_to_unicode's real effect (called once, argument elided, presumably consuming the raw
//   filename this function writes to the scratch buffer before overwriting it with root_path);
//   the trailing scratch buffer's own size/ownership (assumed caller-allocated, at least
//   MAX_PATH*2 bytes, since it ends up holding "root_path\filename\").

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

// win32_find_dataa is types/game.h's (the Win32 WIN32_FIND_DATAA layout, 0x140 bytes).


extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source); // 0x557990, EAX dst, EDI capacity, EBX source

// blam-cc: EAX -> find_data, stack -> root_path
// Starts a directory enumeration under `root_path`, registers the resulting handle against
// `root_path` in the user_save_path table, and skips forward to the first subdirectory entry
// (attribute bit 0x10) that isn't a dotfile, building "root_path\<subdir>\" into the scratch
// area 0x140 bytes past `find_data`. Returns the find handle on success, NULL if the arguments
// are invalid, or (HANDLE)-1 if the search comes up empty or registration fails.
// FIXED: returns the find handle as an integer (callers compare it with -1), and the parameters are ordered as both
// callers pass them (root, find data) -- registers are bound by name: EAX find_data, stack root_path (0x53d769..0x53d775)
int32_t savegame_find_first(char *root_path, win32_find_dataa *find_data)
{
    char pattern[264];
    void *handle;

    if (root_path == 0 || find_data == 0) {
        return 0;
    }

    sprintf(pattern, "%s\\*.*", root_path);
    handle = FindFirstFileA(pattern, (LPWIN32_FIND_DATAA)find_data);
    if (handle == (void *)0xffffffff) {
        return (int32_t)handle;
    }

    {
        uint8_t exhausted = 0;
        int32_t slot = user_save_path_register((uint32_t)handle, root_path);
        if (slot == -1) {
            FindClose(handle);
            return -1;
        }

        while ((find_data->dwFileAttributes & 0x10) != 0) {
            if (find_data->cFileName[0] != '.') {
                char *scratch = (char *)find_data + 0x140;
                int32_t end;

                {
                    int32_t i = 0;
                    do {
                        scratch[i] = find_data->cFileName[i];
                        i = i + 1;
                    } while (find_data->cFileName[i - 1] != '\0');
                }
                // 0x551c9a..0x551ca5: EAX = find_data + 0x244 (a wide-name buffer after the find data), EDI = 0x80,
                // EBX = find_data->cFileName (still set from 0x551c3f)
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

                return (int32_t)handle;
            }
            if (exhausted) {
                break;
            }
            if (FindNextFileA(handle, (LPWIN32_FIND_DATAA)find_data) == 0) {
                exhausted = 1;
            }
        }
    }

    FindClose(handle);
    return -1;
}

#if 0
Original Ghidra decompilation (0x551bc0), from tools/pack.py 0x551bc0:

HANDLE savegame_find_first(char *param_1)

{
  CHAR *pCVar1;
  char *pcVar2;
  char cVar3;
  bool bVar4;
  LPWIN32_FIND_DATAA in_EAX;
  HANDLE hFindFile;
  int iVar5;
  BOOL BVar6;
  CHAR *pCVar7;
  uint uVar8;
  CHAR *pCVar9;
  undefined2 *puVar10;
  CHAR *pCVar11;
  char local_108 [264];

  if ((param_1 == (char *)0x0) || (in_EAX == (LPWIN32_FIND_DATAA)0x0)) {
    return (HANDLE)0x0;
  }
  _sprintf(local_108,"%s\\*.*",param_1);
  hFindFile = FindFirstFileA(local_108,in_EAX);
  if (hFindFile != (HANDLE)0xffffffff) {
    bVar4 = false;
    iVar5 = user_save_path_register(hFindFile,param_1);
    if (iVar5 == -1) {
      FindClose(hFindFile);
      return (HANDLE)0xffffffff;
    }
    if ((in_EAX->dwFileAttributes & 0x10) != 0) {
      pCVar1 = in_EAX->cFileName;
      do {
        if (*pCVar1 != '.') {
          pCVar7 = pCVar1;
          do {
            cVar3 = *pCVar7;
            pCVar7[(int)in_EAX + (0x140 - (int)pCVar1)] = cVar3;
            pCVar7 = pCVar7 + 1;
          } while (cVar3 != '\0');
          FUN_00557990();
          iVar5 = 0x140 - (int)param_1;
          do {
            cVar3 = *param_1;
            param_1[(int)in_EAX + iVar5] = cVar3;
            param_1 = param_1 + 1;
          } while (cVar3 != '\0');
          puVar10 = (undefined2 *)&in_EAX->field_0x13f;
          do {
            pcVar2 = (char *)((int)puVar10 + 1);
            puVar10 = (undefined2 *)((int)puVar10 + 1);
          } while (*pcVar2 != '\0');
          *puVar10 = 0x5c;
          pCVar7 = pCVar1;
          do {
            cVar3 = *pCVar7;
            pCVar7 = pCVar7 + 1;
          } while (cVar3 != '\0');
          pCVar11 = &in_EAX->field_0x13f;
          do {
            pcVar2 = pCVar11 + 1;
            pCVar11 = pCVar11 + 1;
          } while (*pcVar2 != '\0');
          pCVar9 = pCVar1;
          for (uVar8 = (uint)((int)pCVar7 - (int)pCVar1) >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
            *(undefined4 *)pCVar11 = *(undefined4 *)pCVar9;
            pCVar9 = pCVar9 + 4;
            pCVar11 = pCVar11 + 4;
          }
          puVar10 = (undefined2 *)&in_EAX->field_0x13f;
          for (uVar8 = (int)pCVar7 - (int)pCVar1 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
            *pCVar11 = *pCVar9;
            pCVar9 = pCVar9 + 1;
            pCVar11 = pCVar11 + 1;
          }
          do {
            pcVar2 = (char *)((int)puVar10 + 1);
            puVar10 = (undefined2 *)((int)puVar10 + 1);
          } while (*pcVar2 != '\0');
          *puVar10 = 0x5c;
          return hFindFile;
        }
        if (bVar4) break;
        BVar6 = FindNextFileA(hFindFile,in_EAX);
        if (BVar6 == 0) {
          bVar4 = true;
        }
      } while ((in_EAX->dwFileAttributes & 0x10) != 0);
    }
    FindClose(hFindFile);
    hFindFile = (HANDLE)0xffffffff;
  }
  return hFindFile;
}
#endif
