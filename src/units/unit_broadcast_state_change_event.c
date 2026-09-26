// unit_broadcast_state_change_event  (Ghidra: unit_broadcast_state_change_event)
// address 0x566c00, size 129 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.2
// evidence: functions.md summary ("Queues a networked event/message carrying an optional
//   hash-table-resolved index, used after seat-entry/exit transitions").
// register convention: an index (or -1) on the stack.
//   // blam-cc: param_1 -> index
// UNSURE: hash_table_get is called with zero visible arguments; modelled as taking `index` as
//   its key since that is the only value in scope. message_delta_encode_message's and
//   network_session_broadcast_to_flagged's literal argument lists are reproduced as Ghidra shows them; PTR_DAT_00687130,
//   DAT_006870d0/0x006870d8 and event9_target are outside this batch's evidence and kept as raw
//   externs.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern uint8_t *network_message_table; // 0x00687130, PTR_DAT_00687130, UNSURE shape
extern uint8_t event9_target[];          // 0x00871de0, UNSURE shape

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, EAX bits, ECX server

void unit_broadcast_state_change_event(int32_t index) // blam-cc: param_1 -> index
{
    int32_t resolved = 0;
    if (index != -1) {
        resolved = hash_table_get((hash_table *)(network_message_table + 0xc), index); // 0x566c09..0x566c18
        if (resolved == -1) {
            resolved = 0;
        }
    }

    struct { int32_t *value; int32_t zero; } fields = {0};
    int32_t value = resolved;
    fields.value = &value;
    fields.zero = 0;

    int32_t sent = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xc, 0, (void **)&fields, 0, 1, 0);
    if (sent > 0) {
        network_session_broadcast_to_flagged(sent, network_server_pointer, 1, event9_target, 1, 0, 0, 3);
    }
}

#if 0
Original Ghidra decompilation (0x566c00):

void FUN_00566c00(int param_1)

{
  int iVar1;
  undefined4 *local_8;
  undefined4 local_4;

  iVar1 = 0;
  if (param_1 != -1) {
    iVar1 = hash_table_get();
    if (iVar1 == -1) {
      iVar1 = 0;
    }
  }
  local_8 = &param_1;
  local_4 = 0;
  param_1 = iVar1;
  iVar1 = message_delta_encode_message(0,0xc,0,&local_8,0,1,'\0');
  if (0 < iVar1) {
    FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  }
  return;
}
#endif
