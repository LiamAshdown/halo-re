// console_autocomplete_command  (Ghidra: console_autocomplete_command, already named)
// address 0x4c6bc0, size 697 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: rewritten from the raw disassembly 0x4c6bc0..0x4c6e78 (phase 4 review; Ghidra
//   merges the countdown [esp+0x24] and the line buffer [esp+0x28] into one variable). One
//   caller, console_process_key_events 0x4c65c0 on _input_key_tab. The word being completed
//   starts after the last space, open parenthesis or double quote of
//   console_globals.terminal.input (0x006b70d8); the gather, hs_autocomplete_gather 0x483c90, is
//   called with EAX = that word, ECX = 0x100, EDX = 1 (_console_context_default_bit, a constant
//   here, where console_process_command passes the computed mask), then 0x28 and the name array
//   on the stack (caller pops 8). Strings: 0x00661c78 "profile_load" (hidden from the listing,
//   CEA console_is_token_supar_secret), 0x00669140 "|t" (the terminal tab escape), 0x0065512c ""
//   (a blank line). 0x006b71de is console_globals.terminal.edit.cursor.
// register convention: cdecl, no arguments, no result.
// UNSURE: a single match makes common_index its length (the compare loop runs one past the
//   terminator of a string compared with itself), so the NUL is copied and the cursor lands one
//   past the end of the completed word; and a first-character mismatch leaves common_index -1,
//   which clears the word. Both are what the code does; kept verbatim.

// FIXED (verified against the call site): hs_autocomplete_gather is (category mask 0x28, results) on the stack with
//   the prefix in EAX, 0x100 (the result capacity) in CX and the context mask in DX; the old prototype put the
//   prefix first, so the prefix pointer became the category mask.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "main.h"
#include <string.h>
#include <ctype.h>
#include <stdint.h> // uintptr_t
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern console_globals console_globals_data; // 0x006b7020

extern int standalone_devmode(void); // standalone/loader.c: "-devmode" or HALO_DEVMODE
extern int16_t hs_autocomplete_gather(uint32_t category_mask, char **results, char *prefix, int16_t maximum_count,
    uint16_t gametype_mask); // 0x483c90, blam-cc: EAX -> prefix, CX -> maximum_count, DX -> gametype_mask,
    // stack -> category_mask, results; // 0x483c90, foreign (hs module)
    // blam-cc: EAX -> partial_name, ECX -> mode (0x100, UNSURE), EDX -> context_mask, stack -> max_count, out_names
extern void console_out_printf(uint8_t clear_first, const char *format, ...); // this module, 0x4c6860

// Tab completion for the console input line: gathers every hs name that starts with the word
// under completion, prints them (one per line, or four per line joined with |t tab escapes when
// there are more than 16), and replaces the word with the longest prefix (compared case
// insensitively) the names share, leaving the edit cursor after it. profile_load is never
// listed, and a lone profile_load match is not completed at all.
void console_autocomplete_command(void)
{
    char *input;                      // ebx
    char *word;                       // [esp+0x14]
    char *after_space;
    char *after_paren;
    char *after_quote;
    char **cursor;                    // [esp+0x18]
    int32_t printed_count;            // [esp+0x1c]
    int16_t common_index;             // [esp+0x20] last index shared by every listed name
    uint32_t remaining;               // [esp+0x24]
    uint8_t many_matches;             // [esp+0x13] more than 16 matches: tabulate
    char line[0x400];                 // [esp+0x28]
    char *names[0x100];               // [esp+0x428] (the gather writes at most 0x28)
    char *name;
    int16_t match_count;              // bp
    int16_t limit;                    // ebp, reused once the count is in [esp+0x24]
    int16_t i;                        // si
    uint32_t length;
    int first;

    input = console_globals_data.terminal.input;
    word = input;
    after_space = strrchr(input, ' ') + 1;
    after_paren = strrchr(input, '(') + 1;
    after_quote = strrchr(input, '"') + 1;
    if ((uintptr_t)after_space >= (uintptr_t)word) {
        word = after_space;
    }
    if ((uintptr_t)word <= (uintptr_t)after_paren) {
        word = after_paren;
    }
    if ((uintptr_t)word <= (uintptr_t)after_quote) {
        word = after_quote;
    }

    match_count = hs_autocomplete_gather(0x28, names, word, 0x100,
        standalone_devmode() ? 0 : _console_context_default_bit); // STANDALONE EXTENSION: -devmode completes everything
    if (match_count == 0) {
        return;
    }
    many_matches = match_count > 0x10;
    common_index = 0x7fff;
    if (match_count == 1 && strcmp(names[0], "profile_load") == 0) {
        return;
    }

    line[0] = 0;
    console_out_printf(0, "");
    printed_count = 0;
    if (match_count > 0) {
        remaining = (uint16_t)match_count;
        cursor = names;
        do {
            name = *cursor;
            if (strcmp(name, "profile_load") != 0) {
                length = (uint32_t)strlen(name);
                if ((uint32_t)(int32_t)common_index > length) {
                    limit = (int16_t)length;
                } else {
                    limit = common_index;
                }
                i = 0;
                first = tolower((int)(int8_t)names[0][0]);
                if (tolower((int)(int8_t)name[0]) == first) {
                    do {
                        if (i > limit) {
                            break;
                        }
                        i++;
                        first = tolower((int)(int8_t)names[0][i]);
                    } while (tolower((int)(int8_t)(*cursor)[i]) == first);
                }
                common_index = (int16_t)(i - 1);

                if (many_matches != 0) {
                    strcat(line, *cursor);
                    strcat(line, "|t");                 // 0x00669140
                    if (printed_count % 4 == 3) {
                        console_out_printf(0, line);    // the line is the format string
                        line[0] = 0;
                    }
                } else {
                    console_out_printf(0, *cursor);     // the name is the format string
                }
                printed_count++;
            }
            cursor++;
            remaining--;
        } while (remaining != 0);
    }

    if (many_matches != 0 && (printed_count - 1) % 4 != 3) {
        console_out_printf(0, line);
    }
    if (common_index != 0x7fff) {
        strncpy(word, names[0], (int32_t)common_index + 1);
        word[common_index + 1] = 0;
        console_globals_data.terminal.edit.cursor =
            (int16_t)((int16_t)(word - input) + common_index + 1);
    }
}

#if 0
Original Ghidra decompilation (0x4c6bc0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void console_autocomplete_command(void)

{
  char cVar1;
  undefined2 *puVar2;
  ushort uVar3;
  char *pcVar4;
  char *pcVar5;
  char *pcVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  short sVar10;
  short sVar11;
  char *pcVar12;
  undefined2 *puVar13;
  bool bVar14;
  char *local_814;
  char **local_810;
  uint local_80c;
  undefined1 local_804 [4];
  char local_800 [1024];
  char *local_400 [256];

  local_814 = &DAT_006b70d8;
  pcVar4 = _strrchr(&DAT_006b70d8,0x20);
  pcVar5 = _strrchr(&DAT_006b70d8,0x28);
  pcVar6 = _strrchr(&DAT_006b70d8,0x22);
  if ((char *)0x6b70d7 < pcVar4 + 1) {
    local_814 = pcVar4 + 1;
  }
  if (local_814 <= pcVar5 + 1) {
    local_814 = pcVar5 + 1;
  }
  if (local_814 <= pcVar6 + 1) {
    local_814 = pcVar6 + 1;
  }
  uVar3 = hs_autocomplete_gather(0x28,local_400);
  if (uVar3 != 0) {
    sVar10 = 0x7fff;
    if (uVar3 == 1) {
      iVar8 = 0xd;
      bVar14 = true;
      pcVar4 = local_400[0];
      pcVar5 = "profile_load";
      do {
        if (iVar8 == 0) break;
        iVar8 = iVar8 + -1;
        bVar14 = *pcVar4 == *pcVar5;
        pcVar4 = pcVar4 + 1;
        pcVar5 = pcVar5 + 1;
      } while (bVar14);
      if (bVar14) {
        return;
      }
    }
    local_800[0] = '\0';
    console_out_printf('\0',"");
    local_80c = 0;
    if (0 < (short)uVar3) {
      local_804 = (undefined1  [4])(uint)uVar3;
      local_810 = local_400;
      do {
        pcVar4 = *local_810;
        iVar8 = 0xd;
        bVar14 = true;
        pcVar5 = pcVar4;
        pcVar6 = "profile_load";
        do {
          if (iVar8 == 0) break;
          iVar8 = iVar8 + -1;
          bVar14 = *pcVar5 == *pcVar6;
          pcVar5 = pcVar5 + 1;
          pcVar6 = pcVar6 + 1;
        } while (bVar14);
        if (!bVar14) {
          pcVar5 = pcVar4;
          do {
            cVar1 = *pcVar5;
            pcVar5 = pcVar5 + 1;
          } while (cVar1 != '\0');
          if (pcVar5 + (-1 - (int)(pcVar4 + 1)) < (char *)(int)sVar10) {
            pcVar5 = pcVar4;
            do {
              cVar1 = *pcVar5;
              pcVar5 = pcVar5 + 1;
            } while (cVar1 != '\0');
            sVar10 = ((short)pcVar5 - ((short)pcVar4 + 1)) + -1;
          }
          sVar11 = 0;
          iVar8 = _tolower((int)*local_400[0]);
          iVar7 = _tolower((int)*pcVar4);
          if (iVar7 == iVar8) {
            do {
              if (sVar10 < sVar11) break;
              sVar11 = sVar11 + 1;
              iVar8 = _tolower((int)local_400[0][sVar11]);
              iVar7 = _tolower((int)(*local_810)[sVar11]);
            } while (iVar7 == iVar8);
          }
          sVar10 = sVar11 + -1;
          if (0x10 < (short)uVar3) {
            pcVar4 = *local_810;
            pcVar5 = pcVar4;
            do {
              cVar1 = *pcVar5;
              pcVar5 = pcVar5 + 1;
            } while (cVar1 != '\0');
            pcVar6 = local_804 + 3;
            do {
              pcVar12 = pcVar6 + 1;
              pcVar6 = pcVar6 + 1;
            } while (*pcVar12 != '\0');
            pcVar12 = pcVar4;
            for (uVar9 = (uint)((int)pcVar5 - (int)pcVar4) >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
              *(undefined4 *)pcVar6 = *(undefined4 *)pcVar12;
              pcVar12 = pcVar12 + 4;
              pcVar6 = pcVar6 + 4;
            }
            for (uVar9 = (int)pcVar5 - (int)pcVar4 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
              *pcVar6 = *pcVar12;
              pcVar12 = pcVar12 + 1;
              pcVar6 = pcVar6 + 1;
            }
            puVar2 = (undefined2 *)(local_804 + 3);
            do {
              puVar13 = puVar2;
              puVar2 = (undefined2 *)((int)puVar13 + 1);
            } while (*(char *)((int)puVar13 + 1) != '\0');
            uVar9 = local_80c & 0x80000003;
            *(undefined2 *)((int)puVar13 + 1) = 0x747c;
            *(undefined1 *)((int)puVar13 + 3) = 0;
            if ((int)uVar9 < 0) {
              uVar9 = (uVar9 - 1 | 0xfffffffc) + 1;
            }
            if (uVar9 == 3) {
              console_out_printf('\0',local_800);
              local_800[0] = '\0';
            }
          }
          else {
            console_out_printf('\0',*local_810);
          }
          local_80c = local_80c + 1;
        }
        local_810 = local_810 + 1;
        local_804 = (undefined1  [4])((int)local_804 - 1);
      } while (local_804 != (undefined1  [4])0x0);
      local_804 = (undefined1  [4])0x0;
    }
    if (0x10 < (short)uVar3) {
      uVar9 = local_80c - 1 & 0x80000003;
      if ((int)uVar9 < 0) {
        uVar9 = (uVar9 - 1 | 0xfffffffc) + 1;
      }
      if (uVar9 != 3) {
        console_out_printf('\0',local_800);
      }
    }
    if (sVar10 != 0x7fff) {
      _strncpy(local_814,local_400[0],(int)sVar10 + 1);
      local_814[sVar10 + 1] = '\0';
      _DAT_006b71de = (short)local_814 + -0x70d7 + sVar10;
    }
  }
  return;
}

Disassembly anchors (esp offsets after the four register pushes):
  4c6c13  lea eax,[esp+0x428] ; push eax ; push 0x28 ; mov edx,1 ; mov ecx,0x100 ; mov eax,ebx
  4c6c29  call 0x483c90 ; add esp,8 ; mov ebp,eax      (the count lives in bp)
  4c6c3c  cmp bp,0x10 ; setg [esp+0x13]               many_matches
  4c6ccd  movsx ecx,WORD [esp+0x20] ; cmp ecx,strlen ; jbe  (unsigned: -1 picks strlen)
  4c6d14  cmp si,bp ; jg exit ; inc esi ; tolower(names[0][si]) vs tolower((*cursor)[si])
  4c6d47  dec esi ; mov [esp+0x20],esi                common_index = i - 1
  4c6d98  append word [0x669140] + byte [0x669142]    "|t"
  4c6e61  sub bx,dx ; lea eax,[ebx+edi+1] ; mov [0x6b71de],ax
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
