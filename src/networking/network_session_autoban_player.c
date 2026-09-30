// network_session_autoban_player  (Ghidra: FUN_004e36e0; renamed, suggested by
// out/phase2/results/networking_04.json)
// address 0x4e36e0, size 233 bytes
// name confidence: 0.45   rewrite confidence: 0.65
// evidence: out/phase2/results/networking_04.json 0x4e36e0: "Resolves a player-index parameter
// to a session machine slot, skips already-disconnecting machines, logs 'AUTOBAN: Banning %S.',
// then calls network_banlist_add_ban followed by FUN_004e0af0 to disconnect/kick the machine."
// types/networking.h network_machine (machine_id at +0x0c, channel at +0x00), player (identifier
// at +0x00, name at +0x04, machine_index at +0x64); disassembly (objdump -d -M intel, bin/halo.exe)
// for the register convention and for the field this batch had not otherwise resolved.
// register convention: ECX -> player_handle. Confirmed by disassembly: `cmp ecx,0xffffffff`
// against the incoming register at function entry, with no stack args.
//   // blam-cc: ECX -> player_handle
// UNSURE: `movsx bx, BYTE PTR [edx+0x64]` reads only the low BYTE of player::machine_index (declared
// int16_t in types/game.h) and sign-extends it; modeled here as a byte read through the address
// of that field rather than redeclaring it, since the field's true width/meaning is not resolved
// by this batch. It is then compared against network_machine::machine_id (also int16_t) to find
// the owning machine slot, which is the closest thing to an established use of it in this module.
// UNSURE (load-bearing, preserved as-is): disassembly shows the "channel is still connected ->
// refuse" guard (`test bl,bl / jne`, reading channel->connected) only runs on the path where a
// matching machine slot with a non-NULL channel was actually found. When the loop exhausts all
// 16 machines without a match, execution still falls through to the AUTOBAN print and to
// `machine->unknown_5c` for the identity_lookup_key -- with `machine` NULL. This looks like a
// genuine latent null-pointer read in the original binary (only survivable if this function is
// never actually called for a player with no matching machine); it is preserved exactly rather
// than guarded, per the task's "no invented behaviour" rule.
// UNSURE: the player object's address 4 bytes in (`&target_player->name[0]`) is passed as the
// `network_player_entry *target_player` argument of network_banlist_add_ban, exactly as
// network_banlist_add_ban.c already declares it; only the leading UTF-16 name field of either
// struct is ever read by that callee's callee (FUN_00557950), and both structs happen to have
// their name array at that argument's offset 0, so this is safe punning rather than a real type
// match. See network_banlist_add_ban.c's own header for the full chain.
// UNSURE: FUN_004e0af0's reason code 6 matches sv_ban.c's own call to the same function with the
// same reason, and its blam-cc parameter shape (ECX -> reason, EDI -> machine, stack -> server)
// is copied from that already-resolved file; EDI is callee-saved under cdecl, so it still holds
// the machine pointer found by the loop above by the time this call executes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern data_array *player_data;                    // 0x0087a480
extern network_server_globals *network_server;      // 0x0071c2d4

extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)
extern uint8_t network_banlist_add_ban(int32_t identity_lookup_key, int32_t duration_override_seconds,
    network_player_entry *target_player); // this module, 0x4e35c0

    // blam-cc: ECX -> reason, EDI -> machine, stack -> server; foreign (< this batch), 0x4e0af0,
    // see sv_ban.c / sv_kick.c

// Auto-bans and disconnects player_handle (used by server-side logic such as excessive
// team-killing): finds the live player, finds the session machine slot whose machine_id matches
// its low byte, refuses if that machine's channel is already connected/being torn down, then logs
// the ban, adds it to the ban list, and kicks the machine.
uint8_t network_session_autoban_player(datum_index player_handle) // blam-cc: ECX -> player_handle
{
    int16_t index;
    int16_t salt;
    player *target_player;
    int8_t machine_key;
    network_machine *machine;
    int i;

    if (player_handle == k_datum_index_none) {
        return 0;
    }
    index = (int16_t)player_handle;
    if (index < 0) {
        return 0;
    }
    if (index >= player_data->maximum_count) {
        return 0;
    }
    target_player = (player *)((uint8_t *)player_data->data + (int32_t)player_data->size * (int32_t)index);
    if (target_player->identifier == 0) {
        return 0;
    }
    salt = (int16_t)(player_handle >> 16);
    if (salt != 0 && target_player->identifier != salt) {
        return 0;
    }
    machine_key = *(int8_t *)&target_player->machine_index; // UNSURE: low byte only, see file header
    machine = 0;
    for (i = 0; i < k_network_maximum_machines; i++) {
        if (network_server->machines[i].machine_id == machine_key) {
            machine = &network_server->machines[i];
            break;
        }
    }
    if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
        return 0;
    }
    chimera__console_out((ColorARGB *)0, "AUTOBAN: Banning %S.", target_player->name);
    if (!network_banlist_add_ban(machine->unknown_5c, 0, (network_player_entry *)target_player->name)) {
        return 0;
    }
    if (!network_server_notify_or_resend_challenge(6, machine, network_server)) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e36e0), from tools/pack.py 0x4e36e0:

undefined4 FUN_004e36e0(void)

{
  int *piVar1;
  char cVar2;
  short sVar3;
  int in_ECX;
  int iVar4;
  int iVar5;
  short *psVar6;
  short sVar7;

  if (((in_ECX != -1) && (sVar3 = (short)in_ECX, -1 < sVar3)) &&
     (sVar3 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar5 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar3;
    sVar3 = *(short *)(iVar5 + *(int *)(DAT_0087a480 + 0x34));
    iVar5 = iVar5 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar3 != 0) && ((sVar7 = (short)((uint)in_ECX >> 0x10), sVar7 == 0 || (sVar3 == sVar7))))
    {
      iVar4 = 0;
      psVar6 = (short *)(DAT_0071c2d4 + 0x3c4);
      do {
        if (*psVar6 == (short)*(char *)(iVar5 + 100)) {
          piVar1 = (int *)(iVar4 * 0x60 + 0x3b8 + DAT_0071c2d4);
          if (((piVar1 != (int *)0x0) && (iVar4 = *piVar1, iVar4 != 0)) &&
             (*(char *)(iVar4 + 0xa98) != '\0')) {
            return 0;
          }
          break;
        }
        iVar4 = iVar4 + 1;
        psVar6 = psVar6 + 0x30;
      } while (iVar4 < 0x10);
      iVar5 = iVar5 + 4;
      chimera__console_out("AUTOBAN: Banning %S.",iVar5);
      cVar2 = network_banlist_add_ban(iVar5);
      if ((cVar2 != '\0') && (cVar2 = FUN_004e0af0(DAT_0071c2d4), cVar2 != '\0')) {
        return 1;
      }
    }
  }
  return 0;
}

Disassembly (objdump -d -M intel, bin/halo.exe), the register-carried argument and the loop over
network_server->machines[]:
  4e36e2: cmp ecx,0xffffffff        ; player_handle == k_datum_index_none
  4e36ee: sar edi,0x10              ; salt = (int16_t)(player_handle >> 16)
  4e36fb: mov esi,[0x87a480]        ; esi = player_data
  4e3719: mov cx,[edx+ebx]          ; target_player->identifier, edx = target_player
  4e3736: movsx bx,byte ptr [edx+0x64]  ; machine_key = low byte of target_player->machine_index
  4e373c: mov ebp,[0x71c2d4]        ; ebp = network_server
  4e3746: lea esi,[ebp+0x3c4]       ; &network_server->machines[0].machine_id
  4e3750-4e375c: loop, stride 0x60, 16 iterations
  4e3760-4e376d: edi = &network_server->machines[i] on match
  4e3777: mov bl,[ecx+0xa98]        ; machine->channel->connected
  4e3781: lea esi,[edx+0x4]         ; &target_player->name[0]
  4e3791: mov eax,[edi+0x5c]        ; identity_lookup_key = machine->unknown_5c (edi may be NULL
                                    ; here if the loop found no match -- see UNSURE note above)
  4e37aa: mov ecx,0x6 / 4e37af: call 0x4e0af0   ; FUN_004e0af0(6, edi=machine, [0x71c2d4]=server)
#endif
