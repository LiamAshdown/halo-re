// console_initialize  (Ghidra: FUN_004c62d0, split by Ghidra at 0x4c62f0 / 0x4c6340; the orphan
//   pass first wrote it as shell_console_window_state_initialize, renamed and moved to main in
//   the same pass's review)
// address 0x4c62d0, size 189 bytes (0x4c62d0..0x4c638c, `ret`s at 0x4c6382 and 0x4c638c; modules.json
//   listed 32 bytes and catalogued the continuation as the bogus function 0x4c62f0
//   "weapon_prevents_grenade_throwing")
// name confidence: 0.85   rewrite confidence: 0.85
// evidence: src/main/main_loop.c already calls `console_initialize` at this address and
//   src/main/README.md records 0x4c62d0 / 0x4c62f0 / 0x4c6340 as that one function. Every store
//   (objdump 0x4c62d0..0x4c638d) lands in console_globals (types/main.h, 0x006b7020):
//   0x006b70a8..0x006b70b4 terminal.color <- console_default_color (0x00696554, types/main.h);
//   0x006b70b8 / 0x006b70bc / 0x006b70be terminal.prompt <- the 7 bytes at 0x0066b254,
//   "halo( " and its terminator (copied as a dword, a word and a byte); 0x006b70d8
//   terminal.input[0] = 0; 0x006b79dc history_count = 0; 0x006b79de / 0x006b79e0
//   history_newest_index / history_browse_index = -1; 0x006b7021 enabled = whether any argument
//   that starts with '-' equals "-console" (_stricmp against 0x0066b248, shell_argv 0x00721e90,
//   shell_argc 0x00721e94).
// register convention: no parameters.
//   // blam-cc: none

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "main.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern console_globals console_globals_data; // 0x006b7020
extern ColorARGB console_default_color;      // 0x00696554
extern char **shell_argv;                    // 0x00721e90
extern int32_t shell_argc;                   // 0x00721e94

// Resets the console: default input color, the "halo( " prompt, an empty input line and
// history, and enables it when the command line carries -console.
void console_initialize(void)
{
    static const char k_console_prompt[7] = "halo( "; // 0x0066b254
    int32_t i;

    console_globals_data.terminal.color = console_default_color;
    for (i = 0; i < 7; i++) {
        console_globals_data.terminal.prompt[i] = k_console_prompt[i];
    }
    console_globals_data.history_newest_index = -1;
    console_globals_data.history_browse_index = -1;
    console_globals_data.terminal.input[0] = 0;
    console_globals_data.history_count = 0;

    for (i = 0; i < shell_argc; i++) {
        char *argument = shell_argv[i];
        if (argument[0] == '-' && _stricmp("-console", argument) == 0) {
            console_globals_data.enabled = 1;
            return;
        }
    }
    console_globals_data.enabled = 0;
}

#if 0
Original Ghidra decompilation, both halves (0x4c62d0 and its fall-through continuation,
misattributed by an earlier phase as a separate function "weapon_prevents_grenade_throwing"
@ 0x4c62f0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004c62d0(void)

{
  char *_Str2;
  int iVar1;
  int iVar2;

  _DAT_006b70a8 = DAT_00696554;
  _DAT_006b70b4 = DAT_00696560;
  _DAT_006b70ac = DAT_00696558;
  DAT_006b70be = 0;
  _DAT_006b70b8 = 0x6f6c6168;
  DAT_006b79de = 0xffff;
  DAT_006b79e0._0_2_ = 0xffff;
  _DAT_006b70b0 = DAT_0069655c;
  iVar2 = 0;
  _DAT_006b70bc = 0x2028;
  DAT_006b70d8 = 0;
  DAT_006b79dc = 0;
  if (0 < DAT_00721e94) {
    do {
      _Str2 = *(char **)(DAT_00721e90 + iVar2 * 4);
      if (*_Str2 == '-') {
        iVar1 = __stricmp("-console",_Str2);
        if (iVar1 == 0) {
          DAT_006b7021 = 1;
          return;
        }
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_00721e94);
  }
  DAT_006b7021 = 0;
  return;
}

void weapon_prevents_grenade_throwing(void)

{
  char *_Str2;
  int iVar1;
  undefined4 in_ECX;
  undefined4 in_EDX;
  int iVar2;

  DAT_006b70be = 0;
  _DAT_006b70b8 = 0x6f6c6168;
  DAT_006b79de = 0xffff;
  DAT_006b79e0._0_2_ = 0xffff;
  iVar2 = 0;
  _DAT_006b70bc = 0x2028;
  DAT_006b70d8 = 0;
  DAT_006b79dc = 0;
  _DAT_006b70ac = in_ECX;
  _DAT_006b70b0 = in_EDX;
  if (0 < DAT_00721e94) {
    do {
      _Str2 = *(char **)(DAT_00721e90 + iVar2 * 4);
      if (*_Str2 == '-') {
        iVar1 = __stricmp("-console",_Str2);
        if (iVar1 == 0) {
          DAT_006b7021 = 1;
          return;
        }
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_00721e94);
  }
  DAT_006b7021 = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
