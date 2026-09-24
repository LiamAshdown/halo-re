// network_channels_open  (Ghidra: network_channels_open, already named)
// address 0x441300, size 375 bytes
// name confidence: 0.55   rewrite confidence: 0.4
// evidence: out/phase4/networking_types_notes.md "network_channel (0xae4 listening / 0xa9c
// plain)" section names network_game_socket (0x006f14c4), network_query_socket (0x006f14c8),
// network_local_address (0x006869b0) and network_channels_open_ok (0x006869be) directly.
// register convention: __cdecl, no arguments.
// UNSURE: every callee here (thunk_FUN_0061c3e0, FUN_00614800/850/8b0, FUN_0061e550) is
// foreign GameSpy transport library code (see the "server browser" note in
// networking_types_notes.md: those objects are deliberately not declared as Blam types), so
// their real signatures are not recovered -- only the observed argument count/shape is kept.
// UNSURE: LAB_00441020/00441040/00441060/004410b0/00441200 are callback addresses that fall
// in the gap between network_connection_stats_log_tick's end (0x441004) and this function's
// start (0x441300); Ghidra did not lift them as separate functions (they are presumably the
// channel receive/error/accept trampolines wired up here), so they are declared only by
// address and never rewritten in this batch.
// UNSURE: the DAT_006869b0 bit expression is a byte-order swap of network_local_address;
// kept as literal arithmetic rather than assumed to be htonl.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_game_socket;    // 0x006f14c4
extern int32_t network_query_socket;   // 0x006f14c8
extern uint32_t network_local_address; // 0x006869b0
extern uint8_t network_channels_open_ok; // 0x006869be
extern uint32_t network_game_port;     // 0x00698208
extern uint32_t network_query_port;    // 0x0069820c

// Foreign GameSpy transport calls; see UNSURE note above.
extern void FUN_006148b0(uint32_t address, uint16_t port, void *out_address); // 0x6148b0: fills a
    // 0x16-byte address record (stride confirmed by the imul esi,esi,0x16 at 0x6148c8)
extern int32_t thunk_FUN_0061c3e0(int32_t *socket_out, uint8_t address_buffer[24], int32_t unused_a,
                                   int32_t unused_b, void *receive_callback);
extern void FUN_0061e550(int32_t socket, void *callback); // error callback?
extern void FUN_00614850(int32_t socket, void *callback); // UNSURE
extern void FUN_00614800(int32_t socket, void *callback); // UNSURE

// Callback trampolines in the unlifted gap before this function; see UNSURE note above.
extern void network_channel_gap_441020(void);
extern void network_channel_gap_441040(void);
extern void network_channel_gap_441060(void);
extern void network_channel_gap_4410b0(void);
extern void network_channel_gap_441200(void);

void network_channels_open(void)
{
    uint32_t swapped_address;
    uint8_t game_address_buf[24];
    uint8_t query_address_buf[24];
    int32_t result;

    swapped_address = (uint32_t)((network_local_address << 0x10 | network_local_address & 0xff00 |
                                   network_local_address >> 0x10 & 0xff) << 8) |
                      network_local_address >> 0x18;
    network_channels_open_ok = 1;

    FUN_006148b0(swapped_address, (uint16_t)network_game_port, game_address_buf);
    FUN_006148b0(swapped_address, (uint16_t)network_query_port, query_address_buf);

    if (network_game_socket == 0) {
        result = thunk_FUN_0061c3e0(&network_game_socket, game_address_buf, 0, 0,
                                     network_channel_gap_441060);
        if (result == 0) {
            FUN_0061e550(network_game_socket, network_channel_gap_441020);
            FUN_00614850(network_game_socket, network_channel_gap_441040);
            FUN_00614800(network_game_socket, network_channel_gap_4410b0);
        } else {
            network_channels_open_ok = 0;
        }
    }

    if (network_query_socket == 0 && network_channels_open_ok == 1) {
        result = thunk_FUN_0061c3e0(&network_query_socket, query_address_buf, 0, 0,
                                     network_channel_gap_441060);
        if (result != 0) {
            network_query_port = 0;
            FUN_006148b0(swapped_address, 0, query_address_buf);
            result = thunk_FUN_0061c3e0(&network_query_socket, query_address_buf, 0, 0,
                                         network_channel_gap_441060);
            if (result != 0) {
                network_channels_open_ok = 0;
                return;
            }
        }
        FUN_0061e550(network_query_socket, network_channel_gap_441020);
        FUN_00614850(network_query_socket, network_channel_gap_441040);
        FUN_00614800(network_query_socket, network_channel_gap_441200);
    }
}

#if 0
Original Ghidra decompilation (0x441300):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void network_channels_open(void)

{
  int iVar1;
  uint uVar2;
  undefined1 local_30 [24];
  undefined1 local_18 [24];

  uVar2 = (DAT_006869b0 << 0x10 | DAT_006869b0 & 0xff00 | DAT_006869b0 >> 0x10 & 0xff) << 8 |
          DAT_006869b0 >> 0x18;
  DAT_006869be = '\x01';
  FUN_006148b0(uVar2,(undefined2)DAT_00698208,local_18);
  FUN_006148b0(uVar2,_DAT_0069820c & 0xffff,local_30);
  if (DAT_006f14c4 == 0) {
    iVar1 = thunk_FUN_0061c3e0(&DAT_006f14c4,local_18,0,0,&LAB_00441060);
    if (iVar1 == 0) {
      FUN_0061e550(DAT_006f14c4,&LAB_00441020);
      FUN_00614850(DAT_006f14c4,&LAB_00441040);
      FUN_00614800(DAT_006f14c4,&LAB_004410b0);
    }
    else {
      DAT_006869be = '\0';
    }
  }
  if ((DAT_006f14c8 == 0) && (DAT_006869be == '\x01')) {
    iVar1 = thunk_FUN_0061c3e0(&DAT_006f14c8,local_30,0,0,&LAB_00441060);
    if (iVar1 != 0) {
      _DAT_0069820c = 0;
      FUN_006148b0(uVar2,0,local_30);
      iVar1 = thunk_FUN_0061c3e0(&DAT_006f14c8,local_30,0,0,&LAB_00441060);
      if (iVar1 != 0) {
        DAT_006869be = 0;
        return;
      }
    }
    FUN_0061e550(DAT_006f14c8,&LAB_00441020);
    FUN_00614850(DAT_006f14c8,&LAB_00441040);
    FUN_00614800(DAT_006f14c8,&LAB_00441200);
  }
  return;
}
#endif
