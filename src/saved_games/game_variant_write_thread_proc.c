// game_variant_write_thread_proc  (Ghidra: FUN_0053c150, renamed)
// address 0x53c150, size 260 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/saved_games_functions.md summary "Background-thread worker that writes
// the pending profile record and applies the associated profile file rename/copy," corrected by
// out/phase4/saved_games_types_notes.md: this is the asynchronous *game variant* writer thread
// proc started by game_variant_write_request_start.c (0x53c0b0) over variant_write_request_state,
// whose thread entry point network_thread_create receives as `void (*)(void*)`; the one parameter
// is the request pointer (types/saved_games.h variant_write_request).
// The player_profile_rename call is shown with only its new_name argument (param_1 + 4, i.e.
// &request->variant, whose first field IS the name -- game_variant::name is at offset 0); its own
// file (player_profile_rename.c) establishes handle arrives in EAX with no stack argument for it,
// so the dropped handle argument here is read as request->handle, carried in a register Ghidra
// didn't track across the call, exactly like saved_game_delete_by_handle's missing EDI argument
// two lines later.
// UNSURE: player_profile_rename is called whenever file_reference_close succeeds, even on the
// branch where the seek/write itself failed (write_failed already true); saved_game_delete_by_handle
// then still runs afterward to roll the slot back. This ordering (rename-then-delete-on-failure)
// is reproduced exactly as decompiled, not reordered.
// register convention: __cdecl (well, thread-proc convention), one stack argument (request).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern network_mutex_record *saved_game_files_mutex; // 0x0072143c

extern uint8_t saved_game_open_file_by_handle(int32_t handle, file_reference_record *out_ref); // 0x53c9f0, this module
extern uint8_t file_reference_seek(int32_t offset, file_reference_record *ref); // 0x5558f0, this module
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module
extern uint8_t player_profile_rename(int32_t handle, uint16_t *new_name); // 0x53ce80, this module, blam-cc: EAX handle
extern uint8_t saved_game_delete_by_handle(int32_t handle); // 0x53c960, this module
extern void crc32_update(uint32_t *checksum, const void *data, uint32_t size); // 0x4d02d0
extern uint32_t __stdcall WaitForSingleObject(void *object, uint32_t milliseconds); // Win32
extern int32_t __stdcall ReleaseMutex(void *mutex); // Win32

// blam-cc: one stack argument (request)
// Waits on saved_game_files_mutex, then opens request->handle's saved-game file, crcs and writes
// request->variant over its body, closes it, renames the slot to match the (possibly just
// written) variant's name if the close succeeded, and deletes the slot if the seek or write
// failed. Always releases the mutex before returning. Return value is always 0.
uint32_t game_variant_write_thread_proc(variant_write_request *request)
{
    uint32_t wait_result;
    file_reference_record ref;
    game_variant_file file;
    uint8_t opened;
    uint8_t seeked;
    uint8_t written;
    uint8_t closed;
    uint8_t write_failed;

    wait_result = WaitForSingleObject(saved_game_files_mutex->handle, 5000);
    if (wait_result != 0 && wait_result != 0x80) {
        return 0;
    }
    write_failed = 0;
    opened = saved_game_open_file_by_handle(request->handle, &ref);
    if (opened != 0) {
        file.variant = request->variant;
        file.checksum = 0xffffffff;
        crc32_update(&file.checksum, &file.variant, sizeof(file.variant));
        seeked = file_reference_seek(0, &ref);
        if (seeked == 0) {
            write_failed = 1;
        } else {
            written = file_reference_write(&ref, &file, sizeof(file));
            if (written == 0) {
                write_failed = 1;
            }
        }
        closed = file_reference_close(&ref);
        if (closed != 0) {
            player_profile_rename(request->handle, request->variant.name);
        }
        if (write_failed != 0) {
            saved_game_delete_by_handle(request->handle);
        }
    }
    ReleaseMutex(saved_game_files_mutex->handle);
    return 0;
}

#if 0
Original Ghidra decompilation (0x53c150):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 FUN_0053c150(int param_1)

{
  bool bVar1;
  char cVar2;
  DWORD DVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined1 local_2118 [272];
  undefined4 local_2008 [38];
  undefined4 local_1f70 [2009];
  undefined4 uStack_c;

  uStack_c = 0x53c160;
  DVar3 = WaitForSingleObject((HANDLE)*DAT_0072143c,5000);
  if ((DVar3 != 0) && (DVar3 != 0x80)) {
    return 0;
  }
  bVar1 = false;
  cVar2 = saved_game_open_file_by_handle(local_2118);
  if (cVar2 != '\0') {
    puVar5 = (undefined4 *)(param_1 + 4);
    puVar6 = local_2008;
    for (iVar4 = 0x26; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    local_1f70[0] = 0xffffffff;
    crc32_update(local_1f70,local_2008,0x98);
    cVar2 = file_reference_seek();
    if ((cVar2 == '\0') || (cVar2 = file_reference_write(), cVar2 == '\0')) {
      bVar1 = true;
    }
    cVar2 = file_reference_close();
    if (cVar2 != '\0') {
      player_profile_rename((undefined4 *)(param_1 + 4));
    }
    if (bVar1) {
      saved_game_delete_by_handle();
    }
  }
  ReleaseMutex((HANDLE)*DAT_0072143c);
  return 0;
}
#endif
