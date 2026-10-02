// network_client_connection_handshake_tick  (Ghidra: FUN_004e0590, unnamed)
// address 0x4e0590, size 383 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "State machine that drives the client
// connection handshake/timeout: while connected, updates the disconnect-timeout timer per
// incoming message id, and while not yet connected, (re)starts the timeout ti[mer]." The four
// FUN_004debXX callees are types/networking.h's network_timer_pair helpers, already rewritten
// in this repo (network_timer_advance/_start/_increment_clamped/_decrement_floored).
// register convention: EAX = state (int16_t, a message/step id 0..3), ECX = owner (the object
// whose unknown_9bc-range bytes this function reads/writes).
// blam-cc: EAX -> state, ECX -> owner
// UNSURE (structural): every network_timer_pair call in the original (FUN_004debb0/_50/_d0/_f0)
// is made with no visible arguments even though their real signatures need a timer pointer
// (and, for two of them, extra int32 arguments). The only way the offsets this function itself
// touches (owner+0x9c8, +0x9d0, +0x9d4, +0x9d5, +0x9d6) stay self-consistent is if the timer
// object is `(network_timer_pair *)(owner + 0x9c8)` -- i.e. a compile-time-constant offset
// from `owner`, not a separately passed register -- so that is what is reconstructed here.
// UNSURE: `owner`'s real type is not confirmed; the touched offsets fall exactly inside
// network_server_globals::unknown_9bc (0x9bc..0x9f7), so it is typed that way here, even
// though the function's own summary describes client-side behaviour.
// UNSURE: FUN_00449210 and FUN_004ddd20 (distinct from the global DAT_0071c2dc) have no
// established signatures; called argument-less, matching Ghidra.
// UNSURE: the increment/decrement magnitudes passed to network_timer_increment_clamped and
// network_timer_decrement_floored at the two argument-less call sites are not recoverable
// here; passed as 0 with this note, matching this codebase's precedent for eliding arguments
// it cannot reconstruct (see network_channel_new.c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t network_disconnect_timeout_flag; // 0x0071c2dc
extern void network_timer_advance(network_timer_pair *timer); // 0x4deb50, this module
extern void network_timer_increment_clamped(network_timer_pair *timer, int32_t upper_bound, int32_t increment); // 0x4debb0
extern void network_timer_decrement_floored(network_timer_pair *timer, int32_t decrement); // 0x4debd0
extern void network_timer_start(network_timer_pair *timer, int32_t duration_ms); // 0x4debf0
extern char network_game_all_machines_have_player(network_server_globals *server); // 0x4e04f0, this batch
extern char network_game_any_team_empty(network_server_globals *server); // 0x4e0480, this batch
extern void time_query_performance_counter_ms(void); // other module (UNSURE)
extern char network_channel_short_disconnect_timeout(void); // other module (UNSURE)
extern int32_t network_server_count_connected_machines(network_server_globals *server); // 0x4e1880, this batch

void network_client_connection_handshake_tick(int16_t state, network_server_globals *owner)
{
    network_timer_pair *timer;
    uint8_t *base;

    base = (uint8_t *)owner;
    timer = (network_timer_pair *)(base + 0x9c8);

    if (*(uint8_t *)(base + 0x9d5) != 0) {
        return;
    }
    if (!((network_game_all_machines_have_player(owner) != 0 &&
           network_game_any_team_empty(owner) == 0) ||
          state == 2)) {
        return;
    }

    if (*(uint8_t *)(base + 0x9d4) == 1) {
        if (*(uint8_t *)(base + 0x9d6) != 0) {
            return;
        }
        switch (state) {
        case 0:
            *(uint8_t *)(base + 0x9d6) = 1;
            network_timer_increment_clamped(timer, 0, 0); // UNSURE: elided args
            return;
        case 1:
            *(uint8_t *)(base + 0x9d6) = 1;
            network_timer_advance(timer);
            if (timer->remaining_ms > 999) {
                network_timer_decrement_floored(timer, 0); // UNSURE: elided decrement
                network_timer_advance(timer);
                if (timer->remaining_ms < 999) {
                    network_timer_start(timer, 999);
                    return;
                }
            }
            break;
        case 2:
            *(uint8_t *)(base + 0x9d6) = 1;
            *(uint8_t *)(base + 0x9d4) = 0;
            return;
        case 3:
            *(uint8_t *)(base + 0x9d6) = 1;
            network_timer_start(timer, 0);
            return;
        default:
            break;
        }
    } else {
        char ready;
        int16_t connected_count;

        time_query_performance_counter_ms();
        if (state == 3) {
            network_timer_start(timer, 0);
            *(uint8_t *)(base + 0x9d4) = 1;
            *(uint8_t *)(base + 0x9d6) = 0;
            return;
        }
        if (network_disconnect_timeout_flag == 0 ||
            (connected_count = network_server_count_connected_machines(owner), connected_count > 0)) {
            ready = network_channel_short_disconnect_timeout();
            *(uint8_t *)(base + 0x9d4) = 1;
            network_timer_start(timer, ready != 0 ? 10999 : 30999);
            *(int32_t *)(base + 0x9d0) = 0;
            *(uint8_t *)(base + 0x9d6) = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4e0590):

void FUN_004e0590(void)

{
  char cVar1;
  short in_AX;
  short sVar2;
  int in_ECX;

  if ((*(char *)(in_ECX + 0x9d5) == '\0') &&
     (((cVar1 = FUN_004e04f0(), cVar1 != '\0' && (cVar1 = FUN_004e0480(), cVar1 == '\0')) ||
      (in_AX == 2)))) {
    if (*(char *)(in_ECX + 0x9d4) == '\x01') {
      if (*(char *)(in_ECX + 0x9d6) == '\0') {
        switch(in_AX) {
        case 0:
          *(undefined1 *)(in_ECX + 0x9d6) = 1;
          FUN_004debb0();
          return;
        case 1:
          *(undefined1 *)(in_ECX + 0x9d6) = 1;
          FUN_004deb50();
          if (999 < *(int *)(in_ECX + 0x9c8)) {
            FUN_004debd0();
            FUN_004deb50();
            if (*(int *)(in_ECX + 0x9c8) < 999) {
              FUN_004debf0(999);
              return;
            }
          }
          break;
        case 2:
          *(undefined1 *)(in_ECX + 0x9d6) = 1;
          *(undefined1 *)(in_ECX + 0x9d4) = 0;
          return;
        case 3:
          *(undefined1 *)(in_ECX + 0x9d6) = 1;
          FUN_004debf0(0);
          return;
        }
      }
    }
    else {
      FUN_00449210();
      if (in_AX == 3) {
        FUN_004debf0(0);
        *(undefined1 *)(in_ECX + 0x9d4) = 1;
        *(undefined1 *)(in_ECX + 0x9d6) = 0;
        return;
      }
      if ((DAT_0071c2dc == '\0') || (sVar2 = FUN_004e1880(), 0 < sVar2)) {
        cVar1 = FUN_004ddd20();
        *(undefined1 *)(in_ECX + 0x9d4) = 1;
        FUN_004debf0((-(uint)(cVar1 != '\0') & 0xffffb1e0) + 30999);
        *(undefined4 *)(in_ECX + 0x9d0) = 0;
        *(undefined1 *)(in_ECX + 0x9d6) = 0;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
