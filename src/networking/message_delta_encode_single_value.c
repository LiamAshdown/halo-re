// message_delta_encode_single_value  (Ghidra: FUN_004ec450; named per this rewrite)
// address 0x4ec450, size 52 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary: "Convenience wrapper that builds a
// message-delta message for a single value via message_delta_encode_message." The call passes
// `&stack0x0000000c` (this function's own param_2 storage) as the one-element items array and
// `&stack0x00000010` -- four bytes further into this function's own frame, immediately past its
// two recognized parameters -- as type_offset, which message_delta_encode_message only ever uses
// relative to each item's own address. That relative offset (+4) is reproduced here directly.
// register convention: message_type and value as the __cdecl stack parameters Ghidra recognized,
// force_changed in DL (in_DL, unresolved register read).
// blam-cc: stack -> message_type, value; DL -> force_changed
// UNSURE: whether a genuine third stack parameter exists at the "+4 past value" slot, or this is
// reading uninitialized/caller-frame data; message_delta_encode_message's flag argument is
// hardcoded to 1 by this wrapper, so *some* per-item "type" dword is expected to be there.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t message_delta_encode_message(int32_t flag, int32_t message_type, int32_t changed_offset,
                                             void **items, int32_t type_offset, int32_t count,
                                             char force_changed); // 0x4ec940, this module

// blam-cc: stack -> message_type, value; DL -> force_changed
// Builds a message-delta message carrying a single value: the one-element items array points at
// `value` itself, and the field is reported "changed" (changed_offset = &value) whenever value is
// non-zero.
int32_t message_delta_encode_single_value(int32_t message_type, int32_t value, int32_t type_value, char force_changed)
{
    struct {
        int32_t value;
        int32_t type_value; // UNSURE: see file header
    } item;
    void *items[1];

    item.value = value;
    item.type_value = type_value;
    items[0] = &item;
    return message_delta_encode_message(1, message_type, value != 0 ? (int32_t)(int32_t)&item : 0,
                                         items, (int32_t)(int32_t)&item.type_value, 1, force_changed);
}

#if 0
Original Ghidra decompilation (0x4ec450):

void FUN_004ec450(int param_1,int param_2)

{
  char in_DL;

  message_delta_encode_message
            (1,param_1,-(uint)(param_2 != 0) & (uint)&param_2,(void **)&stack0x0000000c,
             (int)&stack0x00000010,1,in_DL);
  return;
}
#endif
