// unit_broadcast_state_change_event  (Ghidra: unit_broadcast_state_change_event)
// address 0x566c00, size 129 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.85
// REWRITTEN from objdump 0x566c00..0x566c80 and its two callers (0x5680e5, 0x5681de), which both copy a 0x20
//   byte unit_state_change_record onto the stack (rep movs): the record arrives BY VALUE. Its unit is replaced
//   by the network id (object_network_id_table +0x0c, 0 when unknown) and the record itself, as the one
//   item {&record, 0}, is encoded as message 0xc and broadcast (0x4e1a80, reliable, priority 3).
// blam-cc: stack -> record (by value)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"

extern uint8_t *object_network_id_table; // 0x00687130, object_network_id_table

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, ESI table, ECX key
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern network_server_globals *network_server; // 0x0071c2d4
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server

void unit_broadcast_state_change_event(unit_state_change_record record)
{
    int32_t resolved = 0;
    void *items[2];
    int32_t sent;

    if (record.unit != (datum_index)-1) {
        resolved = hash_table_get((hash_table *)(object_network_id_table + 0xc), (int32_t)record.unit);
        if (resolved == -1) {
            resolved = 0;
        }
    }
    record.unit = (datum_index)resolved;
    items[0] = &record;
    items[1] = 0;
    sent = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xc, 0, items, 0, 1, 0);
    if (sent > 0) {
        network_session_broadcast_to_flagged(sent, network_server, 1, network_message_scratch, 1, 0, 0, 3);
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
