// network_client_state_dispatch  (Ghidra: FUN_004d8bb0; renamed, no prior name)
// address 0x4d8bb0, size 66 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md summary ("Dispatches per-frame processing to the
// handler matching the connection's current mode/type field"); the sole caller (network_client_update_dispatch,
// out/halo_decompiled.c around line 143483) loads DAT_0071c2d8 (network_client, per
// types/networking.h) into EAX before the call and reads network_client+0xedc right after it
// returns, confirming in_EAX is network_client here.
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// RESOLVED in the review pass: the switch reads network_client+0xeda as a 16-bit selector with
// five valid values (0..4). types/networking.h declared that field as `pad_eda` (assumed
// padding); this function is the direct evidence that it is a live state value, and the header
// now declares it as `state` with the network_client_state enum.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

// The five per-state handlers. 0x4d8bb0's own dispatch table shows how each is reached:
//   case 0  mov eax,esi / jmp 0x4daa20   -> client in EAX, tail call
//   case 1  mov eax,esi / jmp 0x4dab80   -> client in EAX, tail call
//   case 2  call 0x4daef0                -> client still in ESI
//   case 3  push esi / call 0x4daf80     -> client on the stack
//   case 4  mov eax,esi / jmp 0x4db100   -> client in EAX, tail call
extern char network_join_handshake_tick(network_client_globals *client);        // 0x4daa20
extern char network_join_connect_retry_tick(network_client_globals *client);    // 0x4dab80


int8_t network_client_state_dispatch(network_client_globals *client) // blam-cc: EAX -> client
{
    int8_t result;

    result = 0;
    switch (client->state) {
    case 0:
        result = network_join_handshake_tick(client);
        return result;
    case 1:
        result = network_join_connect_retry_tick(client);
        return result;
    case 2:
        result = network_host_lobby_tick(client);
        break;
    case 3:
        result = network_game_client_update(client);
        return result;
    case 4:
        result = network_host_channel_service_tick(client);
        return result;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4d8bb0):

undefined1 FUN_004d8bb0(void)

{
  undefined1 uVar1;
  int in_EAX;

  uVar1 = 0;
  switch(*(undefined2 *)(in_EAX + 0xeda)) {
  case 0:
    uVar1 = FUN_004daa20();
    return uVar1;
  case 1:
    uVar1 = FUN_004dab80();
    return uVar1;
  case 2:
    uVar1 = FUN_004daef0();
    break;
  case 3:
    uVar1 = network_game_client_update(in_EAX);
    return uVar1;
  case 4:
    uVar1 = FUN_004db100();
    return uVar1;
  }
  return uVar1;
}
#endif
