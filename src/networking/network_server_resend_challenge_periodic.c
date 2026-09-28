// network_server_resend_challenge_periodic  (Ghidra: FUN_004e1450, unnamed)
// address 0x4e1450, size 142 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Resends a prepared challenge/info packet to a
// peer roughly every 5 seconds while a connection attempt is outstanding." server+0x9bc falls
// inside network_server_globals::unknown_9bc (no individual field name available).
// register convention: ESI = server (network_server_globals *).
// blam-cc: ESI -> server

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern void *network_prepare_challenge_packet(void); // 0x4deaf0, this module
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    // blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6;
    // this module, 0x4e19c0

// Every ~5000ms, rebroadcasts the prepared challenge/info packet to the whole session and
// updates the last-sent timestamp at server+0x9bc.
uint32_t network_server_resend_challenge_periodic(network_server_globals *server)
{
    large_integer counter;
    uint32_t now_ms;
    uint32_t *last_sent;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
    last_sent = (uint32_t *)((uint8_t *)server + 0x9bc);
    if (*last_sent + 5000u < now_ms) {
        void *packet;

        packet = network_prepare_challenge_packet();
        network_session_broadcast_to_all(server, 0, packet, 1, 0, 1, 3);
        *last_sent = now_ms;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e1450):

undefined4 FUN_004e1450(void)

{
  uint uVar1;
  int iVar2;
  int unaff_ESI;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar1 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  if (*(int *)(unaff_ESI + 0x9bc) + 5000U < uVar1) {
    iVar2 = network_prepare_challenge_packet();
    FUN_004e19c0(0,iVar2,1,0,1,3);
    *(uint *)(unaff_ESI + 0x9bc) = uVar1;
  }
  return 1;
}
#endif
