// network_game_broadcast_team_object_updates  (Ghidra: FUN_004df950, unnamed)
// address 0x4df950, size 183 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Walks every live game object and, for the
// ones belonging to an active team, encodes and broadcasts an object-update packet,
// returning the total bytes sent and object count through param_2/unaff_EDI." Iterates with
// object_iterator_next (types/objects.h object_iterator, already rewritten at
// src/objects/object_iterator_next.c); object+4 matches types/objects.h object::network_role;
// object+0xb4 matches object::type.
// register convention: EDI = object_count (int32_t *), stack = param_1 (unused), param_2
// (int32_t *bytes_sent).
// blam-cc: EDI -> object_count, stack -> (unused, bytes_sent)
// UNSURE: the iterator's type_mask (object_iterator+0x00) is never assigned a value in the
// decompiled body; treated as "every type" (0xffffffff) to match the summary's "every live
// game object". flags_mask is assigned 0, which passes every object regardless of flags.
// UNSURE: `(&PTR_PTR_0069bfdc)[object->type]` indexes an unresolved per-object-type table;
// declared here as an opaque pointer array and the +0x10 read is transcribed literally
// without a named field, since no struct for that table exists in types/objects.h.
// UNSURE: FUN_004f44f0's real parameters/purpose (object-update encode into a shared
// 0x7ff8-byte scratch buffer) are not independently confirmed here.
// UNSURE: param_1 is a genuine but entirely unused stack parameter in the original.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "networking.h"

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, objects module
extern void *object_type_definitions[12]; // 0x0069bfdc, an ARRAY of the 12 object type definitions (was a pointer variable)
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t object_type_override_get_0x64(uint8_t *out_buffer, int32_t out_buffer_size); // other module; UNSURE
extern uint32_t network_session_send_to_machine(int32_t machine_id, uint8_t *data, int32_t bits,
    int32_t reliable, int32_t unknown_a, int32_t unknown_b, int32_t priority); // 0x4e1930, this batch;
    // UNSURE: parameter names/count guessed from this call site alone, see that file when written

// Encodes and broadcasts an object-update packet for every live object whose network_role is
// zero and whose type-table team slot is populated, accumulating the total encoded size into
// *bytes_sent and the object count into *object_count.
void network_game_broadcast_team_object_updates(int32_t *object_count, uint32_t param_1, int32_t *bytes_sent)
{
    object_iterator iterator;
    object *obj;
    int32_t encoded_bits;

    (void)param_1;
    iterator.type_mask = 0xffffffff; // UNSURE: not shown assigned in the decompilation
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = 0xffffffff;

    obj = object_iterator_next(&iterator);
    while (obj != 0) {
        if (obj->network_role == 0 &&
            *(int32_t *)((uint8_t *)object_type_definitions[obj->type] + 0x10) != -1) {
            encoded_bits = object_type_override_get_0x64(network_message_scratch, 0x7ff8);
            if (encoded_bits > 0) {
                *bytes_sent = *bytes_sent + encoded_bits;
                *object_count = *object_count + 1;
                network_session_send_to_machine(1, network_message_scratch, encoded_bits, 1, 0, 0, 3);
            }
        }
        obj = object_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x4df950):

void FUN_004df950(undefined4 param_1,int *param_2)

{
  int iVar1;
  int *unaff_EDI;
  undefined1 local_10 [4];
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar1 = object_iterator_next(local_10);
  while (iVar1 != 0) {
    if (((*(int *)(iVar1 + 4) == 0) &&
        (*(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar1 + 0xb4)] + 0x10) != -1)) &&
       (iVar1 = FUN_004f44f0(&DAT_00871de0,0x7ff8), 0 < iVar1)) {
      *param_2 = *param_2 + iVar1;
      *unaff_EDI = *unaff_EDI + 1;
      network_session_send_to_machine(1,&DAT_00871de0,iVar1,1,0,0,3);
    }
    iVar1 = object_iterator_next(local_10);
  }
  return;
}
#endif
