// network_client_send_local_player_updates  (Ghidra: network_client_send_local_player_updates,
// already named)
// address 0x4e77e0, size 174 bytes
// name confidence: 0.7   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md; build_local_player_position_update.c /
// build_local_player_vehicle_update.c (this batch, the two builders this dispatches to, and the
// EBX-out_changed / ESI-plr register convention they document); src/game/player_unit_has_parent.c
// (this batch's sibling call to the same FUN_0056cd10, fixing player_unit_has_parent's real
// signature -- FUN_00477210 here is that same function, called with the iterator's own datum
// handle).
// register convention: none beyond the stack-recognized machine_id; this function itself takes no
// parameters visible in the decompile.
// The on-stack object built from DAT_0087a480 (player_data) and the XOR-with-'iter' constant is
// the inline data_iterator constructor (types/memory.h, signature at +0x0c); DAT_006894a1's exact meaning
// (gates which builder to call) is not otherwise established.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>

extern data_array *player_data; // 0x0087a480
extern uint8_t network_client_vehicle_ack_enabled; // 0x006894a1
extern void *machine_table; // 0x00687558 (via player_unit_has_parent's own
    // callee chain; not read directly here)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern uint8_t player_unit_has_parent(datum_index player_handle); // this batch, 0x477210
extern int32_t build_local_player_position_update(uint8_t *out_changed, player *plr); // this module, 0x4e81e0
extern int32_t build_local_player_vehicle_update(uint8_t *out_changed, player *plr); // this module, 0x4e82f0
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *data, int32_t bits,
    int32_t reliable, int32_t unknown_a, int32_t unknown_b, int32_t priority); // 0x4e1930

// Per-tick client routine: for every local player with a live unit, builds either a vehicle
// transform ack (if the player's unit has a parent and vehicle acks are enabled) or a plain
// position ack, and sends the result to machine 1 if anything was encoded.
void network_client_send_local_player_updates(void)
{
    data_iterator iter;
    player *candidate;
    uint8_t out_changed;
    int32_t encoded_size;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    candidate = (player *)data_iterator_next(&iter);
    while (candidate != 0) {
        if (candidate->local_player_index == -1 && candidate->unit != (datum_index)-1) {
            if (player_unit_has_parent(iter.index) == 0 || network_client_vehicle_ack_enabled == 0) {
                encoded_size = build_local_player_position_update(&out_changed, candidate);
            } else {
                encoded_size = build_local_player_vehicle_update(&out_changed, candidate);
            }
            if (0 < encoded_size) {
                network_session_send_to_machine(1, 0, encoded_size, 0, 0, 0, 0);
            }
        }
        candidate = (player *)data_iterator_next(&iter);
    }
}

#if 0
Original Ghidra decompilation (0x4e77e0), from tools/pack.py 0x4e77e0:

void network_client_send_local_player_updates(void)

{
  char cVar1;
  int iVar2;
  undefined1 local_11;
  uint local_10;
  undefined2 local_c;
  undefined4 local_8;
  uint local_4;

  local_10 = DAT_0087a480;
  local_4 = DAT_0087a480 ^ 0x69746572;
  local_c = 0;
  local_8 = 0xffffffff;
  iVar2 = data_iterator_next();
  while (iVar2 != 0) {
    if ((*(short *)(iVar2 + 2) == -1) && (*(int *)(iVar2 + 0x34) != -1)) {
      cVar1 = FUN_00477210();
      if ((cVar1 == '\0') || (DAT_006894a1 == '\0')) {
        iVar2 = build_local_player_position_update();
      }
      else {
        iVar2 = build_local_player_vehicle_update(&local_11);
      }
      if (0 < iVar2) {
        network_session_send_to_machine(1,&DAT_00871de0,iVar2,0,0,0,0);
      }
    }
    iVar2 = data_iterator_next();
  }
  return;
}
#endif
