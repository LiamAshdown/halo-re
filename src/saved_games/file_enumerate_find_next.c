// file_enumerate_find_next  (Ghidra: file_enumerate_find_next, already named)
// address 0x555c10, size 304 bytes
// name confidence: 0.8   rewrite confidence: 0.5
// evidence: already named by Ghidra/CEA. out/phase4/saved_games_types_notes.md
// file_reference_record: "0x555b90, 0x555c10 (location +0x06 copied to the enumeration state,
// path +0x08)". Confirmed against objdump 0x555c10..0x555ea6: both parameters are plain stack
// arguments (out_entry, out_write_time), matching Ghidra's fully-resolved signature; the depth
// index (DAT_0069fa5c low word) is read into EDX before the prologue's stack probe and carried
// in DX/local storage throughout, exactly as file_enumerate_start left it.
// register convention: plain stack arguments (out_entry, an optional file_reference_record
// receiving the found entry; out_write_time, an optional uint32_t[2] receiving its FILETIME).
// UNSURE: the directory-recursion bookkeeping (depth increment/decrement, the two globals at
// 0x00671f98/0x00671f9c and 0x006600b0/0x0069fbcd that Ghidra's export lists as referenced but
// that never appear in the decompiled body shown here) is transliterated as literally as
// possible; this function was not independently re-derived from objdump beyond the parameter
// convention above.

#include "crt.h"
#include "win32.h"
#include <string.h>
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
extern win32_find_dataa file_enumeration_find_data; // 0x0069fba0

extern void path_build_full(char *source, char *destination, int16_t location); // 0x5560d0, this module
extern void path_append_component(char *destination, const char *component); // 0x555ec0, this module
extern void path_remove_last_component(char *path); // 0x555f80, this module


// blam-cc: plain stack arguments (out_entry, out_write_time)
// Advances the (possibly recursive) enumeration started by file_enumerate_start and returns the
// next matching entry through out_entry (built as a full file_reference_record naming it) and,
// if out_write_time is non-null, its FILETIME. Recurses into subdirectories when the recursive
// flag is set, skips "." and "..", and honors the directories-only flag. Returns 1 while there
// is a match, 0 once the (possibly nested) enumeration is exhausted.
uint8_t file_enumerate_find_next(file_reference_record *out_entry, uint32_t *out_write_time)
{
    char search_path[0x100];
    int32_t depth;
    char *end;
    char *dest;
    uint32_t remaining;
    void *handle;
    int32_t found;
    int16_t location;
    int32_t is_dot;

    depth = file_enumeration_pos.depth;
    if (file_enumeration_pos.depth < 0) {
        return 0;
    }

    for (;;) {
        if (file_enumeration_handles[depth] == (void *)-1) {
            path_build_full(file_enumeration_path, search_path, file_enumeration_pos.location);
            end = search_path;
            while (*end != '\0') {
                end++;
            }
            if (end != search_path) {
                *end = '\\';
                end++;
                *end = '\0';
            }
            dest = search_path;
            while (*dest != '\0') {
                dest++;
            }
            remaining = (uint32_t)(0xff - ((int32_t)dest - (int32_t)search_path));
            strncpy(dest, "*.*", remaining);
            search_path[0xff] = '\0';
            handle = FindFirstFileA(search_path, (LPWIN32_FIND_DATAA)&file_enumeration_find_data);
            file_enumeration_handles[depth] = handle;
            if (handle == (void *)-1) {
                goto pop_level;
            }
        } else {
            found = FindNextFileA(file_enumeration_handles[depth], (LPWIN32_FIND_DATAA)&file_enumeration_find_data);
            if (found != 0) {
                goto have_entry;
            }
            FindClose(file_enumeration_handles[depth]);
            file_enumeration_handles[depth] = (void *)-1;
pop_level:
            path_remove_last_component(file_enumeration_path);
            depth--;
            if (depth < 0) {
                file_enumeration_pos.depth = (int16_t)depth;
                return 0;
            }
            continue;
        }

have_entry:
        location = file_enumeration_pos.location;
        if ((file_enumeration_find_data.dwFileAttributes & 0x10) == 0) {
            // a plain file
            if ((file_enumeration_flags_value & 2) == 0) {
                memset(out_entry, 0, sizeof(*out_entry));
                out_entry->signature = k_file_reference_signature;
                out_entry->location = location;
                path_append_component(out_entry->path, file_enumeration_path);
                if ((out_entry->flags & _file_reference_is_file_bit) != 0) {
                    path_remove_last_component(out_entry->path);
                }
                path_append_component(out_entry->path, file_enumeration_find_data.cFileName);
                out_entry->flags = out_entry->flags | _file_reference_is_file_bit;
                if (out_write_time != 0) {
                    out_write_time[0] = file_enumeration_find_data.ftLastWriteTime[0];
                    out_write_time[1] = file_enumeration_find_data.ftLastWriteTime[1];
                }
                file_enumeration_pos.depth = (int16_t)depth;
                return 1;
            }
        } else {
            // a directory: skip "." and ".."
            is_dot = (file_enumeration_find_data.cFileName[0] == '.' &&
                      file_enumeration_find_data.cFileName[1] == '\0');
            if (!is_dot) {
                is_dot = (file_enumeration_find_data.cFileName[0] == '.' &&
                          file_enumeration_find_data.cFileName[1] == '.' &&
                          file_enumeration_find_data.cFileName[2] == '\0');
            }
            if (!is_dot) {
                if ((file_enumeration_flags_value & 2) != 0) {
                    memset(out_entry, 0, sizeof(*out_entry));
                    out_entry->signature = k_file_reference_signature;
                    out_entry->location = location;
                    path_append_component(out_entry->path, file_enumeration_path);
                    path_append_component(out_entry->path, file_enumeration_find_data.cFileName);
                }
                if ((file_enumeration_flags_value & 1) != 0) {
                    if ((file_enumeration_flags_value & 2) == 0) {
                        path_append_component(file_enumeration_path, file_enumeration_find_data.cFileName);
                    }
                    depth++;
                }
                if ((file_enumeration_flags_value & 2) != 0) {
                    if (out_write_time != 0) {
                        out_write_time[0] = file_enumeration_find_data.ftLastWriteTime[0];
                        out_write_time[1] = file_enumeration_find_data.ftLastWriteTime[1];
                    }
                    file_enumeration_pos.depth = (int16_t)depth;
                    return 1;
                }
            }
        }
        if (depth < 0) {
            file_enumeration_pos.depth = (int16_t)depth;
            return 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x555c10):

undefined1 file_enumerate_find_next(undefined4 *param_1,undefined4 *param_2)

{
  char cVar1;
  undefined2 uVar2;
  char *pcVar3;
  HANDLE pvVar4;
  BOOL BVar5;
  int iVar6;
  char *pcVar7;
  undefined4 *puVar8;
  bool bVar9;
  int local_108;
  char local_100;
  undefined4 local_ff;
  undefined1 local_1;

  local_100 = '\0';
  puVar8 = &local_ff;
  for (iVar6 = 0x3f; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  *(undefined2 *)puVar8 = 0;
  *(undefined1 *)((int)puVar8 + 2) = 0;
  local_108 = DAT_0069fa5c;
  if ((short)DAT_0069fa5c < 0) {
    return 0;
  }
  do {
    iVar6 = (int)(short)local_108;
    if (*(HANDLE *)(&DAT_0069fb60 + iVar6 * 4) == (HANDLE)0xffffffff) {
      FUN_005560d0();
      pcVar3 = &local_100;
      do {
        pcVar7 = pcVar3;
        pcVar3 = pcVar7 + 1;
      } while (*pcVar7 != '\0');
      if (pcVar7 != &local_100) {
        *pcVar7 = '\\';
        *pcVar3 = '\0';
        pcVar7 = pcVar3;
      }
      pcVar3 = &local_100;
      do {
        cVar1 = *pcVar3;
        pcVar3 = pcVar3 + 1;
      } while (cVar1 != '\0');
      _strncpy(pcVar7,"*.*",0xff - ((int)pcVar3 - (int)&local_ff));
      local_1 = 0;
      pvVar4 = FindFirstFileA(&local_100,(LPWIN32_FIND_DATAA)&DAT_0069fba0);
      *(HANDLE *)(&DAT_0069fb60 + iVar6 * 4) = pvVar4;
      if (pvVar4 == (HANDLE)0xffffffff) goto LAB_00555df1;
LAB_00555cea:
      uVar2 = DAT_0069fa5c._2_2_;
      if ((DAT_0069fba0 & 0x10) == 0) {
        if ((DAT_0069fa58 & 2) == 0) {
          puVar8 = param_1;
          for (iVar6 = 0x43; iVar6 != 0; iVar6 = iVar6 + -1) {
            *puVar8 = 0;
            puVar8 = puVar8 + 1;
          }
          *param_1 = 0x66696c6f;
          *(undefined2 *)((int)param_1 + 6) = uVar2;
          path_append_component();
          if ((*(byte *)(param_1 + 1) & 1) != 0) {
            path_remove_last_component();
          }
          path_append_component();
          *(byte *)(param_1 + 1) = *(byte *)(param_1 + 1) | 1;
          if (param_2 != (undefined4 *)0x0) {
            *param_2 = DAT_0069fbb4;
            param_2[1] = DAT_0069fbb8;
          }
          DAT_0069fa5c = CONCAT22(DAT_0069fa5c._2_2_,(short)local_108);
          return 1;
        }
      }
      else {
        iVar6 = 2;
        bVar9 = true;
        pcVar3 = &DAT_0069fbcc;
        pcVar7 = ".";
        do {
          if (iVar6 == 0) break;
          iVar6 = iVar6 + -1;
          bVar9 = *pcVar3 == *pcVar7;
          pcVar3 = pcVar3 + 1;
          pcVar7 = pcVar7 + 1;
        } while (bVar9);
        if (!bVar9) {
          iVar6 = 3;
          bVar9 = true;
          pcVar3 = &DAT_0069fbcc;
          pcVar7 = "..";
          do {
            if (iVar6 == 0) break;
            iVar6 = iVar6 + -1;
            bVar9 = *pcVar3 == *pcVar7;
            pcVar3 = pcVar3 + 1;
            pcVar7 = pcVar7 + 1;
          } while (bVar9);
          if (!bVar9) {
            if ((DAT_0069fa58 & 2) != 0) {
              puVar8 = param_1;
              for (iVar6 = 0x43; iVar6 != 0; iVar6 = iVar6 + -1) {
                *puVar8 = 0;
                puVar8 = puVar8 + 1;
              }
              *param_1 = 0x66696c6f;
              *(undefined2 *)((int)param_1 + 6) = uVar2;
              path_append_component();
              path_append_component();
            }
            if ((DAT_0069fa58 & 1) != 0) {
              if ((DAT_0069fa58 & 2) == 0) {
                path_append_component();
              }
              local_108 = local_108 + 1;
            }
            if ((DAT_0069fa58 & 2) != 0) {
              if (param_2 != (undefined4 *)0x0) {
                *param_2 = DAT_0069fbb4;
                param_2[1] = DAT_0069fbb8;
              }
              DAT_0069fa5c = CONCAT22(DAT_0069fa5c._2_2_,(undefined2)local_108);
              return 1;
            }
          }
        }
      }
    }
    else {
      BVar5 = FindNextFileA(*(HANDLE *)(&DAT_0069fb60 + iVar6 * 4),(LPWIN32_FIND_DATAA)&DAT_0069fba0
                           );
      if (BVar5 != 0) goto LAB_00555cea;
      FindClose(*(HANDLE *)(&DAT_0069fb60 + iVar6 * 4));
      *(undefined4 *)(&DAT_0069fb60 + iVar6 * 4) = 0xffffffff;
LAB_00555df1:
      path_remove_last_component();
      local_108 = local_108 + -1;
    }
    if ((short)local_108 < 0) {
      DAT_0069fa5c = CONCAT22(DAT_0069fa5c._2_2_,(short)local_108);
      return 0;
    }
  } while( true );
}
#endif
