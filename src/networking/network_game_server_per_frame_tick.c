// network_game_server_per_frame_tick  (Ghidra: FUN_004e03c0, unnamed)
// address 0x4e03c0, size 183 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Per-frame network tick: in client-processing
// state, drains the queued update packets (FUN_00472cc0) and applies the first matching
// channel's queued update via FUN_004df840/FUN_004e1b50; in the alternate[, ticks the game
// engine directly]." unaff_ESI+4 matches network_server_globals::unknown_004;
// unaff_ESI+0x3c4/+0x9f8 match ::machines[0].machine_id and ::unknown_9f8.
// register convention: CX = update_count (int16_t), ESI = server (network_server_globals *).
// blam-cc: CX -> update_count, ESI -> server
// The byte read at server+0x9f4 is pending_join_entry.machine_index (raw offset cast kept).
// `if (machine != 0)` mirrors the original's (always-true) test at 0x4e0448.

// VERIFIED against disassembly 0x4e03c0..0x4e0477 (2026-09-30): FIXED: no EAX argument (EAX is overwritten by movzx at 0x4e03c0); the finalize/broadcast entry is &server->pending_join_entry (esi+0x9d8, EAX for both callees); broadcast_state_snapshot(EAX = entry, stack = server) was called without args. State dispatch, update loop, machine search (stride 0x60, machine_id word vs sign-extended byte 0x9f4) and pending flag clear match
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void update_server_push_player_tick_history(void); // other module (UNSURE)
extern void game_engine_tick(void); // other module, already named
extern char network_game_session_finalize_and_add_player(network_player_entry *entry,
    network_server_globals *server, network_machine *machine); // 0x4df840, this batch
extern uint32_t network_game_broadcast_state_snapshot(const uint32_t *record, network_server_globals *server); // 0x4e1b50; blam-cc: EAX -> record, stack -> server

// While the server is in state 1 (client-processing), drains `update_count` queued update
// packets and, if a deferred "process this machine's queued update" request is pending
// (unknown_9f8), locates the matching machine by its saved id and finalizes/broadcasts its
// join. In state 2, just ticks the game engine directly.
void network_game_server_per_frame_tick(int16_t update_count, network_server_globals *server)
{
    if (server->state == 1) {
        if (update_count > 0) {
            uint32_t remaining;
            large_integer counter;

            remaining = (uint32_t)update_count;
            do {
                server->update_tick = server->update_tick + 1;
                update_server_push_player_tick_history();
                QueryPerformanceCounter((LARGE_INTEGER *)&counter);
                remaining = remaining - 1;
            } while (remaining != 0);
        }
        if (server->join_finalize_pending != 0) {
            int32_t i;
            int8_t saved_machine_id;
            network_machine *machine;

            saved_machine_id = *((int8_t *)server + 0x9f4); // pending_join_entry.machine_index
            i = 0;
            while (server->machines[i].machine_id != (int16_t)saved_machine_id) {
                i = i + 1;
                if (i > 0xf) {
                    server->join_finalize_pending = 0;
                    return;
                }
            }
            machine = &server->machines[i];
            if (machine != 0) {
                char ok;

                network_player_entry *entry = &server->pending_join_entry;

                ok = network_game_session_finalize_and_add_player(entry, server, machine);
                if (ok != 0) {
                    network_game_broadcast_state_snapshot((const uint32_t *)entry, server);
                }
            }
            server->join_finalize_pending = 0;
        }
    } else if (server->state == 2) {
        game_engine_tick();
    }
}

#if 0
Original Ghidra decompilation (0x4e03c0):

void FUN_004e03c0(void)

{
  char cVar1;
  int iVar2;
  ushort in_CX;
  short *psVar3;
  int unaff_ESI;
  uint uVar4;
  LARGE_INTEGER local_c;

  if (*(short *)(unaff_ESI + 4) == 1) {
    if (0 < (short)in_CX) {
      uVar4 = (uint)in_CX;
      do {
        *(int *)(unaff_ESI + 0x9b8) = *(int *)(unaff_ESI + 0x9b8) + 1;
        FUN_00472cc0();
        QueryPerformanceCounter(&local_c);
        uVar4 = uVar4 - 1;
      } while (uVar4 != 0);
    }
    if (*(char *)(unaff_ESI + 0x9f8) != '\0') {
      iVar2 = 0;
      psVar3 = (short *)(unaff_ESI + 0x3c4);
      while (*psVar3 != (short)*(char *)(unaff_ESI + 0x9f4)) {
        iVar2 = iVar2 + 1;
        psVar3 = psVar3 + 0x30;
        if (0xf < iVar2) {
          *(undefined1 *)(unaff_ESI + 0x9f8) = 0;
          return;
        }
      }
      if (iVar2 * 0x60 + 0x3b8 + unaff_ESI != 0) {
        cVar1 = FUN_004df840();
        if (cVar1 != '\0') {
          FUN_004e1b50();
        }
      }
      *(undefined1 *)(unaff_ESI + 0x9f8) = 0;
    }
  }
  else if (*(short *)(unaff_ESI + 4) == 2) {
    game_engine_tick();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
