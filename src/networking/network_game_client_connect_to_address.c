// network_game_client_connect_to_address  (Ghidra: network_game_client_connect_to_address,
// already named)
// address 0x4dc790, size 314 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Parses a user-entered address[:port] string,
// resolves it to a numeric address, and kicks off the connect handshake." Confirmed against
// objdump -d -M intel bin/halo.exe at 0x4dc790..0x4dc8c9: the address-string argument is EAX
// (saved into EBX at entry, in_EAX in Ghidra's decompile); the four inet_addr() calls per branch
// are the same "call an accessor four times with the same argument and reassemble a byte-swapped
// 32-bit value via mask/shift" pattern already documented in
// network_channel_get_remote_address.c (there for FUN_006175f0/FUN_006147e0), reused verbatim
// here for inet_addr. The result, together with size = k_network_address_size_ipv4 and a port
// (parsed after ':' when present via network_address_parse_port... actually _atol directly here,
// or network_game_socket_port when address_string has no ':'), is written to a local
// s_network_address; disassembly at 0x4dc8b5 (`lea ecx,[esp+0xc]`) passes that record's address
// as the hidden ECX argument to network_client_begin_connect, which the callee's own body confirms by testing
// address->ipv4 != 0 and address->port != 0 (in_ECX and *(short*)(in_ECX+0x12) in its decompile).
// register convention: EAX -> address_string; player_name is the one ordinary cdecl stack
// parameter. blam-cc: EAX -> address_string, stack -> player_name

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdlib.h>
#include <string.h>

extern uint32_t network_game_socket_port; // 0x00698208
extern uint32_t inet_addr(const char *address); // Winsock
extern int32_t interface_loading_screen_text_buffer; // 0x006b2f28, UNSURE identity/type; see
    // network_join_request_resolve_host.c for the same global under this name
extern wchar_t *string_convert_ascii_to_unicode(wchar_t *dest, int32_t dest_bytes, const char *source); // 0x557990, this module's established signature

extern uint32_t network_client_begin_connect(wchar_t *player_name, s_network_address *target_address); // 0x4dc8d0, this batch

// blam-cc: EAX -> address_string
uint32_t network_game_client_connect_to_address(wchar_t *player_name, char *address_string)
{
    char host_part[256];
    char *colon;
    int i;
    s_network_address target;
    uint32_t byte0, byte1, byte2, byte3;

    for (i = 0; ; i++) {
        host_part[i] = address_string[i];
        if (address_string[i] == '\0') {
            break;
        }
    }
    colon = strchr(host_part, ':');
    if (colon == 0) {
        byte0 = inet_addr(address_string);
        byte1 = inet_addr(address_string);
        byte2 = inet_addr(address_string);
        byte3 = inet_addr(address_string);
        target.ipv4 = ((byte0 & 0xff0000 | byte1 >> 0x10) >> 8) |
                      ((byte2 << 0x10 | byte3 & 0xff00) << 8);
        target.size = k_network_address_size_ipv4;
        target.port = (uint16_t)network_game_socket_port;
    } else {
        *colon = '\0';
        byte0 = inet_addr(host_part);
        byte1 = inet_addr(host_part);
        byte2 = inet_addr(host_part);
        byte3 = inet_addr(host_part);
        target.ipv4 = ((byte0 & 0xff0000 | byte1 >> 0x10) >> 8) |
                      ((byte2 << 0x10 | byte3 & 0xff00) << 8);
        target.size = k_network_address_size_ipv4;
        target.port = (uint16_t)atol(colon + 1);
    }
    if (address_string == 0) {
        // Unreachable in practice: the copy loop above already dereferenced address_string
        // unconditionally, so a real NULL would have faulted before this point. Kept verbatim.
        interface_loading_screen_text_buffer = 0;
    } else {
        string_convert_ascii_to_unicode(0, 0, address_string); // UNSURE: elided register arguments; dest/dest_bytes
            // guessed from this module's established string_convert_ascii_to_unicode(dest, dest_bytes, source) shape
    }
    return network_client_begin_connect(player_name, &target);
}

#if 0
Original Ghidra decompilation (0x4dc790):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void network_game_client_connect_to_address(undefined4 param_1)

{
  char cVar1;
  char *in_EAX;
  char *pcVar2;
  undefined1 *puVar3;
  char local_100 [256];

  pcVar2 = in_EAX;
  do {
    cVar1 = *pcVar2;
    pcVar2[(int)(local_100 + -(int)in_EAX)] = cVar1;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  puVar3 = (undefined1 *)FUN_006257e0(local_100,0x3a);
  if (puVar3 == (undefined1 *)0x0) {
    inet_addr(in_EAX);
    inet_addr(in_EAX);
    inet_addr(in_EAX);
    inet_addr(in_EAX);
  }
  else {
    *puVar3 = 0;
    inet_addr(local_100);
    inet_addr(local_100);
    inet_addr(local_100);
    inet_addr(local_100);
    _atol(puVar3 + 1);
  }
  if (in_EAX == (char *)0x0) {
    _DAT_006b2f28 = 0;
  }
  else {
    FUN_00557990();
  }
  FUN_004dc8d0(param_1);
  return;
}
#endif
