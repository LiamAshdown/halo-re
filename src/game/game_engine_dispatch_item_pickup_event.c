// game_engine_dispatch_item_pickup_event  (Ghidra: FUN_0045f850; named per
// out/phase4/game_functions.md)
// address 0x45f850, size 147 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Dispatches a networked game-event notification (event
// id 0x2f, apparently item pickup) through the engine's event/message system").
// register convention: a machine/session id in ECX (in_ECX); param_1 and param_2 are this
// function's own stack parameters.
//   // blam-cc: ECX -> machine_id, stack -> param_1, param_2
// UNSURE: this function is almost entirely built from calls outside this batch's range
// (hash_table_get, network_index_cache_find_or_allocate_slot, message_delta_encode_message, network_session_broadcast_to_flagged) whose exact
// signatures are not established anywhere in this batch's evidence. Ghidra elides
// hash_table_get's arguments entirely and shows param_2 read as a 16-bit scalar before being
// reassigned to `&local_c` for the encode call -- both preserved literally with placeholder
// argument reconstructions, flagged individually below.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h" // hash_table
#include "units.h"
#include "game.h"
#include "networking.h"

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern network_id_table *object_network_id_table; // 0x00687130
extern int32_t network_index_cache_find_or_allocate_slot(int32_t machine_id); // 0x4e9c20, not in this batch; UNSURE signature
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
    // size, then the seven stack arguments. Returns the encoded bit length in EAX.
extern network_server_globals *network_server; // 0x0071c2d4
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server

extern uint8_t shared_hud_text_draw_state; // 0x00871de0, UNSURE identity (see game_types_notes.md)

// blam-cc: ECX -> machine_id, stack -> param_1, param_2
// Resolves `machine_id` to a local slot index (via the hash table, or by allocating one through
// network_index_cache_find_or_allocate_slot if it isn't registered yet), then encodes and broadcasts a networked event 0x2f
// (item pickup) carrying `param_1` and the low 16 bits of `param_2`.
void game_engine_dispatch_item_pickup_event(int32_t machine_id, int32_t param_1, int32_t param_2)
{
    // CORRECTED (phase 4 review, objdump 0x45f883..0x45f8c0): `&param_2` really is a pointer to
    // param_2's own stack slot, and that slot has just been loaded with `&local_c`. The encoder's
    // 4th argument is a pointer TO a pointer to the field block, exactly as Ghidra prints it;
    // MSVC reuses the dead incoming parameter slots as scratch. The block itself is
    // {int32 hash slot, int32 param_1, int16 low half of param_2} -- the third store (0x45f8b3)
    // genuinely is a 16-bit `mov`, unlike the sibling at 0x4608d0.
    struct { int32_t slot; int32_t param_1; int16_t param_2_low; } fields;
    void *fields_ptr;

    fields.slot = 0;
    if (machine_id != -1) {
        fields.slot = hash_table_get(&object_network_id_table->id_to_index, (int32_t)machine_id); // 0x45f855..0x45f864
    }
    if (fields.slot == -1) {
        fields.slot = network_index_cache_find_or_allocate_slot(machine_id);
    }

    fields.param_1 = param_1;
    fields.param_2_low = (int16_t)param_2;
    fields_ptr = &fields;

    network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x2f, 0, &fields_ptr, 0, 1, '\0'), network_server, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
}

#if 0
Original Ghidra decompilation (0x45f850), from tools/pack.py 0x45f850:

void FUN_0045f850(undefined4 param_1,int *param_2)

{
  int in_ECX;
  int extraout_ECX;
  int local_c;
  undefined4 local_8;
  undefined2 local_4;

  local_c = 0;
  if (in_ECX != -1) {
    local_c = hash_table_get();
    in_ECX = extraout_ECX;
  }
  if (local_c == -1) {
    local_c = FUN_004e9c20(in_ECX);
  }
  local_8 = param_1;
  local_4 = param_2._0_2_;
  param_2 = &local_c;
  message_delta_encode_message(0,0x2f,0,&param_2,0,1,'\0');
  FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  return;
}
#endif
