// object_delete_unparented
// address 0x4f5aa0, size 171 bytes
// name confidence: 0.4 (still FUN_004f5aa0 in Ghidra; functions.md's summary -- "Performs the
//   actual work of deleting a single object with no parent: notifies scripts, unlinks it from
//   its cluster, and raises an object-deleted event" -- matches the shape of the code)
// rewrite confidence: 0.5 (raised from 0.35 by the phase-4 review pass: network_index_cache_remove's two register arguments were resolved from the disassembly)
// evidence: types/objects.h object_header (flags at 0x02, _object_header_delete_pending_bit);
//   global 0x008603b0 object_data; callees hash_table_get (0x4f05e0, memory module),
//   message_delta_encode_message (0x4ec940), network_index_cache_remove, FUN_004e1a80.
// register convention: object index in EDI (unaff_EDI).
// UNSURE: hash_table_get's, message_delta_encode_message's and network_index_cache_remove's arguments are not
//   visible in the decompilation beyond the literal constants shown; the object index is passed
//   to hash_table_get and network_index_cache_remove by inference only, matching how every other function in
//   this batch threads its object index through calls Ghidra could not see the arguments of.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t object_network_message_scratch[0x7ff8]; // 0x00871de0, see object_new_with_datum_role_control.c

extern network_id_table *object_network_id_table; // 0x00687130
extern void *object_pooled_node_globals_006870d8; // 0x006870d8, UNSURE: network_index_cache_remove's EAX operand
extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module; UNSURE: key inferred
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940
extern void network_index_cache_remove(void *globals, uint32_t object_index); // 0x4e9d40.
    // Resolved from `objdump -d -M intel bin/halo.exe`: both call sites in this module set
    // EAX to the literal 0x006870d8 and ESI to the object index immediately before the call
    // (0x4f5b16 `mov esi,edi / mov eax,0x6870d8` and 0x4f5b96 `mov eax,0x6870d8` with ESI
    // already holding the index). Ghidra shows neither, so it used to be declared with the
    // object index alone.
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server
    // 0x4e1a80, UNSURE: unexamined

void object_delete_unparented(uint32_t object_index) // blam-cc: EDI -> object_index
{
    object_header *header;
    int32_t looked_up = 0;
    int32_t *scratch_pointer;
    int32_t scratch_tail;
    int32_t encoded_length;

    if (object_index != k_datum_index_none) {
        looked_up = hash_table_get(&object_network_id_table->id_to_index, object_index);
            // table in ESI, key in ECX (verified against the body at 0x4f05e0)
        if (looked_up == -1) {
            looked_up = 0;
        }
    }

    scratch_pointer = &looked_up;
    scratch_tail = 0;
    // 0x4f5ae0: EAX = the network message scratch buffer, EDX = its size; stack (0, 0, 0, &items, 0, 1, 0)
    encoded_length = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0, 0,
                                                  (void **)&scratch_pointer, 0, 1, 0);
    (void)scratch_tail;

    header = (object_header *)object_data->data + (object_index & 0xffff);
    if ((header->flags & _object_header_delete_pending_bit) == 0) {
        network_index_cache_remove(&object_pooled_node_globals_006870d8, object_index); // FIXED: EAX is the container ADDRESS 0x6870d8 (mov eax,imm); its dword is 0xd, not a pointer
    }

    if (encoded_length > 0) {
        network_session_broadcast_to_flagged(encoded_length, network_server_pointer, 1, object_network_message_scratch, 1, 0, 0, 3); // 0x4f5b3b: EAX = the encoded length
    }
}

#if 0
Original Ghidra decompilation (0x4f5aa0):

void FUN_004f5aa0(void)

{
  int iVar1;
  uint unaff_EDI;
  int local_c;
  int *local_8;
  undefined4 local_4;

  local_c = 0;
  if (unaff_EDI != 0xffffffff) {
    local_c = hash_table_get();
    if (local_c == -1) {
      local_c = 0;
    }
  }
  local_8 = &local_c;
  local_4 = 0;
  iVar1 = message_delta_encode_message(0,0,0,&local_8,0,1,'\0');
  if ((*(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + (unaff_EDI & 0xffff) * 0xc) & 8) == 0) {
    FUN_004e9d40();
  }
  if (0 < iVar1) {
    FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  }
  return;
}
#endif
