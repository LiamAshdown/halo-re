// network_player_entry_add  (Ghidra: FUN_004de4e0; named per this rewrite)
// address 0x4de4e0, size 259 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: types/networking.h's own comment on network_player_entry cites this address
// directly: "network_player_entry (0x4de4e0 add, 0x4de5f0 update, 0x4de640 remove, 0x4de900
// find, 0x4de9f0 validate, 0x4df790 colour assignment)". Every offset (session.player_count at
// +0x1a0, session.maximum_players at +0x19d, players[] at +0x1a2 stride 0x20 with
// machine_index/machine_player_index at +0x1c/+0x1d and slot_index at +0x1f) matches
// types/networking.h's network_game_session and network_player_entry exactly. `in_EAX + 7`
// (in_EAX is `undefined4 *`, so this scales to byte +0x1c) and `(int)in_EAX + 0x1d` are the
// incoming record's own machine_index/machine_player_index, validated to be in 0..15 / ==0
// (matching the header's "machine_player_index always 0 on PC build" note) before the lookup.
// The original's unrolled 4-way duplicate-key scan (slots i..i+3 per pass) only distinguishes
// "some slot matches" from "none matches", so the single per-slot loop below is equivalent.
// register convention: session in param_1 (stack), incoming record in EAX (in_EAX). blam-cc:
// EAX -> incoming, stack -> session

// VERIFIED against disassembly 0x4de4e0..0x4de5e3 (2026-09-30): range checks, unrolled duplicate scan (== plain per-slot scan), validate(EAX), free-row search on slot_index==-1, preferred incoming slot, 8-dword copy, player_count++; only AL (1) is returned
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, this batch

// blam-cc: EAX -> incoming
// Looks up an existing player row by (machine_index, machine_player_index); if none matches,
// validates the incoming record (network_player_entry_validate), finds a free row (preferring the incoming
// record's own slot_index if it names an empty row), copies the 32-byte record in, and bumps
// player_count. Returns 1 on success, 0 on failure (only AL is defined; the upper bytes of EAX
// are leftovers, not a packed slot index).
uint32_t network_player_entry_add(network_game_session *session, network_player_entry *incoming)
{
    int8_t machine_index;
    int8_t machine_player_index;
    int32_t i;
    int32_t free_index;
    int8_t incoming_slot;
    uint32_t *src;
    uint32_t *dst;
    int32_t k;

    if (session->player_count >= session->maximum_players) {
        return 0;
    }
    machine_index = incoming->machine_index;
    machine_player_index = incoming->machine_player_index;
    if (machine_index < 0 || machine_index >= 0x10 ||
        machine_player_index < 0 || machine_player_index >= 1) {
        return 0;
    }

    for (i = 0; i < 0x10; i++) {
        if (session->players[i].machine_index == machine_index &&
            session->players[i].machine_player_index == machine_player_index) {
            break; // already present -- matches an existing row, nothing to add
        }
    }
    if (i == 0x10 && network_player_entry_validate(incoming) != 0) {
        free_index = -1;
        for (k = 0; k < 0x10; k++) {
            if (session->players[k].slot_index == -1) {
                free_index = k;
                break;
            }
        }
        incoming_slot = incoming->slot_index;
        if (incoming_slot != -1 && free_index != incoming_slot) {
            free_index = incoming_slot;
        }
        if (free_index != -1) {
            incoming->slot_index = (int8_t)free_index;
            dst = (uint32_t *)&session->players[free_index];
            src = (uint32_t *)incoming;
            for (k = 0; k < 8; k++) {
                dst[k] = src[k];
            }
            session->player_count = session->player_count + 1;
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4de4e0):

uint FUN_004de4e0(int param_1)

{
  char cVar1;
  char cVar2;
  undefined4 *in_EAX;
  uint uVar3;
  char *pcVar4;
  int iVar5;
  char *pcVar6;
  char *pcVar7;
  undefined4 *puVar8;

  uVar3 = (uint)in_EAX & 0xffffff00;
  if ((((*(short *)(param_1 + 0x1a0) < (short)*(char *)(param_1 + 0x19d)) &&
       (cVar1 = *(char *)(in_EAX + 7), -1 < cVar1)) && (cVar1 < '\x10')) &&
     ((cVar2 = *(char *)((int)in_EAX + 0x1d), -1 < cVar2 && (cVar2 < '\x01')))) {
    iVar5 = 0;
    pcVar4 = (char *)(param_1 + 0x1bf);
    do {
      if ((pcVar4[-1] == cVar1) && (*pcVar4 == cVar2)) break;
      if ((pcVar4[0x1f] == cVar1) && (pcVar4[0x20] == cVar2)) {
        iVar5 = iVar5 + 1;
        break;
      }
      if ((pcVar4[0x3f] == cVar1) && (pcVar4[0x40] == cVar2)) {
        iVar5 = iVar5 + 2;
        break;
      }
      if ((pcVar4[0x5f] == cVar1) && (pcVar4[0x60] == cVar2)) {
        iVar5 = iVar5 + 3;
        break;
      }
      iVar5 = iVar5 + 4;
      pcVar4 = pcVar4 + 0x80;
    } while (iVar5 < 0x10);
    if ((iVar5 == 0x10) && (pcVar4 = (char *)network_player_entry_validate(), (char)pcVar4 != '\0')) {
      pcVar6 = (char *)0x0;
      pcVar7 = (char *)(param_1 + 0x1c1);
      do {
        pcVar4 = pcVar6;
        if (*pcVar7 == -1) break;
        pcVar6 = pcVar6 + 1;
        pcVar7 = pcVar7 + 0x20;
        pcVar4 = (char *)0xffffffff;
      } while ((int)pcVar6 < 0x10);
      cVar1 = *(char *)((int)in_EAX + 0x1f);
      if ((cVar1 != -1) && (pcVar4 != (char *)(int)cVar1)) {
        pcVar4 = (char *)(int)cVar1;
      }
      if (pcVar4 != (char *)0xffffffff) {
        *(char *)((int)in_EAX + 0x1f) = (char)pcVar4;
        pcVar7 = (char *)((int)pcVar4 * 0x20 + 0x1a2 + param_1);
        for (iVar5 = 8; iVar5 != 0; iVar5 = iVar5 + -1) {
          *pcVar7 = *puVar8;
        }
        *(short *)(param_1 + 0x1a0) = *(short *)(param_1 + 0x1a0) + 1;
        return CONCAT31((int3)((uint)((int)pcVar4 * 0x20) >> 8),1);
      }
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
