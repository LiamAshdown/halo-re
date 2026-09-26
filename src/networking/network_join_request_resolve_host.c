// network_join_request_resolve_host  (Ghidra: FUN_004ba320; named per
// out/phase2/results/networking_01.json / symbols/review_queue.txt)
// address 0x4ba320, size 819 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Resolves the host/IP for a pending
// 'join server' request and either connects immediately or kicks off an asynchronous hostname
// resolution"); out/phase2/results/networking_01.json evidence ("reads a pending connect
// request (DAT_00719450), retrieves hostname/IP fields via FUN_006175e0/00617640/00617650,
// compares packed address bytes, formats \"%s:%d\" via sprintf, and either connects directly
// via network_game_client_connect_to_address or starts an async resolve with FUN_00614f30 passing callbacks FUN_0044ad80
// and network_join_hostname_resolved_callback (0x4ba270), setting the connect state
// DAT_00718f8c=4"); the packed-address comparison mirrors
// src/networking/network_channel_get_remote_address.c's identical gamespy_array_length-based idiom;
// DAT_0071946c is named `master_server_query_engine` per
// src/networking/master_server_process_pending_requests.c (same address, same GameSpy handle
// role, reused here); DAT_0068e680/4/8, DAT_00718f8c/90 and DAT_006b2f28/68 are the
// interface module's "loading screen" globals per out/phase2/results/interface_01.json
// ("resets every loading-screen related global ... to their inactive sentinel values").
// register convention: Ghidra recognizes no parameters at all (`signature: undefined
// FUN_004ba320(void)`, no in_/unaff_ registers in the body) -- this function reads everything
// from globals.
// UNSURE: disassembly of this function's `call 0x4dc790` (network_game_client_connect_to_
// address) shows `lea eax,[esp+...]` loading the address of the freshly sprintf'd "host:port"
// string immediately before the call, with only the `&network_join_target_address` stack
// argument otherwise visible -- i.e. the callee also takes an EAX argument. The one other call
// site in this batch's range (network_join_hostname_resolved_callback, 0x4ba270, already
// committed) shows the identical `lea eax,[esp+...]; push <stack arg>; call 0x4dc790` shape,
// but that file's own extern declares only the stack parameter. This file declares the fuller,
// two-argument prototype directly (each function file's extern declarations are independent,
// per this project's one-file-per-function model), rather than editing the already-committed
// file; out of scope for this batch, but worth a follow-up pass over 0x4ba270's declaration.
// UNSURE: the three `local_5c` byte-copy loops (inlined strcpy of FUN_006175e0/00617640/
// 00617040's results) write into a local buffer that is never read again anywhere in the
// function -- apparently dead, but preserved for fidelity exactly as
// network_join_hostname_resolved_callback.c preserves its own similarly dead local buffer.
// UNSURE: network_channels_open (0x441300, already committed) is declared `void` there because
// none of its own callers used a return value; this function's Ghidra decompile treats its
// result as a real `uint` fed into `return uVar5 & 0xffffff00`, but that value is genuinely
// just whatever network_channels_open's own last callee happened to leave in EAX (confirmed by
// disassembly: no explicit return-value write before its final `ret`). Preserved as `0` in the
// two return paths that depend on it, since the already-committed `void` signature leaves no
// value to read.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern void *server_browser_join_target; // 0x00719450, a GameSpy peer/query handle;
    // matches server_browser_join_target_has_password / network_join_target_address at the adjacent
    // 0x00719454 / 0x00719458 (network_join_hostname_resolved_callback.c, this module)
extern void *master_server_query_engine; // 0x0071946c, GameSpy query engine object handle,
    // per src/networking/master_server_process_pending_requests.c
extern uint16_t network_join_target_address[128]; // 0x00719458, this module
extern uint8_t server_browser_join_target_has_password;   // 0x00719454, this module

extern int32_t network_game_socket; // 0x006f14c4, this module
extern uint8_t network_channels_open_ok; // 0x006869be, this module (network_channels_open.c)

// Foreign interface-module "loading screen" globals; see file header evidence.
extern int32_t interface_loading_screen_address_a;   // 0x0068e680
extern int32_t interface_loading_screen_address_b;   // 0x0068e684
extern int32_t interface_loading_screen_request_id;  // 0x0068e688
extern int32_t interface_loading_screen_ui_state;    // 0x00718f8c
extern int32_t interface_loading_screen_progress;    // 0x00718f90
extern int32_t interface_loading_screen_text_buffer; // 0x006b2f28
extern int32_t interface_loading_screen_unknown_6b2f68; // 0x006b2f68

// Foreign GameSpy library accessors against server_browser_join_target (see
// out/phase4/networking_types_notes.md: GameSpy SDK objects are deliberately not typed here).
extern uint32_t FUN_00617600(int32_t handle); // port
extern char *FUN_006175e0(int32_t handle);    // hostname/address string
extern uint32_t FUN_00617650(int32_t handle); // alternate/"connect" port
extern char *FUN_00617640(int32_t handle);    // alternate/"connect" address string
extern int32_t FUN_00617620(int32_t handle);  // has-resolved-address flag
extern int32_t FUN_00617630(int32_t handle);  // hostname-already-resolved flag
extern uint32_t gamespy_array_length(int32_t object); // 0x6175f0: returns the uint32 at object+0x00 // address byte source, same idiom as
    // network_channel_get_remote_address.c's identical-named foreign accessor
// Foreign GameSpy library accessors against master_server_query_engine.
extern char *FUN_00617040(void *handle);      // hostname/address string
extern uint32_t FUN_00617060(void *handle);   // address byte source

extern void network_channels_open(void); // 0x441300, this module (declared void: see UNSURE)
extern int32_t network_random_offset(int32_t base); // 0x4403b0, this module
extern void FUN_00616f50(void *handle, char *hostname, uint32_t port, int32_t request_id); // foreign, GameSpy library
extern void network_join_hostname_resolved_callback(int32_t resolve_failed, uint32_t unused,
    uint8_t *hostent); // 0x4ba270, this module
extern void function_do_nothing(void); // 0x44ad80
extern int32_t FUN_00614f30(int32_t hostname, int32_t request_id, int32_t one,
    void (*progress_callback)(void), void (*complete_callback)(int32_t, uint32_t, uint8_t *),
    int32_t zero); // foreign, GameSpy library async hostname resolve
// blam-cc: EAX -> address_string (0x4dc790 strcpy's it out of EAX), stack -> the wide
// hostname/password string. Both call sites in this module and the one at 0x4c86ea agree.
extern uint32_t network_game_client_connect_to_address(char *address_string,
                                                        uint16_t *target_string); // 0x4dc790

uint32_t network_join_request_resolve_host(void)
{
    char dead_scratch[28]; // see file header UNSURE note; never read back
    char host_buffer[0x18 + 1];
    char connect_string[64];
    uint8_t needs_async_resolve = 0;
    char *string_result;
    uint32_t port = 0;

    string_result = FUN_006175e0((int32_t)(uintptr_t)server_browser_join_target);
    if (string_result != 0) {
        strcpy(dead_scratch, string_result);
    }
    FUN_00617650((int32_t)(uintptr_t)server_browser_join_target);
    string_result = FUN_00617640((int32_t)(uintptr_t)server_browser_join_target);
    if (string_result != 0) {
        strcpy(dead_scratch, string_result);
    }
    string_result = FUN_00617040(master_server_query_engine);
    if (string_result != 0) {
        strcpy(dead_scratch, string_result);
    }

    {
        uint32_t byte_a; int32_t byte_b; uint32_t byte_c;
        int32_t byte_d; uint32_t byte_e; uint32_t byte_f;
        int32_t has_resolved_address;

        port = FUN_00617600((int32_t)(uintptr_t)server_browser_join_target);
        byte_a = gamespy_array_length((int32_t)(uintptr_t)server_browser_join_target);
        gamespy_array_length((int32_t)(uintptr_t)server_browser_join_target); // discarded, kept for fidelity
        byte_b = gamespy_array_length((int32_t)(uintptr_t)server_browser_join_target);
        byte_c = gamespy_array_length((int32_t)(uintptr_t)server_browser_join_target);
        FUN_00617060(master_server_query_engine); // discarded, kept for fidelity
        byte_d = FUN_00617060(master_server_query_engine);
        byte_e = FUN_00617060(master_server_query_engine);
        byte_f = FUN_00617060(master_server_query_engine);
        has_resolved_address = FUN_00617620((int32_t)(uintptr_t)server_browser_join_target);

        if (has_resolved_address == 0 ||
            (((byte_a & 0xff0000) >> 8 | (uint32_t)((byte_b << 0x10) | (byte_c & 0xff00)) << 8) !=
             ((uint32_t)((byte_d << 0x10) | (byte_e & 0xff00)) << 8 | ((byte_f >> 8) & 0xff00)))) {
            if (FUN_00617630((int32_t)(uintptr_t)server_browser_join_target) == 0) {
                needs_async_resolve = 1;
            } else {
                strncpy(host_buffer, FUN_006175e0((int32_t)(uintptr_t)server_browser_join_target), 0x18);
                host_buffer[0x18] = 0;
            }
        } else {
            strncpy(host_buffer, FUN_00617640((int32_t)(uintptr_t)server_browser_join_target), 0x18);
            host_buffer[0x18] = 0;
            port = FUN_00617650((int32_t)(uintptr_t)server_browser_join_target);
        }
    }

    network_channels_open();
    if (network_channels_open_ok == 0) {
        server_browser_join_target = 0;
        return 0; // see file header UNSURE note
    }

    if (!needs_async_resolve) {
        interface_loading_screen_address_a = -1;
        interface_loading_screen_address_b = -1;
        interface_loading_screen_request_id = -1;
        interface_loading_screen_ui_state = 0;
        interface_loading_screen_progress = 0;
        interface_loading_screen_text_buffer = 0;
        interface_loading_screen_unknown_6b2f68 = 0;

        sprintf(connect_string, "%s:%d", host_buffer, port & 0xffff);
        {
            uint32_t result = network_game_client_connect_to_address(connect_string,
                                                                      network_join_target_address);
            network_join_target_address[0] = 0;
            server_browser_join_target_has_password = 0;
            server_browser_join_target = 0;
            return result;
        }
    }

    {
        uint32_t resolve_handle = gamespy_array_length(network_game_socket); // see file header: same
            // accessor used elsewhere for address bytes, here against the game socket itself
        char *hostname = FUN_006175e0((int32_t)(uintptr_t)server_browser_join_target);
        uint32_t resolve_port = FUN_00617600((int32_t)(uintptr_t)server_browser_join_target);
        int32_t request_id = network_random_offset(10000);
        uint32_t result;

        FUN_00616f50(master_server_query_engine, hostname, resolve_port, request_id);
        result = FUN_00614f30((int32_t)resolve_handle, request_id, 1, function_do_nothing,
            network_join_hostname_resolved_callback, 0);
        server_browser_join_target = 0;
        if (result == 0) {
            interface_loading_screen_request_id = request_id;
            interface_loading_screen_address_a = -1;
            interface_loading_screen_address_b = -1;
            interface_loading_screen_progress = 0;
            interface_loading_screen_text_buffer = 0;
            interface_loading_screen_unknown_6b2f68 = 0;
            interface_loading_screen_ui_state = 4;
            return 0xffffff01;
        }
        return result & 0xffffff00;
    }
}

#if 0
Original Ghidra decompilation (0x4ba320):

uint FUN_004ba320(void)

{
  char cVar1;
  bool bVar2;
  char *pcVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  int iVar11;
  undefined4 uVar12;
  undefined4 uVar13;
  undefined4 uVar14;
  undefined4 uVar15;
  size_t sVar16;
  char local_78 [21];
  uint local_63;
  char local_5c [28];
  char local_40 [64];

  local_78[1] = '\0';
  local_78[2] = '\0';
  local_78[3] = '\0';
  local_78[4] = '\0';
  local_78[5] = '\0';
  local_78[6] = '\0';
  local_78[7] = '\0';
  local_78[8] = '\0';
  local_78[9] = '\0';
  local_78[10] = '\0';
  local_78[0xb] = '\0';
  local_78[0xc] = '\0';
  local_78[0xd] = '\0';
  local_78[0xe] = '\0';
  local_78[0xf] = '\0';
  local_78[0x10] = '\0';
  local_78[0x11] = '\0';
  local_78[0x12] = '\0';
  local_78[0x13] = '\0';
  local_78[0x14] = '\0';
  local_78[0] = '\0';
  local_63 = 0;
  bVar2 = false;
  FUN_00617600(DAT_00719450);
  pcVar3 = (char *)FUN_006175e0(DAT_00719450);
  if (pcVar3 != (char *)0x0) {
    iVar6 = -(int)pcVar3;
    do {
      cVar1 = *pcVar3;
      pcVar3[(int)(local_5c + iVar6)] = cVar1;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
  }
  FUN_00617650(DAT_00719450);
  pcVar3 = (char *)FUN_00617640(DAT_00719450);
  if (pcVar3 != (char *)0x0) {
    iVar6 = -(int)pcVar3;
    do {
      cVar1 = *pcVar3;
      pcVar3[(int)(local_5c + iVar6)] = cVar1;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
  }
  pcVar3 = (char *)FUN_00617040(DAT_0071946c);
  if (pcVar3 != (char *)0x0) {
    iVar6 = -(int)pcVar3;
    do {
      cVar1 = *pcVar3;
      pcVar3[(int)(local_5c + iVar6)] = cVar1;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
  }
  uVar4 = FUN_00617600(DAT_00719450);
  uVar5 = FUN_006175f0(DAT_00719450);
  FUN_006175f0(DAT_00719450);
  iVar6 = FUN_006175f0(DAT_00719450);
  uVar7 = FUN_006175f0(DAT_00719450);
  FUN_00617060(DAT_0071946c);
  iVar8 = FUN_00617060(DAT_0071946c);
  uVar9 = FUN_00617060(DAT_0071946c);
  uVar10 = FUN_00617060(DAT_0071946c);
  iVar11 = FUN_00617620(DAT_00719450);
  if ((iVar11 == 0) ||
     (((uVar5 & 0xff0000) >> 8 | (iVar6 << 0x10 | uVar7 & 0xff00) << 8) !=
      ((iVar8 << 0x10 | uVar9 & 0xff00) << 8 | uVar10 >> 8 & 0xff00))) {
    iVar6 = FUN_00617630(DAT_00719450);
    if (iVar6 == 0) {
      bVar2 = true;
    }
    else {
      sVar16 = 0x18;
      pcVar3 = (char *)FUN_006175e0(DAT_00719450);
      _strncpy(local_78,pcVar3,sVar16);
      local_63 = local_63 & 0xffffff;
    }
  }
  else {
    sVar16 = 0x18;
    pcVar3 = (char *)FUN_00617640(DAT_00719450);
    _strncpy(local_78,pcVar3,sVar16);
    local_63 = local_63 & 0xffffff;
    uVar4 = FUN_00617650(DAT_00719450);
  }
  uVar5 = network_channels_open();
  if (DAT_006869be == '\0') {
    DAT_00719450 = 0;
    return uVar5 & 0xffffff00;
  }
  if (!bVar2) {
    DAT_0068e680 = 0xffffffff;
    DAT_0068e684 = 0xffffffff;
    DAT_0068e688 = 0xffffffff;
    DAT_00718f8c = 0;
    DAT_00718f90 = 0;
    _DAT_006b2f28 = 0;
    _DAT_006b2f68 = 0;
    _sprintf(local_40,"%s:%d",local_78,uVar4 & 0xffff);
    uVar5 = network_game_client_connect_to_address(&DAT_00719458);
    DAT_00719458 = 0;
    DAT_00719454 = 0;
    DAT_00719450 = 0;
    return uVar5;
  }
  uVar12 = FUN_006175f0(DAT_006f14c4);
  uVar13 = FUN_006175e0(DAT_00719450);
  uVar14 = FUN_00617600(DAT_00719450);
  uVar15 = FUN_004403b0(10000);
  FUN_00616f50(DAT_0071946c,uVar13,uVar14,uVar15);
  uVar5 = FUN_00614f30(uVar12,uVar15,1,FUN_0044ad80,FUN_004ba270,0);
  DAT_00719450 = 0;
  if (uVar5 == 0) {
    DAT_0068e688 = uVar15;
    DAT_0068e680 = 0xffffffff;
    DAT_0068e684 = 0xffffffff;
    DAT_00718f90 = 0;
    _DAT_006b2f28 = 0;
    _DAT_006b2f68 = 0;
    DAT_00718f8c = 4;
    return 0xffffff01;
  }
  return uVar5 & 0xffffff00;
}
#endif
