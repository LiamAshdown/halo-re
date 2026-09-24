// master_server_connection_start  (Ghidra: master_server_connection_start, already named)
// address 0x4b6000, size 101 bytes
// name confidence: 0.55   rewrite confidence: 0.35
// evidence: out/phase4/networking_types_notes.md "server browser" section:
// server_list_mutex (0x007196a8) and server_list_thread (0x007196ac) are named there;
// mutex_create.c and network_thread_create.c document the out-parameter conventions this
// function relies on.
// register convention: __cdecl, no arguments. mutex_create's out_handle parameter (EDI) is
// not shown at this call site (Ghidra already analyzed mutex_create as taking it via EDI, so
// the load must be an invisible `lea edi, server_list_mutex` immediately before the call);
// reconstructed explicitly here as `&server_list_mutex`, matching mutex_create.c's own
// convention.
// UNSURE: network_thread_create's out_handle is written to `&server_list_thread`, but
// types/networking.h declares that global as `int32_t` (a plain valid/invalid flag) --
// network_thread_create actually writes a `network_thread_record *` there (NULL on failure,
// a real pointer on success), and every other reader only ever tests it against zero, so the
// two interpretations agree at every use site; cast through here rather than changing the
// header's declared type.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_mutex_record *server_list_mutex; // 0x007196a8
extern network_thread_record *server_list_thread; // 0x007196ac, see UNSURE
extern uint32_t master_server_request_flags;    // 0x0071969c
extern int32_t master_server_last_result;       // 0x007196a4

extern int32_t mutex_create(network_mutex_record **out_handle); // 0x440510, this module
extern int32_t network_thread_create(uint8_t flags, void *start_address, void *parameter,
                                       network_thread_record **out_handle); // 0x440460, this module
extern void sig__setup_master_server_connection_sig(void); // thread entry point, foreign/unresolved
extern int32_t CloseHandle(void *object); // Win32

// Resets the master-server request state, creates the server-list mutex, and starts the
// background thread that owns the master-server connection. On thread-creation failure, tears
// the mutex slot back down by hand and reports failure. Returns 1 on success, 0 otherwise
// (including when the mutex itself could not be created).
int32_t master_server_connection_start(void)
{
    int32_t mutex_ok;
    int32_t thread_ok;
    network_mutex_record *mutex_slot;

    master_server_request_flags = 0;
    master_server_last_result = 0;
    mutex_ok = mutex_create(&server_list_mutex);
    if (mutex_ok != 0) {
        thread_ok = network_thread_create(0, (void *)sig__setup_master_server_connection_sig, 0,
                                           (network_thread_record **)&server_list_thread);
        mutex_slot = server_list_mutex;
        if (thread_ok != 0) {
            return 1;
        }
        CloseHandle(mutex_slot->handle);
        mutex_slot->name[0] = 0;
        mutex_slot->handle = 0;
        mutex_slot->in_use = 0;
        server_list_mutex = 0;
        server_list_thread = 0;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4b6000):

undefined4 master_server_connection_start(void)

{
  undefined4 *puVar1;
  char cVar2;

  DAT_0071969c = 0;
  DAT_007196a4 = 0;
  cVar2 = mutex_create();
  if (cVar2 != '\0') {
    cVar2 = network_thread_create(0,&sig__setup_master_server_connection_sig,0,&DAT_007196ac);
    puVar1 = DAT_007196a8;
    if (cVar2 != '\0') {
      return 1;
    }
    CloseHandle((HANDLE)*DAT_007196a8);
    *(undefined1 *)(puVar1 + 1) = 0;
    *puVar1 = 0;
    *(undefined1 *)(puVar1 + 9) = 0;
    DAT_007196a8 = (undefined4 *)0x0;
    DAT_007196ac = 0;
  }
  return 0;
}
#endif
