// sv_ban  (Ghidra: sv_ban, already named)
// address 0x4e3990, size 231 bytes
// name confidence: 0.85   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md; CEA-pdb match on "sv_ban is a server-only
// function!"; disassembly (objdump -d -M intel) pins the console-command convention and the
// FUN_004e0810/FUN_004e0af0/network_banlist_add_ban register mapping (see this file's #if 0
// block, and network_banlist_add_ban.c for the callee side).
// register convention: EAX -> argument_count, ECX -> arguments.
//   // blam-cc: EAX -> argument_count, ECX -> arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int16_t network_game_mode; // 0x00719720 (types/game.h), 2 == host
extern char sv_ban_duration_arg_buffer[]; // 0x0066d6a4, UNSURE: scratch buffer reused by 0x4e51c0
extern network_server_globals *network_server; // 0x0071c2d4

extern network_player_entry *sv_find_client_by_name_or_index(char *name_or_index); // this batch, 0x4e3f70
extern int32_t parse_time_duration_string(char *string, char default_unit, uint8_t *unit_table); // this batch, 0x4e51c0
extern network_machine *network_machine_find_by_id(network_server_globals *server, int16_t machine_id);
    // blam-cc: ESI -> server, EDI -> machine_id; foreign (< this batch), 0x4e0810
extern uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine, network_server_globals *server);
    // blam-cc: ECX -> reason, EDI -> machine, stack -> server; foreign (< this batch), 0x4e0af0
extern uint8_t network_banlist_add_ban(int32_t identity_lookup_key, int32_t duration_override_seconds,
    network_player_entry *target_player); // this batch, 0x4e35c0
extern void *console_color_006851fc; // 0x006851fc, a ColorARGB * the original loads into EAX
extern void *console_color_00685218; // 0x00685218, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: bans (and disconnects) the client named or indexed by the first argument, for
// the duration given by an optional second argument (parsed as d/h/m/s, default minutes) or the
// escalating penalty table if omitted. Refuses on a client or against the local client.
void sv_ban(uint32_t argument_count, int32_t *arguments) // blam-cc: EAX -> argument_count, ECX -> arguments
{
    int32_t duration = 0;
    network_player_entry *player;
    network_machine *machine;

    if (network_game_mode != 2) {
        chimera__console_out((ColorARGB *)0, "sv_ban is a server-only function!");
        return;
    }
    if (0 < (int32_t)argument_count && (int32_t)argument_count < 3) {
        if (argument_count == 2) {
            duration = parse_time_duration_string((char *)arguments[1], 'm', (uint8_t *)sv_ban_duration_arg_buffer);
            if (duration == -1) {
                chimera__console_out((ColorARGB *)console_color_00685218, "Incorrect usage. Type help sv_ban for more information.");
                return;
            }
        }
        player = sv_find_client_by_name_or_index((char *)arguments[0]);
        if (player != 0) {
            machine = network_machine_find_by_id(network_server, player->machine_index);
            if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
                chimera__console_out((ColorARGB *)0, "sv_ban:  Can't ban a local client!");
                return;
            }
            network_banlist_add_ban(machine->unknown_5c, duration, player);
            network_server_notify_or_resend_challenge(6, machine, network_server);
        }
        return;
    }
    chimera__console_out((ColorARGB *)console_color_006851fc, "Incorrect usage. Type help sv_ban for more information.");
}

#if 0
Original Ghidra decompilation (0x4e3990), from tools/pack.py 0x4e3990:

void sv_ban(void)

{
  int in_EAX;
  int iVar1;
  int *piVar2;

  if (DAT_00719720 != 2) {
    chimera__console_out("sv_ban is a server-only function!");
    return;
  }
  if ((0 < in_EAX) && (in_EAX < 3)) {
    if (in_EAX == 2) {
      iVar1 = parse_time_duration_string(0x6d,&DAT_0066d6a4);
      if (iVar1 == -1) {
        chimera__console_out("Incorrect usage. Type help sv_ban for more information.");
        return;
      }
    }
    iVar1 = sv_find_client_by_name_or_index();
    if (iVar1 != 0) {
      piVar2 = (int *)FUN_004e0810();
      if (((piVar2 != (int *)0x0) && (*piVar2 != 0)) && (*(char *)(*piVar2 + 0xa98) != '\0')) {
        chimera__console_out("sv_ban:  Can\'t ban a local client!");
        return;
      }
      network_banlist_add_ban(iVar1);
      FUN_004e0af0(DAT_0071c2d4);
    }
    return;
  }
  chimera__console_out("Incorrect usage. Type help sv_ban for more information.");
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) for the register convention:
  4e39b7: mov esi,[ecx]                ; esi = arguments[0]
  4e39bb: mov eax,[ecx+4]              ; eax = arguments[1], parse_time_duration_string's string arg
  4e39be: push 0x66d6a4
  4e39c3: push 0x6d                    ; 'm'
  4e39c5: call parse_time_duration_string
  4e39e7: mov eax,esi
  4e39e9: call sv_find_client_by_name_or_index    ; EAX = arguments[0]; ECX unchanged from entry
  4e39f4: mov esi,ds:0x71c2d4
  4e39fb: movsx edi,BYTE PTR [ebx+0x1c]           ; edi = player->machine_index
  4e39ff: call FUN_004e0810
  4e3a1a: mov eax,[edi+0x5c]                      ; identity_lookup_key = machine->unknown_5c
  4e3a1d: push ebx                                 ; target_player, stack arg
  4e3a1e: mov ecx,ebp                              ; duration
  4e3a20: call network_banlist_add_ban
  4e3a25: mov eax,ds:0x71c2d4
  4e3a2a: push eax                                 ; server, stack arg
  4e3a2b: mov ecx,0x6                              ; reason = 6
  4e3a30: call FUN_004e0af0
#endif
