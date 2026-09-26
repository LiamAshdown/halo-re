// object_queue_pickup_denied_event
// address 0x4efbf0, size 136 bytes
// name confidence: 0.3 (still FUN_004efbf0 in Ghidra; named from out/phase4/objects_functions.md's
// summary, "Packages a damage-related argument block into a network/UI event (type 0x31) and
// dispatches it")
// rewrite confidence: 0.25
// evidence: none beyond the message-type constant; the source struct (`unaff_EDI`, a pointer
// this decompile never receives as a visible parameter) is entirely UNSURE.
// UNSURE: `unaff_EDI` is a 3-dword source record read but never assigned anywhere in this
// decompile — an implicit parameter threaded in from a wider caller context, exactly like the
// unaff_EBX case in object_apply_shield_damage.c. `in_ECX` (the hash_table_get key) is likewise
// implicit. Both are modeled as explicit parameters here since this function's own definition is
// being written, but every caller of it in this codebase is outside this module's range.
// int32_t key in ECX (in_ECX); uint32_t *source in EDI (unaff_EDI, a 3-dword record).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern uint8_t object_network_message_scratch[0x7ff8]; // 0x00871de0, UNSURE

extern uint8_t *object_pooled_node_globals; // 0x00687130
extern int32_t hash_table_get(hash_table *table, uint32_t key); // UNSURE: zero visible args; this module, 0x4f05e0 (skipped
    // as library/non-objects code, but still a valid callee here)
extern int32_t message_delta_encode_message(uint32_t a1, uint32_t a2, uint32_t a3, void *scratch,
    uint32_t a5, uint32_t a6, uint8_t a7); // network module, 0x4ec940
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, ECX server

// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: ECX -> key, EDI -> source, stack -> param_1
void object_queue_pickup_denied_event(void *param_1, int32_t key, uint32_t *source)
{
    // UNSURE: this 5-dword block (`local_14` through `local_4` in the decompile) is laid out
    // contiguously on the stack and passed as one array via `&local_14`; preserved here as one
    // struct in the same order rather than the five disconnected locals Ghidra shows.
    struct {
        int32_t looked_up;
        void *original_param_1;
        uint32_t source0;
        uint32_t source1;
        uint32_t source2;
    } block;

    block.looked_up = 0;
    if (key != -1) {
        block.looked_up = hash_table_get((hash_table *)(object_pooled_node_globals + 0xc), key);
    }
    block.source1 = source[1];
    block.source0 = source[0];
    block.original_param_1 = param_1;
    block.source2 = source[2];

    message_delta_encode_message(0, 0x31, 0, &block.looked_up, 0, 1, 0);
    network_session_broadcast_to_flagged(network_server_pointer, 1, object_network_message_scratch, 0, 0, 0, 3);
}

#if 0
Original Ghidra decompilation (0x4efbf0):

void FUN_004efbf0(undefined4 *param_1)

{
  int in_ECX;
  undefined4 *unaff_EDI;
  undefined4 local_14;
  void *local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_14 = 0;
  if (in_ECX != -1) {
    local_14 = hash_table_get();
  }
  local_8 = unaff_EDI[1];
  local_c = *unaff_EDI;
  local_10 = param_1;
  local_4 = unaff_EDI[2];
  param_1 = &local_14;
  message_delta_encode_message(0,0x31,0,&param_1,0,1,'\0');
  FUN_004e1a80(1,&DAT_00871de0,0,0,0,3);
  return;
}
#endif
