// network_handle_registry_close_all  (Ghidra: FUN_00441bb0, still unnamed -> renamed)
// address 0x441bb0, size 59 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("closes and clears any outstanding
// handles left in a fixed-size global handle/flag table"). out/phase2/networking/00.md shows
// the only two call sites: network_receive_queue_new (0x441bf0) calls this before allocating a
// fresh queue, and network_receive_queue_free (0x441c80) calls it right after freeing one --
// both are "sweep anything stale before/after touching the receive-queue pool" call sites.
// The table runs from 0x006f14d0 to (not including) 0x006f16d0, i.e. 64 slots of 8 bytes, and
// ends exactly where network_pending_connection_count (0x006f16d0, types/networking.h) begins,
// which is what pins its length. Each slot's first dword is itself a pointer to an 8-byte
// {handle, in_use-byte} pair -- the exact shape of network_thread_record (types/networking.h)
// -- so the slot is read here as network_thread_record*.
// register convention: __cdecl, no arguments.
// UNSURE: no other function in this module reads or writes this table, so which code
// populates a slot (and therefore what class of thread/handle this registry actually tracks)
// could not be confirmed here; the record shape is reused rather than guessed at.
// network_handle_registry_slot now lives in types/networking.h (folded from this file).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_handle_registry_slot network_handle_registry[64]; // 0x006f14d0

// blam-cc: __cdecl, no arguments
// Sweeps the 64-slot handle registry; for every slot that is both registered and still points
// at a live record, closes the record's Win32 handle and clears both the record and the slot.
void network_handle_registry_close_all(void)
{
    int32_t i;
    network_thread_record *record;

    for (i = 0; i < 64; i++) {
        record = network_handle_registry[i].record;
        if (record != 0 && network_handle_registry[i].registered != 0) {
            CloseHandle(record->handle);
            record->handle = 0;
            record->in_use = 0;
            network_handle_registry[i].record = 0;
            network_handle_registry[i].registered = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x441bb0):

void FUN_00441bb0(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;

  puVar2 = &DAT_006f14d0;
  do {
    puVar1 = (undefined4 *)*puVar2;
    if ((puVar1 != (undefined4 *)0x0) && (*(char *)(puVar2 + 1) != '\0')) {
      CloseHandle((HANDLE)*puVar1);
      *puVar1 = 0;
      *(undefined1 *)(puVar1 + 1) = 0;
      *puVar2 = 0;
      *(undefined1 *)(puVar2 + 1) = 0;
    }
    puVar2 = puVar2 + 2;
  } while ((int)puVar2 < 0x6f16d0);
  return;
}
#endif
