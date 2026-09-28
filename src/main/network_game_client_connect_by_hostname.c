// network_game_client_connect_by_hostname  (Ghidra: network_game_client_connect_by_hostname,
// already named)
// address 0x4c83e0, size 274 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/main_functions.md summary; out/phase4/main_types_notes.md pins
// main_globals_data.connect_address/connect_pending (0x00719a7a/0x00719a79) and identifies
// 0x00718f94/0x00718fc9/0x00718fa4 as the interface module's ui_root_widget/ui_split_screen/
// ui_unknown_718fa4 globals (src/interface/interface_handle_quit_request.c). 0x00719754/
// 0x0071973c/0x00719757 are, per main.h's own byte-width-confirmed field table, actually
// main_globals.switch_structure_bsp_index/save_map/return_to_main_menu (confirmed by
// main_menu_return_and_reset 0x4c8a60, which clears the same return_to_main_menu byte once it
// services the request) -- NOT the interface module's own less-informed "split_screen_quit_
// prompt_*" guess for the same addresses. FUN_006257e0 is the strchr-shaped foreign helper used
// the same way in src/networking/network_banlist_load.c and
// src/networking/network_game_client_connect_to_address.c. host_port_string is GlobalAlloc'd by
// the caller (network_game_client_connect_to_address_async, this module) and freed here.
// register convention: __stdcall thread procedure (ends `ret 4` at 0x4c84ef; CreateThread start
// address in network_game_client_connect_to_address_async), one parameter (host_port_string).
// The phase 3 header said __cdecl; corrected in the phase 4 review, which also re-checked the
// body against 0x4c83e0..0x4c84ef (no other drift; network_hostname_resolve_with_timeout takes ECX).
// UNSURE: hostent->h_addr_list is accessed as a raw +0xc dereference, not a typed struct,
// matching src/networking/network_local_hostent_get.c / network_initialize.c's treatment of
// Winsock's hostent as opaque.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern main_globals main_globals_data; // 0x00719700
extern void *connect_thread;   // 0x00719b64, this module; HANDLE of the worker thread, 0 when idle

// Foreign interface-module globals; see file header evidence.
extern uint8_t ui_split_screen;                              // 0x00718fc9
extern widget_instance *ui_root_widget[1];                   // 0x00718f94
extern int16_t network_join_error_code; // 0x00718fa4, foreign (interface module), -1 when none pending

extern void network_dispatch_initialize(void); // 0x4414c0, foreign (networking module)
extern char network_hostname_resolve_with_timeout(char *hostname); // 0x4c8370, this module
extern uint8_t network_game_client_connect_to_address_async(char *address, char *password); // 0x4c8500, this module
extern char *strchr(const char *string, int character); // CRT strchr (0x6257e0: the MSVC asm strchr)

// Resolves host_port_string (a "host" or "host:port" string, GlobalAlloc'd by the caller) and
// stages the result for the main loop to connect to: on success, formats the resolved dotted
// address (with the port re-appended, if one was given) into main_globals_data.connect_address and
// arms main_globals_data.connect_pending. On failure, re-enters the "no address" path of
// network_game_client_connect_to_address_async to clear the staged connect state, then -- unless
// the UI is already showing the main menu in split-screen -- arms the "can't connect" prompt.
// Always frees host_port_string and clears connect_thread before returning.
uint32_t __stdcall network_game_client_connect_by_hostname(char *host_port_string)
{
    uint16_t port;
    char *colon;
    void *host;
    char *address_text;

    port = 0;
    colon = strchr(host_port_string, ':');
    if (colon != 0) {
        port = (uint16_t)atol(colon + 1);
        *colon = '\0';
    }
    network_dispatch_initialize();
    if (network_hostname_resolve_with_timeout(host_port_string) != 0) {
        host = gethostbyname(host_port_string);
        if (host != 0) {
            address_text = inet_ntoa(**(struct in_addr **)((char *)host + 0xc));
            if (port == 0) {
                strncpy(main_globals_data.connect_address, address_text, 0x1f);
                main_globals_data.connect_address[0x1f] = 0;
                main_globals_data.connect_pending = 1;
            } else {
                sprintf(main_globals_data.connect_address, "%s:%d", address_text, (unsigned int)port);
                main_globals_data.connect_pending = 1;
            }
            goto done;
        }
    }
    network_game_client_connect_to_address_async(0, 0);
    if (ui_split_screen == 1 && ui_root_widget[0] != 0 &&
        strncmp(ui_root_widget[0]->name, "the_main_menu", 14) == 0) {
        goto done;
    }
    if (network_join_error_code == -1) {
        network_join_error_code = 0x35;
    }
    main_globals_data.switch_structure_bsp_index = -1;
    main_globals_data.save_map = 0;
    main_globals_data.return_to_main_menu = 1;
done:
    GlobalFree(host_port_string);
    connect_thread = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x4c83e0):

int __cdecl network_game_client_connect_by_hostname(char *host_port_string)

{
  char cVar1;
  undefined1 *puVar2;
  long lVar3;
  hostent *phVar4;
  char *pcVar5;
  int iVar6;
  ushort uVar7;
  char *pcVar8;
  bool bVar9;

  uVar7 = 0;
  puVar2 = (undefined1 *)FUN_006257e0(host_port_string,0x3a);
  if (puVar2 != (undefined1 *)0x0) {
    lVar3 = _atol(puVar2 + 1);
    uVar7 = (ushort)lVar3;
    *puVar2 = 0;
  }
  FUN_004414c0();
  cVar1 = network_hostname_resolve_with_timeout();
  if (cVar1 != '\0') {
    phVar4 = gethostbyname(host_port_string);
    if (phVar4 != (hostent *)0x0) {
      pcVar5 = inet_ntoa((in_addr)((_union_1226 *)*phVar4->h_addr_list)->S_un_b);
      if (uVar7 == 0) {
        _strncpy(&DAT_00719a7a,pcVar5,0x1f);
        DAT_00719a99 = 0;
        DAT_00719a79 = 1;
      }
      else {
        _sprintf(&DAT_00719a7a,"%s:%d",pcVar5,(uint)uVar7);
        DAT_00719a79 = 1;
      }
      goto LAB_004c84d9;
    }
  }
  network_game_client_connect_to_address_async((char *)0x0,(char *)0x0);
  if ((DAT_00718fc9 == '\x01') && (DAT_00718f94 != 0)) {
    iVar6 = 0xe;
    bVar9 = true;
    pcVar5 = *(char **)(DAT_00718f94 + 4);
    pcVar8 = "the_main_menu";
    do {
      if (iVar6 == 0) break;
      iVar6 = iVar6 + -1;
      bVar9 = *pcVar5 == *pcVar8;
      pcVar5 = pcVar5 + 1;
      pcVar8 = pcVar8 + 1;
    } while (bVar9);
    if (bVar9) goto LAB_004c84d9;
  }
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 0x35;
  }
  DAT_00719754._0_2_ = 0xffff;
  DAT_0071973c = 0;
  DAT_00719754._3_1_ = 1;
LAB_004c84d9:
  GlobalFree(host_port_string);
  DAT_00719b64 = 0;
  return 0;
}
#endif
