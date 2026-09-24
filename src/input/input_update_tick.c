// input_update_tick  (Ghidra: input_update_tick, already named)
// address 0x48b4b0, size 109 bytes (decompiled body continues through the tail Ghidra
//   mis-split off as a separate function at 0x48b520, "effect_new_on_object_marker"; that
//   address is the `jne` of `cmp al,1` inside this function's own mode dispatch and the jump
//   table at 0x48b5d4 belongs to it too -- see out/phase4/input_types_notes.md, so its code is
//   folded into this one file and 0x48b520 is not written separately)
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: objdump 0x48b4b0..0x48b5d4 confirmed against the Ghidra decompile of 0x48b4b0
//   (which already includes the 0x48b520 tail). ECX at the FUN_00490b50 call site is
//   `system_keys[esi]` (word load `mov cx,[esi*2+0x68e40c]`), matching types/input.h's
//   system_key_states / system_keys documentation. The mode_flags dispatch matches
//   input_mode_flags exactly (game/menu/keyboard-capture/bind-scan bits).
// register convention: no parameters, no return value.
//   // blam-cc: input_get_key_state(key) -> ECX = key (word), returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <string.h>

extern input_abstraction_globals input_globals; // 0x00710328
extern uint32_t input_menu_exit_deadline;                     // 0x00712918
extern int16_t system_keys[k_input_system_key_count];         // 0x0068e40c
extern int64_t performance_frequency;                         // 0x006ac8f8/0x006ac8fc
extern int32_t QueryPerformanceCounter(large_integer *counter); // 0x0063a0ac IAT

extern uint8_t input_get_key_state(int16_t key_index);        // 0x00490b50, blam-cc: CX -> key
extern void input_key_block_timers_expire(void);              // 0x00490ca0
extern void input_scan_any_bound_input(void);                 // 0x0048f8c0 (already named)
extern void input_menu_generate_events(void);                 // 0x0048ec50 (see input_types_notes.md)
extern void input_game_action_update(void);                   // 0x0048cca0 (body runs on through 0x48d270 to 0x48ea4f)

// Per-frame input tick. Resyncs the millisecond time base, marks the frame idle (cleared later
// if the game action update finds a change), expires key-block timers, refreshes the three
// system key hold states (grave, escape, print screen), and dispatches on the current input
// mode: run the game action update once the post-menu exit delay has passed, generate menu
// navigation events, clear the local player's action state while the keyboard rebind-capture UI
// owns the keyboard, or drive the bind-capture scan machine.
void input_update_tick(void)
{
    large_integer counter;
    uint32_t now_ms;
    int32_t i;
    uint8_t mode;

    QueryPerformanceCounter(&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / performance_frequency);

    input_globals.idle = 1;
    input_key_block_timers_expire();

    for (i = 0; i < k_input_system_key_count; i++) {
        input_globals.system_key_states[i] = input_get_key_state(system_keys[i]);
    }

    mode = input_globals.mode_flags;
    if (mode == _input_mode_game_bit) {
        if (input_menu_exit_deadline < now_ms) {
            input_game_action_update();
        }
    } else if ((mode & _input_mode_bind_scan_bit) != 0) {
        input_scan_any_bound_input();
    } else if ((mode & _input_mode_keyboard_capture_bit) != 0) {
        memset(&input_globals.states[0], 0, sizeof(input_globals.states[0]));
    } else if ((mode & _input_mode_menu_bit) != 0) {
        input_menu_exit_deadline = now_ms + k_input_menu_exit_delay_ms;
        input_menu_generate_events();
    }
    // else: none of the recognized mode bits are set -- no-op, matching the default jump-table
    // entry (sVar2 == 0xff falls past the `ja` at 0x48b552).
}

#if 0
Original Ghidra decompilation (0x48b4b0, includes the 0x48b520 tail), from tools/pack.py 0x48b4b0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void input_update_tick(void)

{
  undefined1 uVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  undefined8 uVar5;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar5 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar3 = __alldiv(uVar5,DAT_006ac8f8,DAT_006ac8fc);
  DAT_00712540 = 1;
  FUN_00490ca0();
  iVar4 = 0;
  do {
    uVar1 = FUN_00490b50();
    (&DAT_007127d0)[iVar4] = uVar1;
    iVar4 = iVar4 + 1;
  } while (iVar4 < 3);
  if (DAT_00712542 == 1) {
    sVar2 = 0;
  }
  else if ((DAT_00712542 & 8) == 0) {
    if ((DAT_00712542 & 4) == 0) {
      sVar2 = (-(ushort)((DAT_00712542 & 2) != 0) & 0xff02) + 0xff;
    }
    else {
      sVar2 = 2;
    }
  }
  else {
    sVar2 = 3;
  }
  switch(sVar2) {
  case 0:
    if (DAT_00712918 < uVar3) {
      FUN_0048cca0();
      return;
    }
    break;
  case 1:
    DAT_00712918 = uVar3 + 200;
    FUN_0048ec50();
    break;
  case 2:
    DAT_00712498 = 0;
    DAT_0071249c = 0;
    _DAT_007124a0 = 0;
    _DAT_007124a4 = 0;
    _DAT_007124a8 = 0;
    _DAT_007124ac = 0;
    _DAT_007124b0 = 0;
    _DAT_007124b4 = 0;
    _DAT_007124b8 = 0;
    _DAT_007124bc = 0;
    return;
  case 3:
    input_scan_any_bound_input();
    return;
  }
  return;
}

objdump 0x48b4b0..0x48b5d4 (Intel syntax), the register evidence for the FUN_00490b50 loop and
the jump table this decompile already resolved:

0048b4b0: sub    esp,0x8
0048b4b3: push   esi
0048b4b4: push   edi
0048b4b5: lea    eax,[esp+0x8]
0048b4b9: push   eax
0048b4ba: call   DWORD PTR ds:0x63a0ac                ; QueryPerformanceCounter
0048b4c0: mov    ecx,DWORD PTR [esp+0xc]
0048b4c4: mov    edx,DWORD PTR [esp+0x8]
0048b4c8: push   0x0
0048b4ca: push   0x3e8
0048b4cf: push   ecx
0048b4d0: push   edx
0048b4d1: call   0x62de80                              ; __allmul
0048b4d6: mov    ecx,DWORD PTR ds:0x6ac8fc
0048b4dc: push   ecx
0048b4dd: mov    ecx,DWORD PTR ds:0x6ac8f8
0048b4e3: push   ecx
0048b4e4: push   edx
0048b4e5: push   eax
0048b4e6: call   0x639230                              ; __alldiv
0048b4eb: mov    edi,eax                               ; edi = now_ms for the rest of the function
0048b4ed: mov    BYTE PTR ds:0x712540,0x1              ; idle = 1
0048b4f4: call   0x490ca0                              ; input_key_block_timers_expire
0048b4f9: xor    esi,esi
0048b500: mov    cx,WORD PTR [esi*2+0x68e40c]           ; ecx = system_keys[esi]
0048b508: call   0x490b50                               ; input_get_key_state(ecx)
0048b50d: mov    BYTE PTR [esi+0x7127d0],al             ; system_key_states[esi] = al
0048b513: inc    esi
0048b514: cmp    esi,0x3
0048b517: jl     0x48b500
0048b519: mov    al,ds:0x712542                         ; al = mode_flags
0048b51e: cmp    al,0x1
0048b520: jne    0x48b526                               ; (Ghidra name for this address: effect_new_on_object_marker)
0048b522: xor    eax,eax
0048b524: jmp    0x48b54c
0048b526: test   al,0x8
0048b528: je     0x48b531
0048b52a: mov    eax,0x3
0048b52f: jmp    0x48b54c
0048b531: test   al,0x4
0048b533: je     0x48b53c
0048b535: mov    eax,0x2
0048b53a: jmp    0x48b54c
0048b53c: and    al,0x2
0048b53e: neg    al
0048b540: sbb    eax,eax
0048b542: and    eax,0xffffff02
0048b547: add    eax,0xff
0048b54c: movsx  eax,ax
0048b54f: cmp    eax,0x3
0048b552: ja     0x48b5ce                               ; default: fall through to epilogue
0048b554: jmp    DWORD PTR [eax*4+0x48b5d4]
0048b55b: cmp    edi,DWORD PTR ds:0x712918
0048b561: jbe    0x48b5ce
0048b563: call   0x48cca0                                ; input_game_action_update
0048b568..0048b56d: epilogue / ret
0048b56e: call   0x48f8c0                                ; input_scan_any_bound_input
0048b573..0048b578: epilogue / ret
0048b579..0048b5bc: clear local_player_input_state[0] (0x712498..0x7124bc), epilogue / ret
0048b5bd: add    edi,0xc8                                ; now_ms + 200
0048b5c3: mov    DWORD PTR ds:0x712918,edi
0048b5c9: call   0x48ec50                                ; input_menu_generate_events
0048b5ce..0048b5d3: epilogue / ret
0048b5d4: jump table (4 dwords): 0048b55b, 0048b5bd, 0048b579, 0048b56e
#endif
