// network_game_server_handoff_object_ownership  (Ghidra: FUN_004dfa10, unnamed)
// address 0x4dfa10, size 512 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Attempts to hand off ownership of the
// object identified by param_2 to a matching channel slot in param_1's table, invoking a
// completion callback stored at DAT_006f1d20+0x90 once all 16 slots have been [processed]."
// param_1+0x1aa is network_server_globals::session.players (session at +0x008, players at
// session+0x1a2). param_1+0x3c4 is network_server_globals::machines[0].machine_id
// (machines at +0x3b8, machine_id at +0xc). player+0x20/+0x34 match types/game.h player's
// team and unit fields; object_header/object 0x106 match types/objects.h's vitality_flags
// and _object_health_frozen_bit.
// register convention: stack = server (network_server_globals *), machine (network_machine *).
// blam-cc: stack -> server, machine
// UNSURE: this function calls FUN_004df950 twice with no register set up for FUN_004df950's
// own EDI (object-count) parameter, meaning EDI must be an implicit pass-through parameter of
// THIS function too; modelled here as an extra, forwarded-only int32_t * parameter,
// register EDI, whose value this function never itself reads.
// UNSURE: when the inner machine-id search (over network_machine::machine_id) fails to find
// a match, the original leaves `iVar5` at 0 and falls straight into
// `*(byte *)(iVar5 + 0xe)`, i.e. a read of absolute address 0xe. Preserved exactly (no bounds
// check invented); this path is presumably unreachable in practice since a valid player
// entry's machine_index should always resolve to a live machine.
// UNSURE: FUN_004d98f0's parameter convention (a single byte in AL, here the entry's
// slot_index) is inferred only from this and the sibling call in
// network_object_release_ownership_claim.c.
// UNSURE: FUN_004779d0, FUN_00466e80, FUN_00466ee0, FUN_00477a80, build_player_full_resync_update
// signatures are inferred solely from their arguments at this call site.
// reconciled: R04 0x006f1d20 void * network_game_engine_callback_block -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480, stride 0x200 (game module)
extern data_array *object_data; // 0x008603b0 (objects module)
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)

extern void network_game_broadcast_team_object_updates(int32_t *object_count, uint32_t param_1, int32_t *bytes_sent); // 0x4df950, this batch
extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, blam-cc: EAX -> entry
    // blam-cc: EAX -> entry; 0x4de9f0, other module. The EAX convention is pinned by
    // network_server_check_machine_timeout (0x4e0f80 `mov eax,esi` / 0x4e102b
    // `lea eax,[esp+0x20]`), both immediately before the call.
extern uint32_t player_data_iterator_advance(uint8_t slot_index); // 0x4d98f0, other module (UNSURE)
extern int32_t game_engine_notify_object_value_event(int32_t team); // 0x4779d0, other module (UNSURE)
extern void build_player_full_resync_update(int32_t machine_id); // 0x4e7d90, other module (UNSURE)
extern int32_t game_engine_player_profile_cache_find(void); // 0x466e80, other module (UNSURE)
extern void game_engine_capture_player_profile(int32_t value); // 0x466ee0, other module (UNSURE)
extern void game_engine_send_unit_weapon_loadout(void *machine, int32_t team, int32_t machine_id); // 0x477a80, other module (UNSURE)

typedef void (*network_join_complete_callback)(int32_t unused, int32_t machine_id);

// For each of the 16 player-table slots, if the slot's (machine_index, machine_player_index)
// key does not already belong to `machine`, locates the network_machine record that does own
// it and, when that machine's channel is established and its player is a live, non-frozen
// unit, transfers network ownership of that unit to `machine`. Invokes the completion
// callback at current_game_engine+0x90 once all 16 slots are processed.
void network_game_server_handoff_object_ownership(int32_t *object_count_passthrough,
                                                    network_server_globals *server,
                                                    network_machine *machine)
{
    int32_t bytes_sent;
    int32_t machine_id;
    network_player_entry *entry;
    network_player_entry *scan;
    int32_t remaining;
    int32_t i;
    int8_t key_machine_index;
    network_machine *owner;
    uint32_t datum;
    int16_t player_index;
    int16_t salt;
    player *plr;
    int32_t team;
    int32_t unit;
    object_header *hdr;
    object *unit_obj;

    bytes_sent = 0;
    machine_id = (int32_t)machine->machine_id;
    network_game_broadcast_team_object_updates(object_count_passthrough, 0, &bytes_sent);
    network_game_broadcast_team_object_updates(object_count_passthrough, 0, &bytes_sent);

    entry = server->session.players;
    remaining = 16;
    for (;;) {
        int do_transfer;

        do_transfer = 0;
        owner = 0;
        if (network_player_entry_validate(entry) != 0) { // blam-cc: EAX -> entry
            key_machine_index = entry->machine_index;
            scan = server->session.players;
            i = 0;
            do {
                if (scan->machine_index == key_machine_index &&
                    scan->machine_player_index == entry->machine_player_index) {
                    if ((int16_t)key_machine_index != machine->machine_id) {
                        owner = 0;
                        for (i = 0; i <= 15; i = i + 1) {
                            if (server->machines[i].machine_id == (int16_t)key_machine_index) {
                                owner = &server->machines[i];
                                break;
                            }
                        }
                        do_transfer = 1;
                    }
                    break; // either way, stop scanning the player table
                }
                i = i + 1;
                scan = scan + 1;
            } while (i < 16);
        }

        if (do_transfer && owner != 0 && (owner->flags & 0x04) != 0) { // UNSURE: bit 0x04 is not
                                                                        // enumerated in network_machine_flags
            datum = player_data_iterator_advance((uint8_t)entry->slot_index) /* FIXED 2026-09-29: the original reads +0x1f, not +0x1e */;
            if (datum != 0xffffffff) {
                player_index = (int16_t)datum;
                if (player_index >= 0 && player_index < player_data->maximum_count) {
                    plr = (player *)((uint8_t *)player_data->data + player_data->size * player_index);
                    if (plr->identifier != 0) {
                        salt = (int16_t)(datum >> 16);
                        if (salt == 0 || plr->identifier == salt) {
                            team = plr->team;
                            unit = plr->unit;
                            game_engine_notify_object_value_event(team);
                            build_player_full_resync_update(machine_id);
                            if (game_engine_player_profile_cache_find() != -1) {
                                game_engine_capture_player_profile(0);
                            }
                            if ((uint32_t)unit != 0xffffffff) {
                                hdr = &((object_header *)object_data->data)[unit & 0xffff];
                                unit_obj = hdr->data;
                                if ((unit_obj->vitality_flags & 0x04) == 0) { // _object_health_frozen_bit
                                    game_engine_send_unit_weapon_loadout(owner, team, machine_id);
                                }
                            }
                        }
                    }
                }
            }
        }

        entry = entry + 1;
        remaining = remaining - 1;
        if (remaining == 0) {
            if (current_game_engine != 0 &&
                *(void **)((uint8_t *)current_game_engine + 0x90) != 0) {
                ((network_join_complete_callback)(*(void **)((uint8_t *)current_game_engine + 0x90)))(0, machine_id);
            }
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4dfa10):

void FUN_004dfa10(int param_1,int param_2)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  short sVar4;
  int iVar5;
  short *psVar6;
  short sVar7;
  int iVar8;
  int iVar9;
  int local_10;
  int local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_10 = 0;
  local_c = 0;
  iVar8 = (int)*(short *)(param_2 + 0xc);
  FUN_004df950(param_1,&local_10);
  FUN_004df950(param_1,&local_10);
  iVar9 = param_1 + 0x1aa;
  local_c = 0x10;
LAB_004dfa70:
  local_10 = iVar9;
  cVar1 = FUN_004de9f0();
  if (cVar1 != '\0') {
    cVar1 = *(char *)(iVar9 + 0x1c);
    iVar5 = 0;
    pcVar2 = (char *)(param_1 + 0x1c7);
    do {
      if ((pcVar2[-1] == cVar1) && (*pcVar2 == *(char *)(iVar9 + 0x1d))) {
        if ((short)cVar1 != *(short *)(param_2 + 0xc)) {
          iVar5 = 0;
          iVar3 = 0;
          psVar6 = (short *)(param_1 + 0x3c4);
          goto LAB_004dfac6;
        }
        break;
      }
      iVar5 = iVar5 + 1;
      pcVar2 = pcVar2 + 0x20;
    } while (iVar5 < 0x10);
  }
  goto LAB_004dfbd8;
  while( true ) {
    iVar3 = iVar3 + 1;
    psVar6 = psVar6 + 0x30;
    if (0xf < iVar3) break;
LAB_004dfac6:
    if (*psVar6 == (short)cVar1) {
      iVar5 = iVar3 * 0x60 + 0x3b8 + param_1;
      break;
    }
  }
  if (((((*(byte *)(iVar5 + 0xe) & 4) != 0) &&
       (iVar5 = FUN_004d98f0((int)*(char *)(iVar9 + 0x1f)), iVar5 != -1)) &&
      (sVar7 = (short)iVar5, -1 < sVar7)) && (sVar7 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar3 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar7;
    sVar7 = *(short *)(iVar3 + *(int *)(DAT_0087a480 + 0x34));
    iVar3 = iVar3 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar7 != 0) && ((sVar4 = (short)((uint)iVar5 >> 0x10), sVar4 == 0 || (sVar7 == sVar4)))) {
      local_8 = *(undefined4 *)(iVar3 + 0x20);
      local_4 = *(undefined4 *)(iVar3 + 0x34);
      FUN_004779d0(local_8);
      build_player_full_resync_update(iVar8);
      iVar9 = FUN_00466e80();
      if (iVar9 != -1) {
        FUN_00466ee0(0);
      }
      iVar9 = local_10;
      if ((*(uint *)(iVar3 + 0x34) != 0xffffffff) &&
         ((*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                             (*(uint *)(iVar3 + 0x34) & 0xffff) * 0xc) + 0x106) & 4) == 0)) {
        FUN_00477a80(iVar5,local_8,iVar8);
        iVar9 = local_10;
      }
    }
  }
LAB_004dfbd8:
  iVar9 = iVar9 + 0x20;
  local_c = local_c + -1;
  if (local_c == 0) {
    if (*(code **)(DAT_006f1d20 + 0x90) != (code *)0x0) {
      local_10 = iVar9;
      (**(code **)(DAT_006f1d20 + 0x90))(0,iVar8);
    }
    return;
  }
  goto LAB_004dfa70;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
