// game_engine_sync_variant_defaults  (Ghidra: FUN_0045fc80; named per
// out/phase4/game_functions.md)
// address 0x45fc80, size 145 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Synchronizes the active game variant's default option
// block into the local cache and, if hosting, into the network session state"); types/game.h
// game_variant (0x98 bytes, 0x26 dwords), globals 0x0087aa80 (game_engine_pending_variant),
// 0x0087ab20 (game_engine_active_variant), 0x0071c2d4 (network_server, variant at +0x10c,
// game_engine_index at +0x13c).
// UNSURE: DAT_0087aab0 (the cached engine index compared against the session's own) and the four
// globals reset at the very end (0x00719754/0x0071973c/0x00719738/0x0071974f) are not attributed
// to this module anywhere in this batch's evidence; kept as raw externs with generic names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"

extern game_variant game_engine_pending_variant; // 0x0087aa80
extern game_variant game_engine_active_variant;  // 0x0087ab20
extern int32_t cached_network_engine_index;      // 0x0087aab0, UNSURE identity
extern network_server_globals *network_server;
extern network_client_globals *network_client;

extern uint16_t unknown_00719754; // UNSURE identity/owning module
extern uint8_t unknown_0071973c;  // UNSURE identity/owning module; written as a BYTE
                                  //   (objdump 0x45fcff "mov ds:0x71973c,al")
extern uint8_t unknown_00719738;  // UNSURE identity/owning module; written as a BYTE
                                  //   (objdump 0x45fd04 "mov BYTE PTR ds:0x719738,0x1")
extern uint8_t unknown_0071974f;  // UNSURE identity/owning module

extern void main_queue_map_change_by_name_or_clear(void); // 0x4c87a0
extern void network_game_broadcast_player_set_changed(void *session); // 0x4e1bf0, not in this batch

// Copies the pending game variant into the active/local copy, and, while hosting a session whose
// cached engine index disagrees, also pushes it into the network session's own variant field and
// notifies the network layer. When neither a session nor a client is active, resets a small block
// of otherwise-unattributed globals to their idle defaults.
void game_engine_sync_variant_defaults(void)
{
    void *session;
    uint8_t hosting;

    main_queue_map_change_by_name_or_clear();

    session = network_server;
    hosting = (session != 0);

    game_engine_active_variant = game_engine_pending_variant;

    if (hosting && *(int32_t *)((uint8_t *)session + 0x13c) != cached_network_engine_index) {
        *(game_variant *)((uint8_t *)session + 0x10c) = game_engine_pending_variant;
        network_game_broadcast_player_set_changed(session);
        session = network_server;
    }

    if (network_client == 0 && session == 0) {
        unknown_00719754 = 0xffff;
        unknown_0071973c = 0;
        unknown_00719738 = 1;
        unknown_0071974f = 0;
    }
}

#if 0
Original Ghidra decompilation (0x45fc80), from tools/pack.py 0x45fc80:

/* WARNING: Removing unreachable block (ram,0x0045fc9e) */
/* WARNING: Removing unreachable block (ram,0x0045fca0) */

void FUN_0045fc80(void)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  bool bVar5;

  main_queue_map_change_by_name_or_clear();
  iVar1 = DAT_0071c2d4;
  bVar5 = DAT_0071c2d4 != 0;
  puVar3 = &DAT_0087aa80;
  puVar4 = &DAT_0087ab20;
  for (iVar2 = 0x26; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar3;
    puVar3 = puVar3 + 1;
    puVar4 = puVar4 + 1;
  }
  if ((bVar5) && (*(int *)(iVar1 + 0x13c) != DAT_0087aab0)) {
    puVar3 = &DAT_0087aa80;
    puVar4 = (undefined4 *)(iVar1 + 0x10c);
    for (iVar2 = 0x26; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    FUN_004e1bf0(iVar1);
    iVar1 = DAT_0071c2d4;
  }
  if ((DAT_0071c2d8 == 0) && (iVar1 == 0)) {
    DAT_00719754._0_2_ = 0xffff;
    DAT_0071973c = 0;
    DAT_00719738 = 1;
    DAT_0071974f = 0;
  }
  return;
}
#endif
