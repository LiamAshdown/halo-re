// network_channel_key_close  (Ghidra: FUN_004de8c0; named per this rewrite)
// address 0x4de8c0, size 55 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Releases the channel previously obtained for
// the given (player,machine) key via player_new_local, recording the returned index at in_EAX+0x1f
// if valid." Mirrors network_channel_key_open.c.
// register convention: EAX -> entry, EBX -> requested_handle.
// blam-cc: EAX -> entry, EBX -> requested_handle
// FIXED (register inputs, objdump): EBX (read at 0x4de8dd, "mov eax,ebx" right before the call)
// is not an implicit result of player_new_local -- it is player_new_local's own EAX/
// requested_handle register argument (confirmed against src/game/player_new_local.c's recovered
// signature: EAX -> requested_handle, stack -> machine_index, local_player_index,
// identifier_record). EBX is callee-saved in cdecl, so it is unchanged by the call, and the
// `cmp ebx,0xffffffff` afterward re-examines this function's own EBX input, not player_new_local's
// (unused) EAX return value. The previous header's guess that EBX was written by the call was
// wrong. Also corrected while fixing this: `esi` (entry) is pushed directly as
// player_new_local's identifier_record argument (0x4de8d6, `push esi`), not a local &index
// out-param -- the previous rewrite invented a 3-argument player_new_local call with an
// out-parameter that objdump does not support.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t network_channel_key_resolve_target(network_player_entry *entry); // 0x4ddcc0, this batch
extern datum_index player_new_local(datum_index requested_handle, uint32_t machine_index,
    int16_t local_player_index, uint16_t *identifier_record); // 0x473940, src/game/player_new_local.c

// blam-cc: EAX -> entry, EBX -> requested_handle
int32_t network_channel_key_close(network_player_entry *entry, datum_index requested_handle)
{
    int16_t key;

    if (network_channel_key_resolve_target(entry) == 0) {
        key = -1;
    } else {
        key = entry->machine_player_index;
    }
    player_new_local(requested_handle, entry->machine_index, key, (uint16_t *)entry); // return value unused, matches objdump
    if (requested_handle != (datum_index)-1) {
        entry->slot_index = (int8_t)requested_handle;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4de8c0):

undefined4 FUN_004de8c0(void)

{
  char cVar1;
  short sVar2;
  int in_EAX;
  int unaff_EBX;

  cVar1 = FUN_004ddcc0();
  if (cVar1 == '\0') {
    sVar2 = -1;
  }
  else {
    sVar2 = (short)*(char *)(in_EAX + 0x1d);
  }
  FUN_00473940((int)*(char *)(in_EAX + 0x1c),sVar2);
  if (unaff_EBX != -1) {
    *(char *)(in_EAX + 0x1f) = (char)unaff_EBX;
    return 1;
  }
  return 0;
}
#endif
