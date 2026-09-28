// console_process_command  (Ghidra: console_process_command, already named)
// address 0x4c6a80, size 320 bytes
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: types/main.h console_globals (history ring); types/hs.h hs_preserve_token_case
// (0x007102fd); src/networking/network_banlist_load.c's strchr(strchr-shaped) and
// src/hs/*.c's hs_compile_and_evaluate. Disassembly (objdump -d -M intel, bin/halo.exe,
// 0x4c6a80..0x4c6bbf) resolves every register Ghidra otherwise drops: EDI is the command line
// (matching main_types_notes.md); the context_flags stack argument is forwarded unchanged to
// console_command_context_mask; and hs_autocomplete_gather (0x483c90, foreign/hs module) is
// called with EAX -> the parsed command word, ECX = 0x100, EDX -> the context mask, then two
// stack arguments (max_count, out_names) -- main_types_notes.md's "EDX at 0x4c6b4c" pins the
// mask register exactly.
// register convention: EDI -> command_line, stack -> context_flags. // blam-cc: EDI -> command_line
// UNSURE: hs_autocomplete_gather's ECX = 0x100 has no established meaning (a mode or capacity
// constant); passed through verbatim.

// FIXED (verified against the call site): hs_autocomplete_gather is (category mask 0x28, results) on the stack with
//   the prefix in EAX, 0x100 (the result capacity) in CX and the context mask in DX; the old prototype put the
//   prefix first, so the prefix pointer became the category mask.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "hs.h"
#include "main.h"
#include <string.h>

extern console_globals console_globals_data; // 0x006b7020
extern uint8_t hs_preserve_token_case;  // 0x007102fd

extern char *strchr(const char *string, int character); // CRT strchr (0x6257e0: the MSVC asm strchr)
extern uint32_t console_command_context_mask(uint32_t context_flags); // this module, 0x4c69c0
extern int16_t hs_autocomplete_gather(uint32_t category_mask, char **results, char *prefix, int16_t maximum_count,
    uint16_t gametype_mask); // 0x483c90, blam-cc: EAX -> prefix, CX -> maximum_count, DX -> gametype_mask,
    // stack -> category_mask, results; // 0x483c90, foreign (hs module)
    // blam-cc: EAX -> partial_name, ECX -> mode (0x100, UNSURE), EDX -> context_mask, stack -> max_count, out_names
extern void console_out_printf(uint8_t clear_first, const char *format, ...); // this module, 0x4c6860
extern char hs_compile_and_evaluate(const char *command); // 0x484400, foreign (hs module)
extern int32_t __stricmp(const char *a, const char *b);
extern int standalone_devmode(void); // standalone/loader.c: "-devmode" or HALO_DEVMODE

// Records command_line in the command history ring (unless it is a comment: leading ';', '#' or
// "//"), then checks that its first space-delimited word names an hs function currently allowed
// by console_command_context_mask(context_flags) before compiling and evaluating it. Reports and
// refuses to evaluate a command that is not currently available. Returns the compiled script's
// result, or 0 if the line was a comment or the command was unavailable.
char console_process_command(char *command_line, uint32_t context_flags) // blam-cc: EDI -> command_line
{
    char command_name[0x100];
    char *space;
    char *out_names[0x28];
    uint32_t context_mask;
    int16_t match_count;
    int16_t i;
    int16_t history_index;
    char result;

    if (command_line[0] == ';' || command_line[0] == '#' ||
        (command_line[0] == '/' && command_line[1] == '/')) {
        return 0;
    }

    strncpy(command_name, command_line, 0xff);
    space = strchr(command_name, ' ');
    if (space != 0) {
        *space = 0;
    }

    history_index = (int16_t)((console_globals_data.history_newest_index + 1) & 7);
    console_globals_data.history_newest_index = history_index;
    strcpy(console_globals_data.history[history_index], command_line);

    if (console_globals_data.history_count < 8) {
        console_globals_data.history_count = console_globals_data.history_count + 1;
    } else {
        console_globals_data.history_count = 8;
    }
    console_globals_data.history_browse_index = -1;

    context_mask = console_command_context_mask(context_flags);
    if (standalone_devmode()) {
        context_mask = 0; // STANDALONE EXTENSION (not in the binary): -devmode lifts the availability filter
    }
    match_count = hs_autocomplete_gather(0x28, out_names, command_name, 0x100, (uint16_t)context_mask);
    for (i = match_count - 1; i >= 0; i--) {
        if (__stricmp(command_name, out_names[i]) == 0) {
            hs_preserve_token_case = 1;
            result = hs_compile_and_evaluate(command_line);
            hs_preserve_token_case = 0;
            return result;
        }
    }
    console_out_printf(0, "Requested function \"%s\" cannot be executed now.", command_name);
    return 0;
}

#if 0
Original Ghidra decompilation (0x4c6a80):

char console_process_command(undefined4 param_1)

{
  char cVar1;
  short sVar2;
  undefined1 *puVar3;
  uint uVar4;
  char *pcVar5;
  int iVar6;
  char *pcVar7;
  char *unaff_EDI;
  char local_500 [252];
  undefined4 uStack_404;
  undefined4 local_400 [256];

  cVar1 = *unaff_EDI;
  if (((cVar1 != ';') && (cVar1 != '#')) && ((cVar1 != '/' || (unaff_EDI[1] != '/')))) {
    _strncpy(local_500,unaff_EDI,0xff);
    uStack_404._3_1_ = 0;
    puVar3 = (undefined1 *)FUN_006257e0(local_500,0x20);
    if (puVar3 != (undefined1 *)0x0) {
      *puVar3 = 0;
    }
    uVar4 = (int)DAT_006b79de + 1U & 0x80000007;
    if ((int)uVar4 < 0) {
      uVar4 = (uVar4 - 1 | 0xfffffff8) + 1;
    }
    DAT_006b79de = (short)uVar4;
    pcVar5 = &DAT_006b71e4 + DAT_006b79de * 0xff;
    pcVar7 = unaff_EDI;
    do {
      cVar1 = *pcVar7;
      pcVar7 = pcVar7 + 1;
      *pcVar5 = cVar1;
      pcVar5 = pcVar5 + 1;
    } while (cVar1 != '\0');
    iVar6 = (int)DAT_006b79dc;
    DAT_006b79dc = 8;
    if (iVar6 + 1 < 9) {
      DAT_006b79dc = (short)(iVar6 + 1);
    }
    DAT_006b79e0._0_2_ = 0xffff;
    FUN_004c69c0(param_1);
    sVar2 = hs_autocomplete_gather(0x28,local_400);
    while( true ) {
      if (sVar2 < 1) {
        console_out_printf('\0',"Requested function \"%s\" cannot be executed now.",local_500);
        return '\0';
      }
      iVar6 = __stricmp(local_500,(char *)(&uStack_404)[sVar2]);
      if (iVar6 == 0) break;
      sVar2 = sVar2 + -1;
    }
    DAT_007102fd = 1;
    cVar1 = hs_compile_and_evaluate(unaff_EDI);
    DAT_007102fd = 0;
    return cVar1;
  }
  return '\0';
}

Disassembly (0x4c6a80..0x4c6bbf) resolving the hs_autocomplete_gather registers:

004c6b3d:  lea    ecx,[esp+0x10c]        ; &out_names
004c6b44:  push   ecx
004c6b45:  push   0x28                   ; max_count
004c6b47:  mov    ecx,0x100
004c6b4c:  mov    edx,eax                ; context_mask (result of console_command_context_mask)
004c6b4e:  lea    eax,[esp+0x14]         ; &command_name
004c6b52:  call   0x483c90               ; hs_autocomplete_gather(EAX, ECX=0x100, EDX, 0x28, &out_names)
#endif
