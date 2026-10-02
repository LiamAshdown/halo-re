// sv_kick  (Ghidra: sv_kick, already named)
// address 0x4e3910, size 115 bytes
// name confidence: 0.9   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md; CEA-pdb match on the "sv_kick is a server-only
// function!" string; the console-command register convention (EAX/ECX) is established across
// this batch by sv_maxplayers_evaluate.c's existing extern (src/hs/sv_maxplayers_evaluate.c:
// "blam-cc: EAX -> argument_count ... which pass both on the stack" for the two exceptions) and
// confirmed here by disassembly (objdump -d -M intel): `test eax,eax` / `mov ecx,[ecx]`-shaped
// argument access.
// register convention: EAX -> name_or_index. This differs from most sv_* commands in this batch
// (EAX -> argument_count, ECX -> arguments): sv_kick always takes exactly one required console
// argument, and disassembly shows it calls sv_find_client_by_name_or_index (this batch, which
// itself takes a single string in EAX) immediately, without first moving anything into EAX --
// so sv_kick's own incoming EAX must already be that string, not a count.
//   // blam-cc: EAX -> name_or_index
// UNSURE: FUN_004e0810 and FUN_004e0af0 (both foreign, < this batch) -- their parameter shapes
// are reconstructed from this function's and sv_ban.c's disassembly (see those files' #if 0
// blocks) rather than from their own decompilation.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t network_game_mode; // 0x00719720 (types/game.h), 2 == host
extern network_server_globals *network_server; // 0x0071c2d4

extern network_player_entry *sv_find_client_by_name_or_index(char *name_or_index); // this batch, 0x4e3f70
extern network_machine *network_machine_find_by_id(network_server_globals *server, int16_t machine_id);
    // blam-cc: ESI -> server, EDI -> machine_id; foreign (< this batch), 0x4e0810
extern uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine, network_server_globals *server);
    // blam-cc: ECX -> reason, EDI -> machine, stack -> server; foreign (< this batch), 0x4e0af0
extern void *global_white_argb; // 0x006851fc, a ColorARGB * the original loads into EAX
extern void *console_message_default_color; // 0x00685218, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: disconnects (kicks) the client named or indexed by the console argument.
// Refuses if this machine is not hosting, if no matching client is found, or if the resolved
// machine is the local (listen-server) client.
void sv_kick(char *name_or_index) // blam-cc: EAX -> name_or_index
{
    network_player_entry *player;
    network_machine *machine;

    if (network_game_mode != 2) {
        chimera__console_out((ColorARGB *)console_message_default_color, (char *)"sv_kick is a server-only function!");
        return;
    }
    player = sv_find_client_by_name_or_index(name_or_index);
    if (player != 0) {
        machine = network_machine_find_by_id(network_server, player->machine_index);
        if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
            chimera__console_out((ColorARGB *)global_white_argb, (char *)"sv_kick:  Can't kick a local client!");
            return;
        }
        network_server_notify_or_resend_challenge(7, machine, network_server);
    }
}

#if 0
Original Ghidra decompilation (0x4e3910), from tools/pack.py 0x4e3910:

void sv_kick(void)

{
  undefined4 uVar1;
  int iVar2;
  int *piVar3;

  if (DAT_00719720 != 2) {
    chimera__console_out("sv_kick is a server-only function!");
    return;
  }
  iVar2 = sv_find_client_by_name_or_index();
  uVar1 = DAT_0071c2d4;
  if (iVar2 != 0) {
    piVar3 = (int *)FUN_004e0810();
    if (((piVar3 != (int *)0x0) && (*piVar3 != 0)) && (*(char *)(*piVar3 + 0xa98) != '\0')) {
      chimera__console_out("sv_kick:  Can\'t kick a local client!");
      return;
    }
    FUN_004e0af0(uVar1);
  }
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) for the register convention:
  4e3910: cmp WORD PTR ds:0x719720,0x2
  4e391a: call sv_find_client_by_name_or_index    ; EAX=argument_count, ECX=arguments unchanged
  4e392b: movsx edi,BYTE PTR [eax+0x1c]            ; edi = player->machine_index
  4e392f: call FUN_004e0810                        ; esi = network_server (set at 4e3924)
  4e3948: push esi                                 ; server, stack arg
  4e3949: mov ecx,0x7                               ; reason = 7
  4e394e: mov edi,eax                               ; machine
  4e3950: call FUN_004e0af0
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
