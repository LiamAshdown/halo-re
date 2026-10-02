// sv_single_flag_force_reset  (Ghidra: sv_single_flag_force_reset, already named)
// address 0x4e3100, size 86 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md; disassembly (objdump -d -M intel, bin/halo.exe)
// -- Ghidra's own decompile of this function is materially wrong (two "Removing unreachable
// block" warnings hide the entire notification path), so this rewrite is built from the
// disassembly directly rather than from the pseudo-C:
//   4e3101: mov al,ds:0x71c306         ; old_value = DAT_0071c306
//   4e3107/4e310b: [esp+7]=al, [esp+6]=al  ; before/value scratch bytes, both seeded with old_value
//   4e310f: mov eax,[esp+0x10]         ; eax = 2nd caller stack arg (arguments)
//   4e3113: push 0x661094              ; name = "sv_single_flag_force_reset"
//   4e3118: push eax                   ; arguments
//   4e3119: mov eax,[esp+0x14]         ; eax = 1st caller stack arg (argument_count) -- loaded
//                                       ;   into EAX right before the call and never stored,
//                                       ;   i.e. it is console_command_bool_get_set's EAX input
//   4e311d: lea ebx,[esp+0xe]          ; value = &[esp+6]'s address (the "value" scratch byte)
//   4e3121: call 0x4e2990              ; console_command_bool_get_set(argument_count, &value,
//                                       ;   arguments, name)
//   4e3126/4e312a: reload value (bl) and the untouched before-copy (al)
//   4e3131: cmp bl,al / je 0x4e314d    ; skip the notice if the value did not change
//   4e3135: cmp DAT_006f1d20,0 / je 0x4e314d  ; skip if no engine callback block (not mid-game)
//   4e313e/4e3145: chimera__console_out("Game in progress...  Changes will apply to the next game.")
//   4e314d: mov DAT_0071c306,bl        ; always persist the (possibly unchanged) value
// register convention: both parameters are genuine stack (cdecl) parameters -- this function is
// one of the "pass both on the stack" exceptions sv_kick.c's own header notes, not the more
// common EAX/stack split.
// blam-cc: stack -> argument_count, arguments
// reconciled: R04 0x006f1d20 void * network_engine_callback_block -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t network_single_flag_force_reset_value; // 0x0071c306 (UNSURE name)
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)

extern void console_command_bool_get_set(uint32_t argument_count, uint8_t *value, char **arguments,
    const char *name); // 0x4e2990, this module
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// blam-cc: stack -> argument_count, arguments
// Console command: gets or sets the "single flag force reset" boolean via the shared
// boolean-command helper; if the stored value actually changes while a game is in progress
// (current_game_engine != 0), warns that the change only takes effect next game.
void sv_single_flag_force_reset(uint32_t argument_count, char **arguments)
{
    uint8_t old_value = network_single_flag_force_reset_value;
    uint8_t new_value = old_value;

    console_command_bool_get_set(argument_count, &new_value, arguments, "sv_single_flag_force_reset");

    if (new_value != old_value && current_game_engine != 0) {
        chimera__console_out((ColorARGB *)0, (char *)"Game in progress...  Changes will apply to the next game.");
    }
    network_single_flag_force_reset_value = new_value;
}

#if 0
Original Ghidra decompilation (0x4e3100), from tools/pack.py 0x4e3100:

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Removing unreachable block (ram,0x004e3135) */
/* WARNING: Removing unreachable block (ram,0x004e313e) */

void sv_single_flag_force_reset(undefined4 param_1,undefined4 param_2)

{
  undefined1 uVar1;

  uVar1 = DAT_0071c306;
  console_command_bool_get_set(param_2,"sv_single_flag_force_reset");
  DAT_0071c306 = uVar1;
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) -- see header for the annotated walkthrough that
supersedes the pseudo-C above:
  4e3100: 51                push   ecx
  4e3101: a0 06 c3 71 00    mov    al,ds:0x71c306
  4e3106: 53                push   ebx
  4e3107: 88 44 24 07       mov    BYTE PTR [esp+0x7],al
  4e310b: 88 44 24 06       mov    BYTE PTR [esp+0x6],al
  4e310f: 8b 44 24 10       mov    eax,DWORD PTR [esp+0x10]
  4e3113: 68 94 10 66 00    push   0x661094
  4e3118: 50                push   eax
  4e3119: 8b 44 24 14       mov    eax,DWORD PTR [esp+0x14]
  4e311d: 8d 5c 24 0e       lea    ebx,[esp+0xe]
  4e3121: e8 6a f8 ff ff    call   0x4e2990
  4e3126: 8a 5c 24 0e       mov    bl,BYTE PTR [esp+0xe]
  4e312a: 8a 44 24 0f       mov    al,BYTE PTR [esp+0xf]
  4e312e: 83 c4 08          add    esp,0x8
  4e3131: 3a d8             cmp    bl,al
  4e3133: 74 18             je     0x4e314d
  4e3135: a1 20 1d 6f 00    mov    eax,ds:0x6f1d20
  4e313a: 85 c0             test   eax,eax
  4e313c: 74 0f             je     0x4e314d
  4e313e: 68 34 d8 66 00    push   0x66d834
  4e3143: 33 c0             xor    eax,eax
  4e3145: e8 06 3a fb ff    call   0x496b50
  4e314a: 83 c4 04          add    esp,0x4
  4e314d: 88 1d 06 c3 71 00 mov    BYTE PTR ds:0x71c306,bl
  4e3153: 5b                pop    ebx
  4e3154: 59                pop    ecx
  4e3155: c3                ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
