// chimera__on_connect  (Ghidra: chimera__on_connect, already named)
// address 0x4d8ed0, size 380 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Begins connecting to a server after
// the user accepts a join/invite, posting the 'Connecting' status text and copying the target
// address into the connection object"); shares the client+0xec4/+0xae0../+0xadc/+0xab4../+0xeda
// offsets with network_connection_initiate.c (0x4d8cf0, same task batch), which independently
// confirms client+0xab4 is an s_network_address and names client+0xec4 unknown_ec4.
// register convention: the target address (six dwords: s_network_address + one extra dword)
// arrives in EBX (unaff_EBX); client and the nine-dword session-info payload are ordinary cdecl
// stack parameters. // blam-cc: EBX -> target_address; stack -> client, session_info
// UNSURE: `network_address_to_string`'s first call (at function entry, before anything else
// runs) has no visible argument; reconstructed as formatting `target_address` itself, the only
// address in scope yet, for a "Connecting to ..." status line. The second call matches
// network_connection_initiate.c's own reconstruction (the connection's own about-to-be-replaced
// endpoint address).
// UNSURE: client->state is declared as padding in types/networking.h but is used here (and in
// network_client_state_dispatch.c, network_client_connect_progress_percent.c and
// network_connection_initiate.c) as a live connection-mode discriminant; not renamed, per the
// task's rule against editing that header.
// UNSURE: DAT_00718f90 and DAT_00718f8c are not declared anywhere in types/networking.h; named
// generically below (interface_loading_screen_progress / join_ui_state) from their being reset together right
// after the "Connecting" status text is posted, but neither name is independently confirmed.
// UNSURE: network_connection_endpoint_set (network_connection_endpoint_set) is called with no visible argument at
// its call site; reconstructed as (target_address, client) since target_address is exactly what
// was just copied into client's endpoint fields a few lines above.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, owned by the timing/system module
extern int16_t network_join_error_code; // 0x00718fa4, the pending join/disconnect error
                                        // string index; -1 means none. WORD-sized everywhere
                                        // (cmp/mov WORD PTR ds:0x718fa4), consumed and reset by
                                        // the main-menu display_error call at 0x4a9ff0.
extern int32_t interface_loading_screen_progress; // 0x00718f90, UNSURE name; see file header
extern int32_t join_ui_state;     // 0x00718f8c, UNSURE name; see file header
extern char *network_address_to_string(s_network_address *addr); // 0x440570
extern int16_t network_channel_attempt_connect(int32_t a, int32_t b); // 0x441f60
extern void console_printf_verbose(const char *text); // 0x496a80, UNSURE signature: posts a UI status string


extern int32_t network_connection_endpoint_set(const uint32_t *source, network_client_globals *connection); // 0x4d8c50


// blam-cc: EBX -> target_address; stack -> client, session_info
int8_t chimera__on_connect(const uint32_t *target_address, network_client_globals *client,
                            const uint32_t *session_info)
{
    network_connection_attempt_state *attempt;
    network_connection_endpoint *endpoint;
    large_integer counter;
    int32_t started_ms;
    network_channel *endpoint_probe;
    int16_t registration_result;
    int32_t i;

    network_address_to_string((s_network_address *)target_address); // UNSURE argument
    client->unknown_ec4 = 1;
    attempt = &client->connect_attempt;
    attempt->unknown_00 = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    started_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

    attempt->elapsed_counter = 0;
    attempt->started_ms = started_ms;
    attempt->unknown_0c = 0;
    for (i = 0; i < 9; i = i + 1) {
        attempt->session_info[i] = session_info[i];
    }

    endpoint_probe = client->channel;
    if (endpoint_probe->endpoint != 0) {
        network_address_to_string(&(&client->connection)->address); // UNSURE argument
        registration_result = network_channel_attempt_connect(0x96640, 1);
        if (registration_result != 0) {
            goto retry_limit_check;
        }
    }
    if (endpoint_probe->endpoint != 0) {
        client->state = 1; // UNSURE: live connection-mode value, not padding
        attempt->elapsed_counter = 0;
        console_printf_verbose("Connecting");

        endpoint = &client->connection;
        endpoint->address.ipv4 = 0;
        endpoint->address.ipv6_1 = 0;
        endpoint->address.ipv6_2 = 0;
        endpoint->address.ipv6_3 = 0;
        *(uint32_t *)&endpoint->address.size = 0;
        endpoint->unknown_14 = 0;
        endpoint->last_send_ms = 0;
        endpoint->message_count = 0;
        endpoint->retry_count = 0;
        endpoint->unknown_20 = 0;
        endpoint->ready = 0;
        endpoint->unknown_23 = 0;
        endpoint->control_block = 0;

        endpoint->address.ipv4 = target_address[0];
        endpoint->address.ipv6_1 = target_address[1];
        endpoint->address.ipv6_2 = target_address[2];
        endpoint->address.ipv6_3 = target_address[3];
        *(uint32_t *)&endpoint->address.size = target_address[4];
        endpoint->unknown_14 = target_address[5];

        interface_loading_screen_progress = 0;
        join_ui_state = 5;
        if (endpoint->address.size == k_network_address_size_ipv4) {
            network_connection_endpoint_set(target_address, client); // UNSURE argument
        }
        return 1;
    }
retry_limit_check:
    if (network_join_error_code == -1) {
        network_join_error_code = 7;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4d8ed0):

undefined1 chimera__on_connect(int param_1,undefined4 *param_2)

{
  short sVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 *unaff_EBX;
  undefined4 *puVar4;
  undefined8 uVar5;
  LARGE_INTEGER local_8;

  network_address_to_string();
  *(undefined4 *)(param_1 + 0xec4) = 1;
  *(undefined4 *)(param_1 + 0xae0) = 0;
  QueryPerformanceCounter(&local_8);
  uVar5 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar2 = __alldiv(uVar5,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(param_1 + 0xae8) = 0;
  *(undefined4 *)(param_1 + 0xae4) = uVar2;
  *(undefined1 *)(param_1 + 0xaec) = 0;
  puVar4 = (undefined4 *)(param_1 + 0xaee);
  for (iVar3 = 9; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = *param_2;
    param_2 = param_2 + 1;
    puVar4 = puVar4 + 1;
  }
  iVar3 = **(int **)(param_1 + 0xadc);
  if ((iVar3 != 0) && (iVar3 != 0)) {
    network_address_to_string();
    sVar1 = FUN_00441f60(0x96640,1);
    if (sVar1 != 0) goto LAB_004d8f8f;
  }
  if (iVar3 != 0) {
    *(undefined2 *)(param_1 + 0xeda) = 1;
    *(undefined4 *)(param_1 + 0xae8) = 0;
    FUN_00496a80("Connecting");
    *(undefined4 *)(param_1 + 0xab4) = 0;
    *(undefined4 *)(param_1 + 0xab8) = 0;
    *(undefined4 *)(param_1 + 0xabc) = 0;
    *(undefined4 *)(param_1 + 0xac0) = 0;
    *(undefined4 *)(param_1 + 0xac4) = 0;
    *(undefined4 *)(param_1 + 0xac8) = 0;
    *(undefined4 *)(param_1 + 0xacc) = 0;
    *(undefined4 *)(param_1 + 0xad0) = 0;
    *(undefined4 *)(param_1 + 0xad4) = 0;
    *(undefined4 *)(param_1 + 0xad8) = 0;
    *(undefined4 *)(param_1 + 0xab4) = *unaff_EBX;
    *(undefined4 *)(param_1 + 0xab8) = unaff_EBX[1];
    *(undefined4 *)(param_1 + 0xabc) = unaff_EBX[2];
    *(undefined4 *)(param_1 + 0xac0) = unaff_EBX[3];
    *(undefined4 *)(param_1 + 0xac4) = unaff_EBX[4];
    *(undefined4 *)(param_1 + 0xac8) = unaff_EBX[5];
    DAT_00718f90 = 0;
    DAT_00718f8c = 5;
    if (*(short *)(unaff_EBX + 4) == 4) {
      FUN_004d8c50();
    }
    return 1;
  }
LAB_004d8f8f:
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 7;
    return 0;
  }
  return 0;
}
#endif
