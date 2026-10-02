// network_game_server_handle_client_join  (Ghidra: FUN_004dfc90, unnamed)
// address 0x4dfc90, size 643 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Server/host-side handling of a client
// (re)join: validates the channel table, timestamps the join, and either delegates to the
// client path or spawns the new player's game objects and updates the owners..."
// param_1+0x1aa/+0x1c7/+0x3c4 match network_server_globals::session.players and
// ::machines[0].machine_id exactly as in the sibling ownership-handoff functions in this
// batch. The free-slot scan at DAT_006b0b88 matches types/game.h's player_profile_cache[16]
// (stride 0x30, in_use at +0x00, player datum at +0x04) and player_profile_cache_count
// (0x006f1d34) exactly.
// register convention: stack = server (network_server_globals *), machine (network_machine *).
// blam-cc: stack -> server, machine
// UNSURE: mirrors network_game_server_handoff_object_ownership.c's EDI (object-count)
// pass-through: this function calls FUN_004dfa10 with only its two visible arguments, so it
// must itself carry the same implicit EDI parameter, forwarded unchanged.
// UNSURE: the trailing `FUN_004dfc10()` call has no visible argument either; FUN_004dfc10's
// own parameter is BL (a slot index) per network_object_release_ownership_claim.c, so this
// function is modelled as also taking an implicit, forwarded-only BL byte. No source for that
// byte is visible in this function's own body.
// UNSURE: network_player_join_finalize, FUN_004de870, FUN_0045c440, FUN_00479f40 and
// datum_new_at_index_with_salt are called with no visible arguments at these sites; Ghidra's
// own signatures for them are likewise argument-less, so they are declared and called that
// way here, but their true register-passed parameters (if any) are not re-derived.
// UNSURE: the byte at network_machine+0x0e/0x0f is treated as a 16-bit read-modify-write in
// the original (`|= 4` on the word), but only bit 0x04 of the low byte (flags) actually
// changes; rewritten as `machine->flags |= 0x04`, which is byte-for-byte equivalent. Bit 0x04
// is not one of the enumerated network_machine_flags.
// UNSURE: server+0x9c4 has no named field (it falls inside network_server_globals's
// unresolved unknown_9bc span); accessed here through an explicit offset cast.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_client_globals *network_client; // 0x0071c2d8
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern player_profile player_profile_cache[16]; // 0x006b0b88
extern int32_t player_profile_cache_count; // 0x006f1d34

extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, blam-cc: EAX -> entry
    // blam-cc: EAX -> entry; 0x4de9f0, other module. The EAX convention is pinned by
    // network_server_check_machine_timeout (0x4e0f80 `mov eax,esi` / 0x4e102b
    // `lea eax,[esp+0x20]`), both immediately before the call.
extern char network_player_join_finalize(void); // 0x4d9e30, other module (UNSURE args)
extern char network_channel_key_open(void); // 0x4de870, other module (UNSURE args)
extern uint32_t player_data_iterator_advance(uint8_t slot_index); // 0x4d98f0, other module (UNSURE)
extern void datum_new_at_index_with_salt(void); // 0x4d03d0, memory module (UNSURE args here)
extern void player_update_queue_create(void); // other module (UNSURE)
extern void game_engine_player_new_life(uint32_t player_datum); // other module (UNSURE)
extern int32_t game_engine_player_profile_cache_find(void); // 0x466e80, other module (UNSURE)
extern void network_game_server_handoff_object_ownership(int32_t *object_count_passthrough,
    network_server_globals *server, network_machine *machine); // 0x4dfa10, this batch
extern void network_object_release_ownership_claim(uint8_t slot_index); // 0x4dfc10, this batch

// One-time per-round bookkeeping (first call this round records whether a listen-server
// client exists and stamps a millisecond timestamp), then scans the 16 player-table slots
// for the one whose key matches `machine`, finalizes that player's game object (via the
// stats-logging-aware path selection), records it into player_profile_cache, and hands the
// result to network_game_server_handoff_object_ownership /
// network_object_release_ownership_claim.
void network_game_server_handle_client_join(int32_t *object_count_passthrough,
                                             network_server_globals *server,
                                             network_machine *machine,
                                             uint8_t bl_passthrough)
{
    network_player_entry *entry;
    network_player_entry *scan;
    char valid;
    char handled;
    char ok;
    int32_t i;
    int32_t remaining;
    int32_t *field_9c4;
    uint8_t *unknown_9bc_base;
    uint32_t player_datum;
    int32_t profile_index;
    player_profile *profile;
    int32_t j;

    machine->flags |= 0x04;
    unknown_9bc_base = (uint8_t *)server;
    field_9c4 = (int32_t *)(unknown_9bc_base + 0x9c4);

    if (server->state != 1) {
        int16_t *machine_id_ptr;
        char all_processed_or_invalid;

        all_processed_or_invalid = 1;
        machine_id_ptr = (int16_t *)((uint8_t *)server + 0x3c4);
        for (i = 4; i != 0; i = i - 1) {
            if (machine_id_ptr[0] >= 0 && machine_id_ptr[0] < 16 &&
                (((uint8_t *)machine_id_ptr)[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            if (machine_id_ptr[0x30] >= 0 && machine_id_ptr[0x30] < 16 &&
                (((uint8_t *)(machine_id_ptr + 0x30))[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            if (machine_id_ptr[0x60] >= 0 && machine_id_ptr[0x60] < 16 &&
                (((uint8_t *)(machine_id_ptr + 0x60))[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            if (machine_id_ptr[0x90] >= 0 && machine_id_ptr[0x90] < 16 &&
                (((uint8_t *)(machine_id_ptr + 0x90))[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            machine_id_ptr = machine_id_ptr + 0xc0;
        }
        if (all_processed_or_invalid) {
            char has_client;

            has_client = (network_client != 0);
            server->state = 1;
            *field_9c4 = 0;
            server->session.map_loaded = has_client ? *((uint8_t *)network_client + 0xec0) : 0;
            // UNSURE: network_client+0xec0 == &network_client->session + 0x3ac,
            // i.e. network_client->session.unknown_3ac
        }
        if (*field_9c4 == 0) {
            large_integer counter;

            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            *field_9c4 = (int32_t)((counter.quad_part * 1000) / performance_frequency);
        }
    }

    entry = server->session.players;
    remaining = 16;
    for (;;) {
        valid = network_player_entry_validate(entry); // blam-cc: EAX -> entry
        handled = 0;
        if (valid != 0) {
            scan = server->session.players;
            i = 0;
            do {
                if (scan->machine_index == entry->machine_index &&
                    scan->machine_player_index == entry->machine_player_index) {
                    if ((int16_t)entry->machine_index == machine->machine_id) {
                        if ((server->flags >> 2 & 1) == 0) {
                            ok = network_player_join_finalize();
                        } else {
                            ok = 0;
                            if (network_player_entry_validate(entry) != 0 && server->state == 1) {
                                // UNSURE: EAX at this second call site was not re-derived;
                                // entry is the only live candidate.
                                ok = network_channel_key_open();
                                if (ok != 0) {
                                    player_data_iterator_advance((uint8_t)entry->slot_index) /* FIXED 2026-09-29: the original reads +0x1f, not +0x1e */;
                                    datum_new_at_index_with_salt();
                                    datum_new_at_index_with_salt();
                                    player_update_queue_create();
                                }
                            }
                        }
                        if (ok != 0) {
                            machine->player_joined = 1;
                            player_datum = player_data_iterator_advance((uint8_t)entry->slot_index) /* FIXED 2026-09-29: the original reads +0x1f, not +0x1e */;
                            game_engine_player_new_life(player_datum);
                            if (game_engine_player_profile_cache_find() != -1) {
                                handled = 1;
                            } else {
                                // find or add a free player_profile_cache slot
                                for (j = 0; j < 16; j = j + 1) {
                                    profile = &player_profile_cache[j];
                                    if (profile->in_use == 0) {
                                        profile->in_use = 1;
                                        profile->player = (datum_index)player_datum;
                                        player_profile_cache_count = player_profile_cache_count + 1;
                                        break;
                                    }
                                }
                                handled = 1;
                            }
                        }
                    }
                    break;
                }
                i = i + 1;
                scan = scan + 1;
            } while (i < 16);
        }
        if (handled) {
            network_game_server_handoff_object_ownership(object_count_passthrough, server, machine);
            network_object_release_ownership_claim(bl_passthrough);
        }
        entry = entry + 1;
        remaining = remaining - 1;
        if (remaining == 0) {
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4dfc90):

void FUN_004dfc90(int param_1,int param_2)

{
  undefined1 uVar1;
  char cVar2;
  char cVar3;
  short *psVar4;
  undefined4 uVar5;
  char *pcVar6;
  int iVar7;
  int iVar8;
  bool bVar9;
  undefined8 uVar10;
  LARGE_INTEGER local_8;

  *(ushort *)(param_2 + 0xe) = *(ushort *)(param_2 + 0xe) | 4;
  iVar7 = DAT_0071c2d8;
  bVar9 = true;
  if (*(short *)(param_1 + 4) != 1) {
    psVar4 = (short *)(param_1 + 0x3c4);
    iVar8 = 4;
    do {
      if (((-1 < *psVar4) && (*psVar4 < 0x10)) && ((*(byte *)(psVar4 + 1) & 4) == 0)) {
        bVar9 = false;
      }
      if (((-1 < psVar4[0x30]) && (psVar4[0x30] < 0x10)) && ((*(byte *)(psVar4 + 0x31) & 4) == 0)) {
        bVar9 = false;
      }
      if (((-1 < psVar4[0x60]) && (psVar4[0x60] < 0x10)) && ((*(byte *)(psVar4 + 0x61) & 4) == 0)) {
        bVar9 = false;
      }
      if (((-1 < psVar4[0x90]) && (psVar4[0x90] < 0x10)) && ((*(byte *)(psVar4 + 0x91) & 4) == 0)) {
        bVar9 = false;
      }
      psVar4 = psVar4 + 0xc0;
      iVar8 = iVar8 + -1;
    } while (iVar8 != 0);
    if (bVar9) {
      bVar9 = DAT_0071c2d8 == 0;
      *(undefined2 *)(param_1 + 4) = 1;
      *(undefined4 *)(param_1 + 0x9c4) = 0;
      if (bVar9) {
        uVar1 = 0;
      }
      else {
        uVar1 = *(undefined1 *)(iVar7 + 0xec0);
      }
      *(undefined1 *)(param_1 + 0x3b4) = uVar1;
    }
    if (*(int *)(param_1 + 0x9c4) == 0) {
      QueryPerformanceCounter(&local_8);
      uVar10 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
      uVar5 = __alldiv(uVar10,DAT_006ac8f8,DAT_006ac8fc);
      *(undefined4 *)(param_1 + 0x9c4) = uVar5;
    }
  }
  iVar7 = param_1 + 0x1aa;
  local_8.s.LowPart = 0x10;
LAB_004dfdb0:
  cVar2 = FUN_004de9f0();
  if (cVar2 != '\0') {
    iVar8 = 0;
    pcVar6 = (char *)(param_1 + 0x1c7);
    do {
      if ((pcVar6[-1] == *(char *)(iVar7 + 0x1c)) && (*pcVar6 == *(char *)(iVar7 + 0x1d))) {
        if ((short)*(char *)(iVar7 + 0x1c) == *(short *)(param_2 + 0xc)) {
          if ((*(byte *)(param_1 + 6) >> 2 & 1) == 0) {
            cVar2 = network_player_join_finalize();
          }
          else {
            cVar2 = '\0';
            cVar3 = FUN_004de9f0();
            if (((cVar3 != '\0') && (*(short *)(param_1 + 4) == 1)) &&
               (cVar2 = FUN_004de870(), cVar2 != '\0')) {
              FUN_004d98f0((int)*(char *)(iVar7 + 0x1f));
              datum_new_at_index_with_salt();
              datum_new_at_index_with_salt();
              FUN_00479f40();
            }
          }
          if (cVar2 != '\0') {
            *(undefined1 *)(param_2 + 0x50) = 1;
            uVar5 = FUN_004d98f0((int)*(char *)(iVar7 + 0x1f));
            FUN_0045c440(uVar5);
            iVar8 = FUN_00466e80();
            if (iVar8 != -1) goto LAB_004dfedb;
            iVar8 = 0;
            pcVar6 = (char *)&DAT_006b0b88;
            goto LAB_004dfeb0;
          }
        }
        break;
      }
      iVar8 = iVar8 + 1;
      pcVar6 = pcVar6 + 0x20;
    } while (iVar8 < 0x10);
  }
  goto LAB_004dfef9;
  while( true ) {
    pcVar6 = pcVar6 + 0x30;
    iVar8 = iVar8 + 1;
    if (0x6b0e87 < (int)pcVar6) break;
LAB_004dfeb0:
    if (*pcVar6 == '\0') {
      *(undefined1 *)(&DAT_006b0b88 + iVar8 * 0xc) = 1;
      (&DAT_006b0b8c)[iVar8 * 0xc] = uVar5;
      DAT_006f1d34 = DAT_006f1d34 + 1;
      break;
    }
  }
LAB_004dfedb:
  FUN_004dfa10(param_1,param_2);
  FUN_004dfc10();
LAB_004dfef9:
  iVar7 = iVar7 + 0x20;
  local_8.s.LowPart = local_8.s.LowPart - 1;
  if (local_8.s.LowPart == 0) {
    return;
  }
  goto LAB_004dfdb0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
