// server_browser_latch_join_target  (Ghidra: FUN_004b6730, still unnamed -> renamed)
// address 0x4b6730, size 161 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("latches the currently selected
// server-list entry as the join target, prompting for a password first if the server requires
// one"); server_list_mutex/server_list_thread/server_list (types/networking.h) match
// DAT_007196a8/ac/bc/c0; the "password" string key matches the GameSpy bool accessor
// SBServerGetBoolValue that networking_types_notes.md's "server browser" section documents.
// register convention: __cdecl, no arguments.
// UNSURE: DAT_00719491 (a "join requested" gate, guessed name), DAT_00719450 (the latched
// entry pointer), DAT_00719454 (the has-password result) and network_host_edit_field_00719410 have no documented
// names in networking_types_notes.md; declared here only by address with best-guess names.
// UNSURE: virtual_keyboard_open's argument meaning (0x12, 0xc) was not resolved (outside this
// session's range).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_thread_record *server_list_thread; // 0x007196ac
extern network_mutex_record *server_list_mutex; // 0x007196a8
extern server_list_globals server_list;         // 0x007196bc
extern int32_t server_browser_selected_index; // 0x006953f4
extern uint8_t server_browser_join_requested;   // 0x00719491, see UNSURE
extern void *server_browser_join_target;        // 0x00719450, see UNSURE
extern uint8_t server_browser_join_target_has_password; // 0x00719454, see UNSURE
extern int32_t network_host_edit_field_00719410;                    // see UNSURE
extern uint32_t master_server_request_flags;    // 0x0071969c

extern int32_t SBServerGetBoolValue(void *entry, const char *key, int32_t default_value); // foreign, GameSpy bool accessor // foreign, GameSpy library
extern void virtual_keyboard_open(int32_t screen_id, int32_t field_id); // foreign, outside this session's range

// blam-cc: __cdecl, no arguments
void server_browser_latch_join_target(void)
{
    uint32_t wait_result;

    if (server_list_thread != 0) {
        wait_result = WaitForSingleObject(server_list_mutex->handle, 100);
        if (wait_result != 0 && wait_result != 0x80) {
            return;
        }
    }
    if (server_browser_join_requested != 0 && -1 < server_browser_selected_index &&
        server_browser_selected_index < server_list.result_count) {
        server_browser_join_target =
            *(void **)((uint8_t *)&server_list + server_browser_selected_index * 4);
        server_browser_join_target_has_password =
            SBServerGetBoolValue(server_browser_join_target, "password", 0);
        if (server_browser_join_target_has_password != 0) {
            virtual_keyboard_open(0x12, 0xc);
            network_host_edit_field_00719410 = 0;
        }
        master_server_request_flags = master_server_request_flags | 4;
    }
    if (server_list_thread != 0) {
        ReleaseMutex(server_list_mutex->handle);
    }
}

#if 0
Original Ghidra decompilation (0x4b6730):

void FUN_004b6730(void)

{
  DWORD DVar1;

  if (((DAT_007196ac != 0) && (DVar1 = WaitForSingleObject((HANDLE)*DAT_007196a8,100), DVar1 != 0))
     && (DVar1 != 0x80)) {
    return;
  }
  if (((DAT_00719491 != '\0') && (-1 < DAT_006953f4)) && (DAT_006953f4 < DAT_007196c0)) {
    DAT_00719450 = *(undefined4 *)(DAT_007196bc + DAT_006953f4 * 4);
    DAT_00719454 = FUN_006174d0(DAT_00719450,"password",0);
    if (DAT_00719454 != '\0') {
      virtual_keyboard_open(0x12,0xc);
      DAT_00719410 = 0;
    }
    DAT_0071969c = DAT_0071969c | 4;
  }
  if (DAT_007196ac != 0) {
    ReleaseMutex((HANDLE)*DAT_007196a8);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
