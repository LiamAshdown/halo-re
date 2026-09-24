// network_game_action_apply  (Ghidra: FUN_004da320; renamed, no prior name)
// address 0x4da320, size 787 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Applies a single queued network-game
// action by type id, calling the specific per-type handler (ammo pickups, player/vehicle
// network updates, etc.)"). Tracing the control flow directly (rather than relying on the
// truncated summary text) shows: when network_game_mode == 1 (client) the large ~50-case switch
// runs; when network_game_mode == 2 (host) only a 7-case subset (ids 6, 0xb, 0xf, 0x1a, 0x21,
// 0x22, 0x35) runs, using gotos into the large switch's own case bodies for the ids they share;
// any other mode is a no-op.
// register convention: the queued action entry arrives in EAX (in_EAX), a pointer whose
// dereference's dword at +4 is the type id. // blam-cc: EAX -> action_entry
// UNSURE: every one of the ~45 per-type handler functions is called with literally zero visible
// arguments in Ghidra's output; none are in this task's address range (they span the ai,
// objects, items, player-update and message-delta modules), and none of their own files were
// available to cross-check a real register convention against, so they are declared and called
// exactly as Ghidra shows -- void, no arguments -- rather than guessed. This is one blanket note
// covering all of them, instead of repeating it 45 times.
// UNSURE: `DAT_0071c2c0` (set to 1 around every handler call, 0 after) is not declared anywhere
// in types/networking.h; named generically below as a re-entrancy/"applying" guard flag.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int16_t network_game_mode; // 0x00719720
extern uint8_t network_action_apply_active; // 0x0071c2c0, UNSURE name

// UNSURE: all zero-argument, per the file header note above.
extern void FUN_00456ad0(void);
extern void FUN_0045f8f0(void);
extern void FUN_004609d0(void);
extern void FUN_00466d00(void);
extern void FUN_00466e60(void);
extern void FUN_00467230(void);
extern void FUN_00468320(void);
extern void FUN_0046bca0(void);
extern void FUN_00470a10(void);
extern void FUN_004778c0(void);
extern void FUN_00477c70(void);
extern void FUN_00478f10(void);
extern void FUN_00479b40(void);
extern void FUN_004aaf70(void);
extern void FUN_004ae200(void);
extern void FUN_004bbe20(void);
extern void FUN_004bdb40(void);
extern void FUN_004bf1c0(void);
extern void FUN_004c0ca0(void);
extern void item_add_ammunition(void);
extern void FUN_004c3530(void);
extern void FUN_004c3870(void);
extern void FUN_004c4ac0(void);
extern void FUN_004c5c10(void);
extern void network_player_ping_field_update_and_report(void *decode_context);
    // blam-cc: EAX -> decode_context; this module, 0x4dbaa0
extern void network_channel_key_send_state(void);
extern void network_client_handle_server_text_message(void);
extern void player_update_client_local_player_update_from_network(void);
extern void player_update_client_local_player_vehicle_update_from_network(void);
extern void player_update_client_remote_player_action_update_from_network(void);
extern void FUN_004e5720(void);
extern void player_update_client_remote_player_total_biped_update_from_network(void);
extern void player_update_client_remote_player_total_vehicle_update_from_network(void);
extern void player_update_client_remote_player_position_delta_from_network(void);
extern void player_update_client_remote_player_vehicle_position_delta_from_network(void);
extern void message_delta_parameters_protocol_receive_update(void);
extern void FUN_004ec390(void);
extern void object_apply_shield_charge_and_notify(void);
extern void object_apply_linked_impulse(void);
extern void object_type_override_call_0x70_release_node(void);
extern void object_delete_by_pooled_node_id(void);
extern void FUN_0055b110(void);
extern void FUN_00566c90(void);
extern void FUN_0056c400(void);
extern void FUN_0056ddb0(void);
extern void FUN_00572110(void);

// blam-cc: EAX -> action_entry
void network_game_action_apply(int32_t **action_entry)
{
    int32_t type_id = (*action_entry)[1];

    if (network_game_mode != 1) {
        if (network_game_mode != 2) {
            return;
        }
        switch (type_id) {
        case 6:
            goto case_6;
        default:
        host_default:
            network_action_apply_active = 0;
            return;
        case 0xb:
        case_b:
            network_action_apply_active = 1;
            FUN_00456ad0();
            network_action_apply_active = 0;
            return;
        case 0xf:
        case_f:
            network_action_apply_active = 1;
            FUN_004aaf70();
            network_action_apply_active = 0;
            return;
        case 0x1a:
        case_1a:
            network_action_apply_active = 1;
            FUN_00470a10();
            network_action_apply_active = 0;
            return;
        case 0x21:
        case_21:
            network_action_apply_active = 1;
            network_channel_key_send_state();
            network_action_apply_active = 0;
            return;
        case 0x22:
        case_22:
            network_action_apply_active = 1;
            message_delta_parameters_protocol_receive_update();
            FUN_004ec390();
            network_action_apply_active = 0;
            return;
        case 0x35:
        case_35:
            network_action_apply_active = 1;
            network_player_ping_field_update_and_report(action_entry);
            network_action_apply_active = 0;
            return;
        }
    }

    network_action_apply_active = 1;
    switch (type_id) {
    case 0:
        object_delete_by_pooled_node_id();
        network_action_apply_active = 0;
        return;
    case 1:
    case 2:
    case 3:
        object_type_override_call_0x70_release_node();
        network_action_apply_active = 0;
        return;
    case 4:
        object_type_override_call_0x70_release_node();
        network_action_apply_active = 0;
        return;
    case 5:
        object_type_override_call_0x70_release_node();
        network_action_apply_active = 0;
        return;
    case 6:
    case_6:
        network_action_apply_active = 1;
        FUN_004ae200();
        network_action_apply_active = 0;
        return;
    case 7:
        FUN_004778c0();
        network_action_apply_active = 0;
        return;
    case 8:
        FUN_00477c70();
        network_action_apply_active = 0;
        return;
    case 9:
        FUN_0056c400();
        network_action_apply_active = 0;
        return;
    case 10:
        FUN_00478f10();
        network_action_apply_active = 0;
        return;
    case 0xb:
        goto case_b;
    case 0xc:
        FUN_00566c90();
        network_action_apply_active = 0;
        return;
    default:
        goto host_default;
    case 0xe:
        FUN_00479b40();
        network_action_apply_active = 0;
        return;
    case 0xf:
        goto case_f;
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
        FUN_00466e60();
        network_action_apply_active = 0;
        return;
    case 0x15:
        FUN_00466d00();
        network_action_apply_active = 0;
        return;
    case 0x16:
        FUN_00467230();
        network_action_apply_active = 0;
        return;
    case 0x17:
        FUN_00468320();
        network_action_apply_active = 0;
        return;
    case 0x18:
        FUN_004609d0();
        network_action_apply_active = 0;
        return;
    case 0x19:
        FUN_0046bca0();
        network_action_apply_active = 0;
        return;
    case 0x1a:
        goto case_1a;
    case 0x1b:
        FUN_0056ddb0();
        network_action_apply_active = 0;
        return;
    case 0x1c:
        FUN_00572110();
        network_action_apply_active = 0;
        return;
    case 0x1d:
        FUN_0055b110();
        network_action_apply_active = 0;
        return;
    case 0x1e:
        FUN_004c0ca0();
        network_action_apply_active = 0;
        return;
    case 0x1f:
        FUN_004bbe20();
        network_action_apply_active = 0;
        return;
    case 0x20:
        FUN_004c5c10();
        network_action_apply_active = 0;
        return;
    case 0x21:
        goto case_21;
    case 0x22:
        goto case_22;
    case 0x23:
        player_update_client_local_player_update_from_network();
        network_action_apply_active = 0;
        return;
    case 0x24:
        player_update_client_local_player_vehicle_update_from_network();
        network_action_apply_active = 0;
        return;
    case 0x25:
        player_update_client_remote_player_action_update_from_network();
        network_action_apply_active = 0;
        return;
    case 0x26:
        FUN_004e5720();
        network_action_apply_active = 0;
        return;
    case 0x27:
        player_update_client_remote_player_position_delta_from_network();
        network_action_apply_active = 0;
        return;
    case 0x28:
        player_update_client_remote_player_vehicle_position_delta_from_network();
        network_action_apply_active = 0;
        return;
    case 0x29:
        player_update_client_remote_player_total_biped_update_from_network();
        network_action_apply_active = 0;
        return;
    case 0x2a:
        player_update_client_remote_player_total_vehicle_update_from_network();
        network_action_apply_active = 0;
        return;
    case 0x2b:
        FUN_004c3530();
        network_action_apply_active = 0;
        return;
    case 0x2c:
        item_add_ammunition();
        network_action_apply_active = 0;
        return;
    case 0x2d:
        FUN_004c3870();
        network_action_apply_active = 0;
        return;
    case 0x2e:
        FUN_004c4ac0();
        network_action_apply_active = 0;
        return;
    case 0x2f:
        FUN_0045f8f0();
        network_action_apply_active = 0;
        return;
    case 0x30:
        FUN_004bdb40();
        network_action_apply_active = 0;
        return;
    case 0x31:
        object_apply_linked_impulse();
        network_action_apply_active = 0;
        return;
    case 0x32:
        object_apply_shield_charge_and_notify();
        network_action_apply_active = 0;
        return;
    case 0x33:
        FUN_004bf1c0();
        network_action_apply_active = 0;
        return;
    case 0x35:
        goto case_35;
    case 0x37:
        network_client_handle_server_text_message();
        network_action_apply_active = 0;
        return;
    }
}

#if 0
Original Ghidra decompilation (0x4da320):

void FUN_004da320(void)

{
  int *in_EAX;

  if (DAT_00719720 != 1) {
    if (DAT_00719720 != 2) {
      return;
    }
    switch(*(undefined4 *)(*in_EAX + 4)) {
    case 6:
      goto switchD_004da34c_caseD_6;
    default:
LAB_004da631:
      DAT_0071c2c0 = 0;
      return;
    case 0xb:
switchD_004da34c_caseD_b:
      DAT_0071c2c0 = 1;
      FUN_00456ad0();
      DAT_0071c2c0 = 0;
      return;
    case 0xf:
switchD_004da34c_caseD_f:
      DAT_0071c2c0 = 1;
      FUN_004aaf70();
      DAT_0071c2c0 = 0;
      return;
    case 0x1a:
switchD_004da34c_caseD_1a:
      DAT_0071c2c0 = 1;
      FUN_00470a10();
      DAT_0071c2c0 = 0;
      return;
    case 0x21:
switchD_004da34c_caseD_21:
      DAT_0071c2c0 = 1;
      FUN_004de950();
      DAT_0071c2c0 = 0;
      return;
    case 0x22:
switchD_004da34c_caseD_22:
      DAT_0071c2c0 = 1;
      message_delta_parameters_protocol_receive_update();
      FUN_004ec390();
      DAT_0071c2c0 = 0;
      return;
    case 0x35:
switchD_004da34c_caseD_35:
      DAT_0071c2c0 = 1;
      FUN_004dbaa0();
      DAT_0071c2c0 = 0;
      return;
    }
  }
  DAT_0071c2c0 = 1;
  switch(*(undefined4 *)(*in_EAX + 4)) {
  case 0:
    object_delete_by_pooled_node_id();
    DAT_0071c2c0 = 0;
    return;
  case 1:
  case 2:
  case 3:
    object_type_override_call_0x70_release_node();
    DAT_0071c2c0 = 0;
    return;
  case 4:
    object_type_override_call_0x70_release_node();
    DAT_0071c2c0 = 0;
    return;
  case 5:
    object_type_override_call_0x70_release_node();
    DAT_0071c2c0 = 0;
    return;
  case 6:
switchD_004da34c_caseD_6:
    DAT_0071c2c0 = 1;
    FUN_004ae200();
    DAT_0071c2c0 = 0;
    return;
  case 7:
    FUN_004778c0();
    DAT_0071c2c0 = 0;
    return;
  case 8:
    FUN_00477c70();
    DAT_0071c2c0 = 0;
    return;
  case 9:
    FUN_0056c400();
    DAT_0071c2c0 = 0;
    return;
  case 10:
    FUN_00478f10();
    DAT_0071c2c0 = 0;
    return;
  case 0xb:
    goto switchD_004da34c_caseD_b;
  case 0xc:
    FUN_00566c90();
    DAT_0071c2c0 = 0;
    return;
  default:
    goto LAB_004da631;
  case 0xe:
    FUN_00479b40();
    DAT_0071c2c0 = 0;
    return;
  case 0xf:
    goto switchD_004da34c_caseD_f;
  case 0x10:
  case 0x11:
  case 0x12:
  case 0x13:
  case 0x14:
    FUN_00466e60();
    DAT_0071c2c0 = 0;
    return;
  case 0x15:
    FUN_00466d00();
    DAT_0071c2c0 = 0;
    return;
  case 0x16:
    FUN_00467230();
    DAT_0071c2c0 = 0;
    return;
  case 0x17:
    FUN_00468320();
    DAT_0071c2c0 = 0;
    return;
  case 0x18:
    FUN_004609d0();
    DAT_0071c2c0 = 0;
    return;
  case 0x19:
    FUN_0046bca0();
    DAT_0071c2c0 = 0;
    return;
  case 0x1a:
    goto switchD_004da34c_caseD_1a;
  case 0x1b:
    FUN_0056ddb0();
    DAT_0071c2c0 = 0;
    return;
  case 0x1c:
    FUN_00572110();
    DAT_0071c2c0 = 0;
    return;
  case 0x1d:
    FUN_0055b110();
    DAT_0071c2c0 = 0;
    return;
  case 0x1e:
    FUN_004c0ca0();
    DAT_0071c2c0 = 0;
    return;
  case 0x1f:
    FUN_004bbe20();
    DAT_0071c2c0 = 0;
    return;
  case 0x20:
    FUN_004c5c10();
    DAT_0071c2c0 = 0;
    return;
  case 0x21:
    goto switchD_004da34c_caseD_21;
  case 0x22:
    goto switchD_004da34c_caseD_22;
  case 0x23:
    player_update_client_local_player_update_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x24:
    player_update_client_local_player_vehicle_update_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x25:
    player_update_client_remote_player_action_update_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x26:
    FUN_004e5720();
    DAT_0071c2c0 = 0;
    return;
  case 0x27:
    player_update_client_remote_player_position_delta_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x28:
    player_update_client_remote_player_vehicle_position_delta_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x29:
    player_update_client_remote_player_total_biped_update_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x2a:
    player_update_client_remote_player_total_vehicle_update_from_network();
    DAT_0071c2c0 = 0;
    return;
  case 0x2b:
    FUN_004c3530();
    DAT_0071c2c0 = 0;
    return;
  case 0x2c:
    item_add_ammunition();
    DAT_0071c2c0 = 0;
    return;
  case 0x2d:
    FUN_004c3870();
    DAT_0071c2c0 = 0;
    return;
  case 0x2e:
    FUN_004c4ac0();
    DAT_0071c2c0 = 0;
    return;
  case 0x2f:
    FUN_0045f8f0();
    DAT_0071c2c0 = 0;
    return;
  case 0x30:
    FUN_004bdb40();
    DAT_0071c2c0 = 0;
    return;
  case 0x31:
    object_apply_linked_impulse();
    DAT_0071c2c0 = 0;
    return;
  case 0x32:
    object_apply_shield_charge_and_notify();
    DAT_0071c2c0 = 0;
    return;
  case 0x33:
    FUN_004bf1c0();
    DAT_0071c2c0 = 0;
    return;
  case 0x35:
    goto switchD_004da34c_caseD_35;
  case 0x37:
    network_client_handle_server_text_message();
    DAT_0071c2c0 = 0;
    return;
  }
}
#endif
