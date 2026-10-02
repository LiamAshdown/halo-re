// network_player_assign_random_color  (Ghidra: FUN_004df790; named per this rewrite)
// address 0x4df790, size 165 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: types/networking.h cites this address directly: "network_player_entry (... 0x4df790
// colour assignment)". out/phase4/networking_functions.md: "Randomly assigns a player colour
// index that isn't already in use by another active player, widening the candidate range after
// 10 failed attempts, and stores it at param_1+0x18." param_1+0x18 matches
// network_player_entry.color_index; the scanned array (container+0x1c2 == session.players[0]+
// 0x20 == session.players[1].color_index, stride 0x10 shorts) matches every active player's own
// color_index field.
// register convention: session container in EAX (in_EAX). blam-cc: EAX -> session, stack ->
// entry

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern random_seed effect_random_seed; // 0x00719cd4, types/math.h

extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, this batch

// blam-cc: EAX -> session
void network_player_assign_random_color(network_game_session *session, network_player_entry *entry)
{
    int32_t attempt;
    uint32_t seed;
    int16_t candidate;
    int32_t in_use;
    int32_t i;

    attempt = 0;
    seed = effect_random_seed;
    for (;;) {
        seed = seed * 0x19660d + 0x3c6ef35f;
        if (attempt < 10) {
            candidate = (int16_t)((int32_t)(seed >> 0x10) * 3 >> 0x10);
        } else {
            candidate = (int16_t)((int32_t)(seed >> 0x10) * 0x11 >> 0x10);
        }
        in_use = 0;
        effect_random_seed = seed;
        for (i = 0; i < 0x10; i++) {
            if (network_player_entry_validate(&session->players[i]) != 0 &&
                session->players[i].color_index == candidate) {
                in_use = 1;
                break;
            }
        }
        attempt = attempt + 1;
        if (!in_use) {
            entry->color_index = candidate;
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4df790):

void FUN_004df790(int param_1)

{
  bool bVar1;
  char cVar2;
  int in_EAX;
  short sVar3;
  uint uVar4;
  short *psVar5;
  int iVar6;
  int local_c;

  local_c = 0;
  uVar4 = DAT_00719cd4;
  do {
    uVar4 = uVar4 * 0x19660d + 0x3c6ef35f;
    if (local_c < 10) {
      sVar3 = (short)((uVar4 >> 0x10) * 3 >> 0x10);
    }
    else {
      sVar3 = (short)((uVar4 >> 0x10) * 0x11 >> 0x10);
    }
    bVar1 = true;
    iVar6 = 0;
    psVar5 = (short *)(in_EAX + 0x1c2);
    DAT_00719cd4 = uVar4;
    do {
      cVar2 = FUN_004de9f0();
      if ((cVar2 != '\0') && (*psVar5 == sVar3)) {
        bVar1 = false;
        break;
      }
      iVar6 = iVar6 + 1;
      psVar5 = psVar5 + 0x10;
    } while (iVar6 < 0x10);
    local_c = local_c + 1;
    if (bVar1) {
      *(short *)(param_1 + 0x18) = sVar3;
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
