// network_channel_list_mark_readable  (Ghidra: FUN_004419d0, still unnamed -> renamed)
// address 0x4419d0, size 102 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("iterates a channel list, marking
// each channel that has data ready to read (either explicitly flagged or via a nonzero
// circular-buffer fill level)"); out/phase4/networking_types_notes.md pins
// network_pending_connection_count (0x006f16d0) and network_receive_queue's flags/incoming
// fields.
// register convention: list pointer in EDI (unaff_EDI).
// UNSURE: exactly why a nonzero network_pending_connection_count alone (regardless of actual
// buffered bytes) is enough to mark a data_ready entry readable is not explained here; kept
// exactly as decompiled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern int32_t network_pending_connection_count; // 0x006f16d0

// blam-cc: list pointer in EDI (unaff_EDI)
// Marks every entry in `list` whose queue either already reports data_ready (while a
// connection request is pending) or whose incoming circular buffer has bytes available, by
// setting bit2 of the entry's flags byte. Returns 0 if at least one entry was marked, else
// -13 (0xfffffff3).
int32_t network_channel_list_mark_readable(network_channel_list *list)
{
    int32_t i;
    network_receive_queue *entry;
    circular_buffer *incoming;
    int32_t available;
    int32_t result;

    result = 0xfffffff3;
    if (-1 < list->last_index) {
        for (i = 0; i <= list->last_index; i++) {
            entry = list->entries[i];
            if (entry->data_ready == 1 && 0 < network_pending_connection_count) {
                entry->flags |= 4;
                result = 0;
            } else {
                incoming = entry->incoming;
                if (incoming != 0) {
                    available = incoming->write_cursor - incoming->read_cursor;
                    if (available < 0) {
                        available = available + incoming->capacity;
                    }
                    if (0 < available) {
                        entry->flags |= 4;
                        result = 0;
                    }
                }
            }
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4419d0):

undefined4 FUN_004419d0(void)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  int unaff_EDI;

  iVar3 = DAT_006f16d0;
  iVar6 = 0;
  uVar4 = 0xfffffff3;
  if (-1 < *(int *)(unaff_EDI + 0x10c)) {
    do {
      iVar2 = *(int *)(*(int *)(unaff_EDI + 0x104) + iVar6 * 4);
      if ((*(char *)(iVar2 + 4) == '\x01') && (0 < iVar3)) {
LAB_00441a19:
        pbVar1 = (byte *)(*(int *)(*(int *)(unaff_EDI + 0x104) + iVar6 * 4) + 0xc);
        *pbVar1 = *pbVar1 | 4;
        uVar4 = 0;
      }
      else {
        iVar2 = *(int *)(iVar2 + 0x10);
        if (iVar2 != 0) {
          iVar5 = *(int *)(iVar2 + 0xc) - *(int *)(iVar2 + 8);
          if (iVar5 < 0) {
            iVar5 = iVar5 + *(int *)(iVar2 + 0x10);
          }
          if (0 < iVar5) goto LAB_00441a19;
        }
      }
      iVar6 = iVar6 + 1;
    } while (iVar6 <= *(int *)(unaff_EDI + 0x10c));
  }
  return uVar4;
}
#endif
