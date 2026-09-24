// console_command_context_mask  (Ghidra: FUN_004c69c0; named per types/main.h
//   console_command_context_flags, "Command context mask built by FUN_004c69c0")
// address 0x4c69c0, size 187 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: types/main.h console_command_context_flags (every bit below matches its comments
// exactly); types/game.h current_game_engine (0x006f1d20); types/saved_games.h
// saved_player_profile_slots[0].profile.flags (0x00712dd8+0x11c); main_globals_data.game_connection
// (0x00719720). Disassembly (objdump -d -M intel, bin/halo.exe) confirms the parameter is a
// plain stack argument (`or eax,DWORD PTR [ebp+0x8]`) and that the 8188-byte `rep movsd` Ghidra
// shows is a copy of the whole saved_player_profile_slot (0x1ffc-byte profile) onto the stack
// merely to read its `flags` field at a fixed +0x11c; that copy has no other observable effect,
// so it is not reproduced here (see the UNSURE note).
// register convention: context_flags is the recognized stack parameter (param_1).
// UNSURE: the 8188-byte on-stack copy of saved_player_profile_slots[0] before reading its
// flags field is dropped as a non-observable simplification (no aliasing/threading concern is
// visible in this function); reading the field directly is behaviourally identical.
// reconciled: R22 saved_player_profile_flags gains _saved_player_profile_end_credits_reached_bit (0x0004); the literal 4 now uses it

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "main.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern main_globals main_globals_data;                    // 0x00719700
extern saved_player_profile_slot saved_player_profile_slots[k_maximum_local_player_profiles]; // 0x00712dd8

// Builds the command-availability mask console_process_command checks a command's flags against
// (and chimera__autocomplete_gather filters candidates with): bit 0 and bit 6 are always set;
// bit 4 (no multiplayer engine loaded) or bit 3 (one is) is set depending on current_game_engine;
// bit 1 is set for a network host; a client additionally forbids bits 1 and 2; bit 5 may be
// requested by context_flags but is forbidden (cleared) unless the end-credits profile flag
// (saved_player_profile_flags bit 2, 0x0004) is set, or unconditionally forbidden while a
// multiplayer engine is loaded. context_flags itself may also carry forbidding bits (high byte)
// for any of the low seven bits, applied last.
uint32_t console_command_context_mask(uint32_t context_flags)
{
    uint32_t mask;

    if (current_game_engine == 0) {
        mask = _console_context_default_bit;
        if ((saved_player_profile_slots[0].profile.flags & _saved_player_profile_end_credits_reached_bit) == 0) {
            mask = _console_context_default_bit | k_console_context_exec_file; // forbid bit 5
        }
        mask = mask | _console_context_no_multiplayer_bit;
    } else {
        mask = _console_context_default_bit | _console_context_multiplayer_bit | k_console_context_exec_file; // forbid bit 5
    }

    mask = mask | _console_context_always_bit;
    if (main_globals_data.game_connection == _game_connection_network_client) {
        mask = mask | k_console_context_client_forbidden | _console_context_always_bit;
    } else if (main_globals_data.game_connection == _game_connection_network_server) {
        mask = mask | _console_context_host_bit;
    }

    mask = mask | context_flags;
    if (mask & (_console_context_default_bit << k_console_context_forbidden_shift)) {
        mask = mask & ~_console_context_default_bit;
    }
    if (mask & (_console_context_host_bit << k_console_context_forbidden_shift)) {
        mask = mask & ~_console_context_host_bit;
    }
    if (mask & (_console_context_unknown_04 << k_console_context_forbidden_shift)) {
        mask = mask & ~_console_context_unknown_04;
    }
    if (mask & (_console_context_multiplayer_bit << k_console_context_forbidden_shift)) {
        mask = mask & ~_console_context_multiplayer_bit;
    }
    if (mask & (_console_context_no_multiplayer_bit << k_console_context_forbidden_shift)) {
        mask = mask & ~_console_context_no_multiplayer_bit;
    }
    if (mask & (_console_context_unknown_20 << k_console_context_forbidden_shift)) {
        mask = mask & ~_console_context_unknown_20;
    }
    if (mask & (_console_context_always_bit << k_console_context_forbidden_shift)) {
        mask = mask & ~_console_context_always_bit;
    }
    return mask;
}

#if 0
Original Ghidra decompilation (0x4c69c0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_004c69c0(uint param_1)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined4 local_2008 [71];
  byte local_1eec;

  uVar1 = 1;
  if (DAT_006f1d20 == 0) {
    puVar4 = &DAT_00712dd8;
    puVar5 = local_2008;
    for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    if ((local_1eec >> 2 & 1) == 0) {
      uVar1 = 0x2001;
    }
    uVar1 = uVar1 | 0x10;
  }
  else {
    uVar1 = 0x2009;
  }
  uVar2 = uVar1 | 0x40;
  if (DAT_00719720 == 1) {
    uVar2 = uVar1 | 0x640;
  }
  else if (DAT_00719720 == 2) {
    uVar2 = uVar1 | 0x42;
  }
  uVar2 = uVar2 | param_1;
  if ((param_1 & 0x100) != 0) {
    uVar2 = uVar2 & 0xfffe;
  }
  if ((uVar2 & 0x200) != 0) {
    uVar2 = uVar2 & 0xfffd;
  }
  if ((uVar2 & 0x400) != 0) {
    uVar2 = uVar2 & 0xfffb;
  }
  if ((uVar2 & 0x800) != 0) {
    uVar2 = uVar2 & 0xfff7;
  }
  if ((uVar2 & 0x1000) != 0) {
    uVar2 = uVar2 & 0xffef;
  }
  if ((uVar2 & 0x2000) != 0) {
    uVar2 = uVar2 & 0xffdf;
  }
  if ((uVar2 & 0x4000) != 0) {
    uVar2 = uVar2 & 0xffbf;
  }
  return uVar2;
}

Disassembly (0x4c69c0..0x4c6a7a) confirming the parameter and the flags-field read:

004c69c0:  push   ebp
004c69c1:  mov    ebp,esp
004c69c3:  and    esp,0xfffffff8
004c69c6:  mov    eax,0x2000
004c69cb:  call   0x628240              ; alloca(0x2000)
004c69d0:  mov    ecx,DWORD PTR ds:0x6f1d20   ; current_game_engine
004c69d6:  test   ecx,ecx
004c69d8:  movsx  edx,WORD PTR ds:0x719720    ; main_globals_data.game_connection
004c69df:  push   esi
004c69e0:  push   edi
004c69e1:  mov    eax,0x1
004c69e6:  je     0x4c69ef
004c69e8:  mov    eax,0x2009
004c69ed:  jmp    0x4c6a16
004c69ef:  mov    ecx,0x7ff
004c69f4:  mov    esi,0x712dd8
004c69f9:  lea    edi,[esp+0x8]
004c69fd:  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
004c69ff:  mov    cl,BYTE PTR [esp+0x124]     ; saved_player_profile_slots[0].profile.flags low byte
004c6a06:  shr    cl,0x2
004c6a09:  test   cl,0x1
004c6a0c:  jne    0x4c6a13
004c6a0e:  mov    eax,0x2001
004c6a13:  or     eax,0x10
004c6a16:  or     eax,0x40
004c6a19:  cmp    edx,0x1
004c6a1c:  jne    0x4c6a25
004c6a1e:  or     eax,0x600
004c6a23:  jmp    0x4c6a2c
004c6a25:  cmp    edx,0x2
004c6a28:  jne    0x4c6a2c
004c6a2a:  or     eax,edx
004c6a2c:  or     eax,DWORD PTR [ebp+0x8]     ; context_flags
004c6a2f:  test   ah,0x1
004c6a32:  je     0x4c6a39
004c6a34:  and    eax,0xfffe
004c6a39:  test   ah,0x2
004c6a3c:  je     0x4c6a43
004c6a3e:  and    eax,0xfffd
004c6a43:  test   ah,0x4
004c6a46:  je     0x4c6a4d
004c6a48:  and    eax,0xfffb
004c6a4d:  test   ah,0x8
004c6a50:  je     0x4c6a57
004c6a52:  and    eax,0xfff7
004c6a57:  test   ah,0x10
004c6a5a:  je     0x4c6a61
004c6a5c:  and    eax,0xffef
004c6a61:  test   ah,0x20
004c6a64:  je     0x4c6a6b
004c6a66:  and    eax,0xffdf
004c6a6b:  test   ah,0x40
004c6a6e:  je     0x4c6a75
004c6a70:  and    eax,0xffbf
#endif
