// unit_dispatch_scripted_event_9  (Ghidra: FUN_0056c370)
// address 0x56c370, size 133 bytes, name confidence 0.3, rewrite confidence 0.25
// functions.md: "Dispatches a local scripted event (id 9) built from a hashed lookup value and
// a byte parameter, and on success forwards it through network_session_broadcast_to_flagged."
// blam-cc: param_1 -> event_byte, in_ECX -> hash_key.
// UNSURE: hash_table_get's, message_delta_encode_message's and network_session_broadcast_to_flagged's real argument
// shapes are not recovered; this rewrite keeps the same literal constants the original passes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern uint8_t *network_message_table; // 0x00687130, UNSURE
extern uint8_t event9_target;        // 0x00871de0, UNSURE
extern int32_t network_role_0071c2d4; // 0x0071c2d4, UNSURE

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern int32_t message_delta_encode_message(int32_t a, int32_t event_id, int32_t b, void *payload, int32_t c, int32_t d, uint8_t e); // 0x4ec940, UNSURE signature
extern void network_session_broadcast_to_flagged(int32_t a, void *b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t g); // 0x4e1a80, UNSURE signature

void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key) // blam-cc: param_1, in_ECX
{
    int32_t looked_up = 0;
    if (hash_key != -1) {
        looked_up = hash_table_get((hash_table *)(network_message_table + 0xc), hash_key);
        if (looked_up == -1) {
            looked_up = 0;
        }
    }

    struct { uint8_t byte0; int32_t *payload; } packed = { event_byte, &looked_up };
    int32_t encoded_len = message_delta_encode_message(0, 9, 0, &packed, 0, 1, 0);
    if (0 < encoded_len) {
        network_session_broadcast_to_flagged(1, &event9_target, 1, 0, 0, 3, 0);
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56c370):

void FUN_0056c370(int *param_1)

{
  int iVar1;
  int in_ECX;
  undefined4 uVar2;
  int local_8;
  undefined1 local_4;

  local_8 = 0;
  if (in_ECX != -1) {
    local_8 = hash_table_get();
    if (local_8 == -1) {
      local_8 = 0;
    }
  }
  local_4 = param_1._0_1_;
  param_1 = &local_8;
  uVar2 = 0;
  iVar1 = message_delta_encode_message(0,9,0,&param_1,0,1,'\0');
  if (0 < iVar1) {
    FUN_004e1a80(1,&DAT_00871de0,1,0,0,3,uVar2);
  }
  return;
}
#endif
