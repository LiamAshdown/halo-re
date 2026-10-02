// network_session_host_start  (Ghidra: FUN_00577850)
// address 0x577850, size 159 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x577850..0x5778ee: disposes the old session, opens the channels and starts
//   query/report on the game socket's SOCKET (0x6175f0) with the port, game name and secret key strings, the public
//   flag byte, natneg on, the six host callbacks (the earlier version passed NULL for five of them and the player key
//   callback in the wrong slot) and the argument as user data; registers the natneg callback, and initializes the CD
//   key server with game id 0x319 on the same record. Returns qr2_init_socketA's result.
// blam-cc: cdecl

#include "tags.h"
#include <string.h>
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t network_game_socket;                    // 0x006f14c4 (GT2Socket; its first dword is the SOCKET)
extern void *network_session_host_object;               // 0x00722a20
extern int32_t network_session_start_game_type;          // 0x007227b8 (passed as the query port)
extern char network_session_start_host_name[];           // 0x00722798 (the qr2 game name)
extern char network_session_start_map_name[];            // 0x007227a0 (the qr2 secret key)
extern uint8_t network_session_host_flags_byte;          // 0x0069fe00
extern int32_t network_console_connection_id;            // 0x0069fdfc (the CD key game id)
extern void network_session_host_dispose(void);          // 0x5778f0
extern void network_channels_open(void);                 // 0x441300
extern int32_t qr2_init_socketA(void **qrec_out, uint32_t socket, int32_t port, const char *gamename, const char *secret_key,
    int32_t ispublic, int32_t natnegotiate, void *server_key, void *player_key, void *team_key, void *key_list, void *count,
    void *adderror, void *userdata); // 0x616340 qr2_init_socketA
extern void qr2_register_natneg_callback(void *qrec, void *callback); // 0x615530 qr2_register_natneg_callback
extern void gcd_init_qr2(void *qrec, int32_t game_id, int32_t use_network); // 0x61b6d0 gcd_init_qr2
extern void network_session_host_natneg_callback(int32_t cookie); // 0x578160
extern void network_session_host_qr2_server_key(int32_t key_id, void *buffer, void *user_data); // 0x5779c0
extern void network_session_host_dispatch_message(int32_t key_id, int32_t index, void *buffer, void *user_data); // 0x577e40
extern void network_session_host_qr2_team_key(int32_t key_id, int32_t index, void *buffer, void *user_data); // 0x577f40
extern void network_session_host_qr2_key_list(int32_t key_type, void *keybuffer, void *user_data); // 0x577fb0
extern int32_t network_session_host_qr2_count(int32_t key_type, void *user_data); // 0x5780c0
extern void network_session_host_qr2_add_error(int32_t error, char *message, void *user_data); // 0x578100

int32_t network_session_host_start(void *user_data)
{
    int32_t result;

    network_session_host_dispose();
    network_channels_open();
    result = qr2_init_socketA(&network_session_host_object, *(uint32_t *)network_game_socket, network_session_start_game_type,
        network_session_start_host_name, network_session_start_map_name, network_session_host_flags_byte, 1,
        (void *)network_session_host_qr2_server_key, (void *)network_session_host_dispatch_message,
        (void *)network_session_host_qr2_team_key, (void *)network_session_host_qr2_key_list,
        (void *)network_session_host_qr2_count, (void *)network_session_host_qr2_add_error, user_data);
    qr2_register_natneg_callback(network_session_host_object, (void *)network_session_host_natneg_callback);
    network_console_connection_id = 0x319;
    gcd_init_qr2(network_session_host_object, 0x319, network_session_host_flags_byte);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
