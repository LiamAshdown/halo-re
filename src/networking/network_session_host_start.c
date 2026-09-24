// network_session_host_start  (Ghidra: FUN_00577850; named per this rewrite)
// address 0x577850, size 159 bytes
// name confidence: 0.45   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md summary: "Opens the network channel layer and
// creates a network channel/session object configured with the previously-set host info and a
// set of message-handler callbacks." Its own teardown counterpart (0x5778f0) and per-frame
// update (0x577940) are named to match in this rewrite.
// register convention: the __cdecl stack parameter Ghidra recognized (param_1); everything else
// is a foreign (GameSpy-shaped) call whose arguments Ghidra elided.
// UNSURE: nearly every callee here (FUN_006175f0, FUN_00616340, FUN_00615530, FUN_0061b6d0) is
// foreign vendor-library glue outside this module; their argument lists are transcribed exactly
// as decompiled (which is very likely incomplete -- Ghidra shows no arguments for several calls
// that clearly need them), so this file carries an unusually high uncertainty even by this
// module's standards.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_game_socket;                        // 0x006f14c4
extern void *network_session_host_object;                   // 0x00722a20
extern int32_t network_session_start_game_type;              // 0x007227b8
extern char network_session_start_host_name[];                // 0x00722798
extern char network_session_start_map_name[];                  // 0x007227a0
extern uint32_t network_session_host_flags;                    // 0x0069fe00, UNSURE
extern int32_t network_console_connection_id;                  // 0x0069fdfc, UNSURE: shared with sv_ban / host_dispose

extern void network_session_host_dispose(void);              // 0x5778f0, this module
extern void network_channels_open(void);                     // 0x441300, this module
extern int32_t FUN_006175f0(int32_t socket);                  // foreign, UNSURE
extern void *FUN_00616340(void **object, int32_t query_result, int32_t game_type, void *host_name,
                           void *map_name, uint32_t flags, int32_t a7, void *cb1, void *cb2,
                           void *cb3, void *cb4, void *cb5, void *cb6, int32_t param_1); // foreign, UNSURE
extern void FUN_00615530(void *object, void *callback); // foreign, UNSURE
extern void FUN_0061b6d0(void *object, int32_t message_type, uint32_t flags); // foreign, UNSURE
extern void network_session_host_dispatch_message(int32_t message_type, int32_t param_2, void *reply_target); // 0x577e40, this module

// Opens the network channel layer and creates the GameSpy-shaped session object configured with
// the previously-set host info (network_session_host_start_info_set) and a set of
// message-handler callbacks, one of which is network_session_host_dispatch_message.
void *network_session_host_start(int32_t param_1)
{
    void *result;

    network_session_host_dispose();
    network_channels_open();
    result = (void *)FUN_006175f0(network_game_socket);
    result = FUN_00616340(&network_session_host_object, (int32_t)(long)result,
                           network_session_start_game_type, network_session_start_host_name,
                           network_session_start_map_name, network_session_host_flags, 1,
                           (void *)0, (void *)network_session_host_dispatch_message, (void *)0, (void *)0, (void *)0,
                           (void *)0, param_1);
    FUN_00615530(network_session_host_object, (void *)0);
    network_console_connection_id = 0x319;
    FUN_0061b6d0(network_session_host_object, 0x319, network_session_host_flags);
    return result;
}

#if 0
Original Ghidra decompilation (0x577850):

undefined4 FUN_00577850(undefined4 param_1)

{
  undefined4 uVar1;

  FUN_005778f0();
  network_channels_open();
  uVar1 = FUN_006175f0(DAT_006f14c4);
  uVar1 = FUN_00616340(&DAT_00722a20,uVar1,DAT_007227b8,&DAT_00722798,&DAT_007227a0,DAT_0069fe00,1,
                       &LAB_005779c0,FUN_00577e40,&LAB_00577f40,&LAB_00577fb0,&LAB_005780c0,
                       &LAB_00578100,param_1);
  FUN_00615530(DAT_00722a20,&LAB_00578160);
  DAT_0069fdfc = 0x319;
  FUN_0061b6d0(DAT_00722a20,0x319,DAT_0069fe00);
  return uVar1;
}
#endif
