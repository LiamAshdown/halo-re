// network_client_begin_connect  (Ghidra: FUN_004dc8d0; named per this rewrite)
// address 0x4dc8d0, size 210 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Begins a new outgoing connection attempt:
// allocates/reuses the client connection object, hands off to chimera__on_connect with the
// requested name/options, and marks the network session active on success." Confirmed via
// objdump against network_game_client_connect_to_address.c's call site (`lea ecx,[esp+0xc]`
// right before `call 0x4dc8d0`) that the elided ECX argument is the caller's local
// s_network_address; this function's own body reads it as `in_ECX->ipv4 != 0` and
// `in_ECX->port != 0` before proceeding, which is exactly chimera__on_connect's own
// `target_address` parameter passed straight through.
// register/parameter convention: blam-cc: ECX -> target_address, stack -> player_name
// UNSURE: `*(uint *)(client + 0xf4c) = (uint)local_1048` writes one byte past the end of
// network_client_globals (documented size 0x0f4c); local_1048 is itself never assigned in the
// decompiled body (it sits immediately after the oversized DAT_00712dd8 copy destination, and
// reads as compiler/stack-frame scaffolding around the large local frame's __chkstk probe, not
// hand-written Blam data). Preserved as a raw out-of-struct write of 0 (the closest stand-in for
// an otherwise-uninitialized value) rather than silently dropped, per the no-invented-behaviour
// rule, but this is the single largest source of doubt in this file's rewrite confidence.
// UNSURE: the local scratch this function builds for chimera__on_connect's `session_info`
// argument is, byte for byte, [2 unused bytes][8-wide-char player name][forced NUL][the leading
// bytes of a 0x7ff-dword copy from profile_globals_block] -- same "oversized copy
// into an undersized local, kept verbatim" situation server_browser_open.c already documents for
// the same global.
// UNSURE: network_debug_fill_canary_buffer (0x4e0790, outside this batch's range) is called with no visible
// arguments at its only call site; left as a bare call.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_client_globals *network_client;      // 0x0071c2d8
extern uint8_t network_host_handoff_requested;       // 0x0071c2de
extern int16_t network_game_mode;                    // 0x00719720
extern uint8_t profile_globals_block[0x1ffc]; // 0x00712dd8, established name (see
    // network_game_settings_packet_send.c / server_browser_open.c)

extern network_client_globals *network_session_create(void); // 0x4d8a80, outside this batch
extern void network_debug_fill_canary_buffer(void); // 0x4e0790, outside this batch's range, elided args unresolved
extern int8_t chimera__on_connect(const uint32_t *target_address, network_client_globals *client,
    const uint32_t *session_info); // 0x4d8ed0, this module
extern uint32_t chat_close(void); // 0x4aa900, this module's leave-game path


// blam-cc: ECX -> target_address
uint32_t network_client_begin_connect(wchar_t *player_name, s_network_address *target_address)
{
    network_client_begin_connect_scratch scratch;
    uint32_t result;

    result = 0;
    if (network_client == 0) {
        network_client = network_session_create();
        if (network_client != 0) {
            network_host_handoff_requested = 0;
        }
    }
    memcpy(scratch.config_template, profile_globals_block,
        0x7ff * 4); // UNSURE: overruns config_template, see header
    *(uint32_t *)((uint8_t *)network_client + 0xf4c) = 0; // UNSURE: see header
    if (network_client->state == 0 && target_address->ipv4 != 0 && target_address->port != 0) {
        wcsncpy((wchar_t *)scratch.name, (const wchar_t *)player_name, 8);
        scratch.name_terminator = 0;
        network_debug_fill_canary_buffer();
        if (chimera__on_connect((const uint32_t *)target_address, network_client,
                (const uint32_t *)&scratch) != 0) {
            network_game_mode = 1;
            return 1;
        }
        network_host_handoff_requested = 1;
        result = chat_close() & 0xffffff00;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4dc8d0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_004dc8d0(wchar_t *param_1)

{
  short *psVar1;
  char cVar2;
  uint uVar3;
  int *in_ECX;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined1 local_202c [2];
  wchar_t local_202a [8];
  undefined2 local_201a;
  undefined4 local_2008 [1008];
  byte local_1048;
  undefined4 uStack_c;

  uVar3 = 0x202c;
  uStack_c = 0x4dc8e0;
  if (DAT_0071c2d8 == 0) {
    uVar3 = network_session_create();
    DAT_0071c2d8 = uVar3;
    if (uVar3 != 0) {
      DAT_0071c2de = 0;
    }
  }
  puVar5 = &DAT_00712dd8;
  puVar6 = local_2008;
  for (iVar4 = 0x7ff; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar6 = puVar6 + 1;
  }
  uVar3 = uVar3 & 0xffffff00;
  psVar1 = (short *)(DAT_0071c2d8 + 0xeda);
  *(uint *)(DAT_0071c2d8 + 0xf4c) = (uint)local_1048;
  if (((*psVar1 == 0) && (*in_ECX != 0)) && (*(short *)((int)in_ECX + 0x12) != 0)) {
    _wcsncpy(local_202a,param_1,8);
    local_201a = 0;
    FUN_004e0790();
    cVar2 = chimera__on_connect(DAT_0071c2d8,local_202c);
    if (cVar2 != '\0') {
      DAT_00719720 = 1;
      return 1;
    }
    DAT_0071c2de = 1;
    uVar3 = chat_close();
    uVar3 = uVar3 & 0xffffff00;
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
