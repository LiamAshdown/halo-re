// network_game_server_per_frame_tick  (Ghidra: FUN_004e03c0, unnamed)
// address 0x4e03c0, size 183 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Per-frame network tick: in client-processing
// state, drains the queued update packets (FUN_00472cc0) and applies the first matching
// channel's queued update via FUN_004df840/FUN_004e1b50; in the alternate[, ticks the game
// engine directly]." unaff_ESI+4 matches network_server_globals::unknown_004;
// unaff_ESI+0x3c4/+0x9f8 match ::machines[0].machine_id and ::unknown_9f8.
// register convention: EAX = entry (network_player_entry *, forwarded only to
// network_game_session_finalize_and_add_player), CX = update_count (int16_t),
// ESI = server (network_server_globals *).
// blam-cc: EAX -> entry, CX -> update_count, ESI -> server
// UNSURE: EAX has no visible source in this function's own body; modelled as a pure
// pass-through parameter per the register convention, mirroring the same situation in
// network_game_session_finalize_and_add_player.c.
// UNSURE: the byte this function reads at server+0x9f4 falls inside
// network_server_globals::unknown_9bc (an unresolved 0x3c-byte span); accessed via an
// explicit offset cast rather than a named field.
// UNSURE: `if (machine_ptr != 0)` in the original is effectively always true once the search
// loop finds an index (the computed pointer cannot be NULL); preserved literally rather than
// simplified away.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void update_server_push_player_tick_history(void); // other module (UNSURE)
extern int32_t __stdcall QueryPerformanceCounter(large_integer *counter);
extern void game_engine_tick(void); // other module, already named
extern char network_game_session_finalize_and_add_player(network_player_entry *entry,
    network_server_globals *server, network_machine *machine); // 0x4df840, this batch
extern void network_game_broadcast_state_snapshot(void); // 0x4e1b50, this batch (UNSURE args)

// While the server is in state 1 (client-processing), drains `update_count` queued update
// packets and, if a deferred "process this machine's queued update" request is pending
// (unknown_9f8), locates the matching machine by its saved id and finalizes/broadcasts its
// join. In state 2, just ticks the game engine directly.
void network_game_server_per_frame_tick(network_player_entry *entry, int16_t update_count, network_server_globals *server)
{
    if (server->unknown_004 == 1) {
        if (update_count > 0) {
            uint32_t remaining;
            large_integer counter;

            remaining = (uint32_t)update_count;
            do {
                server->unknown_9b8 = server->unknown_9b8 + 1;
                update_server_push_player_tick_history();
                QueryPerformanceCounter(&counter);
                remaining = remaining - 1;
            } while (remaining != 0);
        }
        if (server->unknown_9f8 != 0) {
            int32_t i;
            int8_t saved_machine_id;
            network_machine *machine;

            saved_machine_id = *((int8_t *)server + 0x9f4); // UNSURE: within unknown_9bc
            i = 0;
            while (server->machines[i].machine_id != (int16_t)saved_machine_id) {
                i = i + 1;
                if (i > 0xf) {
                    server->unknown_9f8 = 0;
                    return;
                }
            }
            machine = &server->machines[i];
            if (machine != 0) {
                char ok;

                ok = network_game_session_finalize_and_add_player(entry, server, machine);
                if (ok != 0) {
                    network_game_broadcast_state_snapshot();
                }
            }
            server->unknown_9f8 = 0;
        }
    } else if (server->unknown_004 == 2) {
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
