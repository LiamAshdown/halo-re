// network_game_settings_packet_receive  (Ghidra: FUN_004d9800; renamed, no prior name)
// address 0x4d9800, size 240 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Processes an incoming game-settings/
// map-data packet from the host, storing it locally and applying it the first time it is
// received"). `request+0x68` (dword-indexed, byte 0x1a0) matches network_game_session::
// player_count exactly; `request+0x21` (byte 0x84) matches network_game_session::server_name's
// own relative offset within the session, confirming the comparison is against
// client->session.server_name (byte 0xb98 = client+0xb14+0x84); the whole-session backup/replace
// copies exactly 0xec dwords, matching sizeof(network_game_session)/4 (0x3b0/4 = 236).
// register convention: the client pointer arrives in EBX (unaff_EBX, plain byte-offset this
// time, unlike the word-scaled EBX seen in network_game_settings_packet_send.c);
// `request` is the sole cdecl stack parameter. // blam-cc: EBX -> client, stack -> request
// UNSURE: the final saved/restored dword (`local_c`, the 236th of the 236 backed-up dwords)
// covers session's last dword, i.e. unknown_3ac plus its three padding bytes -- preserved as a
// raw dword save/restore rather than split into named sub-fields, to keep the "whole session
// minus this one field" semantics obvious.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void main_queue_map_change_by_name_or_clear(void); // 0x4c87a0
extern int32_t join_ui_state; // 0x00718f8c
extern int32_t interface_loading_screen_request_id; // 0x0068e688, UNSURE name; see network_game_settings_packet_send.c
extern char network_game_settings_ack_send(uint8_t *client, int16_t template_row); // 0x4d9f50
    // UNSURE: called with no visible arguments at this call site; template_row's value cannot
    // be recovered from this function's own decompiled body, so 0 is passed (row 0, matching
    // network_game_settings_packet_send.c's own unindexed use of the same template table).

// blam-cc: EBX -> client, stack -> request
int32_t network_game_settings_packet_receive(network_client_globals *client, const uint32_t *request)
{
    const uint8_t *pa, *pb;
    uint8_t a, b;
    int32_t cmp;
    uint32_t saved_last_dword;
    int32_t i;

    if (*(int16_t *)(request + 0x68) < 0 || *(int16_t *)(request + 0x68) > 0x10) {
        return 0;
    }

    pa = (const uint8_t *)(request + 0x21);
    pb = (const uint8_t *)client->session.server_name;
    while (1) {
        a = *pa;
        b = *pb;
        if (a != b) {
            cmp = (1 - (uint32_t)(a < b)) - (uint32_t)((a < b) != 0);
            goto compare_done;
        }
        if (a == 0) {
            cmp = 0;
            break;
        }
        a = pa[1];
        b = pb[1];
        if (a != b) {
            cmp = (1 - (uint32_t)(a < b)) - (uint32_t)((a < b) != 0);
            goto compare_done;
        }
        pa = pa + 2;
        pb = pb + 2;
        if (a == 0) {
            cmp = 0;
            break;
        }
    }
compare_done:
    if (cmp != 0) {
        main_queue_map_change_by_name_or_clear();
        if (join_ui_state != 1) {
            if (join_ui_state != 2 && join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
            }
            join_ui_state = 8;
        }
    }

    saved_last_dword = ((const uint32_t *)&client->session)[235];
    for (i = 0; i < 236; i = i + 1) {
        ((uint32_t *)&client->session)[i] = request[i];
    }
    *(uint32_t *)&client->session.map_loaded = saved_last_dword;

    if (*(uint8_t *)&client->pad_ee2 == 0) {
        network_game_settings_ack_send((uint8_t *)client, 0); // UNSURE argument; see declaration above
        *(uint8_t *)&client->pad_ee2 = 1;
        return 1;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4d9800):

undefined4 FUN_004d9800(undefined4 *param_1)

{
  byte bVar1;
  byte *pbVar2;
  int iVar3;
  undefined4 uVar4;
  int unaff_EBX;
  byte *pbVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  bool bVar8;
  undefined4 local_3b8 [235];
  undefined4 local_c;

  if ((*(short *)(param_1 + 0x68) < 0) || (0x10 < *(short *)(param_1 + 0x68))) {
    uVar4 = 0;
  }
  else {
    pbVar2 = (byte *)(param_1 + 0x21);
    pbVar5 = (byte *)(unaff_EBX + 0xb98);
    do {
      bVar1 = *pbVar2;
      bVar8 = bVar1 < *pbVar5;
      if (bVar1 != *pbVar5) {
LAB_004d9864:
        iVar3 = (1 - (uint)bVar8) - (uint)(bVar8 != 0);
        goto LAB_004d9869;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar2[1];
      bVar8 = bVar1 < pbVar5[1];
      if (bVar1 != pbVar5[1]) goto LAB_004d9864;
      pbVar2 = pbVar2 + 2;
      pbVar5 = pbVar5 + 2;
    } while (bVar1 != 0);
    iVar3 = 0;
LAB_004d9869:
    if (iVar3 != 0) {
      main_queue_map_change_by_name_or_clear();
      if (DAT_00718f8c != 1) {
        if ((DAT_00718f8c != 2) && (DAT_00718f8c == 4)) {
          DAT_0068e688 = 0xffffffff;
        }
        DAT_00718f8c = 8;
      }
    }
    puVar6 = (undefined4 *)(unaff_EBX + 0xb14);
    puVar7 = local_3b8;
    for (iVar3 = 0xec; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar7 = puVar7 + 1;
    }
    puVar6 = (undefined4 *)(unaff_EBX + 0xb14);
    for (iVar3 = 0xec; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *param_1;
      param_1 = param_1 + 1;
      puVar6 = puVar6 + 1;
    }
    *(undefined4 *)(unaff_EBX + 0xec0) = local_c;
    uVar4 = 1;
    if (*(char *)(unaff_EBX + 0xee2) == '\0') {
      FUN_004d9f50();
      *(undefined1 *)(unaff_EBX + 0xee2) = 1;
      return 1;
    }
  }
  return uVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
