// network_channels_close  (Ghidra: network_channels_close, already named)
// address 0x441480, size 57 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: out/phase4/networking_types_notes.md names network_query_socket (0x006f14c8) and
// network_game_socket (0x006f14c4) directly; this is the exact inverse of
// network_channels_open.c, closing both with the same foreign GameSpy transport call.
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_game_socket;  // 0x006f14c4
extern int32_t network_query_socket; // 0x006f14c8

extern void FUN_00614860(int32_t socket); // foreign GameSpy transport call, closes a socket

void network_channels_close(void)
{
    if (network_query_socket != 0) {
        FUN_00614860(network_query_socket);
        network_query_socket = 0;
    }
    if (network_game_socket != 0) {
        FUN_00614860(network_game_socket);
        network_game_socket = 0;
    }
}

#if 0
Original Ghidra decompilation (0x441480):

void __cdecl network_channels_close(void)

{
  if (DAT_006f14c8 != 0) {
    FUN_00614860(DAT_006f14c8);
    DAT_006f14c8 = 0;
  }
  if (DAT_006f14c4 != 0) {
    FUN_00614860(DAT_006f14c4);
    DAT_006f14c4 = 0;
  }
  return;
}
#endif
