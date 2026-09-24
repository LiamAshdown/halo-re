// network_object_record_last_sender  (Ghidra: FUN_004df900, unnamed)
// address 0x4df900, size 75 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Records the current sender (unaff_ESI)
// into the per-machine last-sender table for the machine resolved from param_1, when that
// machine is a real network machine and the object is flagged accordingly." `param_1 + 0x3b4`
// is exactly network_server_globals::session (offset 0x008) + network_game_session's
// unknown_3ac (0x3ac), i.e. server + 0x3b4. The write target
// `(index & 0xffff) * 0x200 + 0xd0 + player_data->data` matches types/game.h's player
// (stride 0x200) field unknown_d0 at offset 0xd0.
// register convention: ESI = sender (int32_t datum/object handle), stack = server
// (network_server_globals *).
// blam-cc: ESI -> sender, stack -> server
// UNSURE: FUN_004d98f0's real argument is not visible at this call site (it is called with
// no operands here, unlike its other two call sites in this batch which pass an explicit
// byte); transcribed as a no-argument call per Ghidra, matching that function's own
// low-confidence (0.3) summary.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_array *player_data; // 0x0087a480, stride 0x200 (game module)
extern uint32_t player_data_iterator_advance(void); // 0x4d98f0, other module;
    // UNSURE: real parameter is register-passed and not visible at this call site

// When the session flag at server->session.unknown_3ac is set, records `sender` as the last
// object to update the resolved player's record (player->unknown_d0), provided the resolve
// succeeded, the resolved index is non-zero, and sender is a valid handle.
uint32_t network_object_record_last_sender(int32_t sender, network_server_globals *server)
{
    uint32_t resolved;

    resolved = player_data_iterator_advance();
    if (resolved == 0xffffffff) {
        return 0;
    }
    if (server->session.unknown_3ac != 0 && resolved != 0 && sender != -1) {
        *(int32_t *)((uint8_t *)player_data->data + (resolved & 0xffff) * 0x200 + 0xd0) = sender;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4df900):

undefined4 FUN_004df900(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  int unaff_ESI;

  uVar1 = FUN_004d98f0();
  if (uVar1 == 0xffffffff) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
    if (((*(char *)(param_1 + 0x3b4) != '\0') && (uVar1 != 0)) && (unaff_ESI != -1)) {
      *(int *)((uVar1 & 0xffff) * 0x200 + 0xd0 + *(int *)(DAT_0087a480 + 0x34)) = unaff_ESI;
      return uVar2;
    }
  }
  return uVar2;
}
#endif
