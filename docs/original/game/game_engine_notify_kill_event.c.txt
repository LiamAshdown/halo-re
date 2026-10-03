// game_engine_notify_kill_event  (Ghidra: FUN_004608d0; renamed per its summary)
// address 0x4608d0, size 242 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Dispatches a networked kill-event notification (event
// id 0x18) to machines whose recorded machine id matches the local player"); the near-identical
// already-committed game_engine_dispatch_item_pickup_event.c (0x45f850), which uses the same
// hash_table_get / message_delta_encode_message / network_session_send_to_machine trio with the
// same "&param_N is really &local_c, the compiler reused the stack slot" pattern; types/game.h
// player (identifier +0x00), and the "network_server read but not owned" note (+0x0071c2d4).
// register convention: a hash-table key in ECX (in_ECX, gates the lookup exactly like
// game_engine_dispatch_item_pickup_event's machine_id); a player index in EAX (in_EAX, used only
// in the machine-matching loop below); param_1/param_2 are this function's own stack parameters.
//   // blam-cc: EAX -> player_index, ECX -> hash_key, stack -> message_type, subject
// UNSURE: the player-index-to-machine
// matching loop over `network_server + 0x3c4` (16 entries, stride 0x60) and the two flag bits it
// tests at each entry's +0xe are outside any header this module owns and are kept as raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h" // hash_table

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data;      // 0x0087a480
extern uint8_t *network_server;     // 0x0071c2d4
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern uint8_t *machine_table; // 0x00687558, its hash_table sits at +0x0c
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
    // EDX -> destination size, then the seven stack arguments. Returns the encoded bit
    // length in EAX. `fields` is a pointer TO a pointer to the field block -- see below.
extern uint32_t network_session_send_to_machine(int32_t machine_id, void *server, uint32_t status_bit, void *data,
    uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority); // 0x4e1930, EAX machine, ESI server

// blam-cc: EAX -> player_index, ECX -> hash_key, stack -> message_type, subject
// Encodes and broadcasts a networked event 0x18 (kill notification) carrying `message_type` and
// the low 16 bits of `subject`, then, if the encode produced a payload, finds the network
// session's machine-record slot whose stored id matches `player_index`'s player-record field at
// +0x64 and, when that slot has both status bits 0x2 and 0x4 set, sends the encoded message to
// that one machine.
void game_engine_notify_kill_event(uint32_t player_index, int32_t hash_key, int32_t message_type,
    datum_index subject)
{
    // CORRECTED (phase 4 review, objdump 0x4608f4..0x460933): the encoder's 4th argument is
    // `&param_1`, and param_1's own stack slot has just been loaded with `&local_c`. The argument
    // is therefore a pointer TO a pointer to the three-dword field block, not the block itself.
    // The block is {hash slot, message_type, subject}, all three FULL dwords -- the store at
    // 0x460927 is a 32-bit `mov`, so `subject` is not truncated to 16 bits as the first pass had
    // it. MSVC reuses the dead incoming parameter slots as the scratch for the outer pointer,
    // which is what produced Ghidra's "&param_N" confusion.
    int32_t fields[3];
    void *fields_ptr;
    int32_t encoded_size;

    fields[0] = 0;
    if (hash_key != -1) {
        fields[0] = hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)hash_key); // 0x4608d8..0x4608e8
        if (fields[0] == -1) {
            fields[0] = 0;
        }
    }
    fields[1] = message_type;
    fields[2] = (int32_t)subject;
    fields_ptr = fields;

    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x18, 0, &fields_ptr, 0, 1, '\0');
    if (0 < encoded_size) {
        player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
        uint8_t player_machine_field = *(uint8_t *)&p->machine_index; // UNSURE: field identity, see types/game.h player::unknown_64
        int16_t *machine = (int16_t *)(network_server + 0x3c4); // UNSURE: network_server layout, not owned here
        int32_t i = 0;

        while ((int32_t)*machine != (int32_t)(int8_t)player_machine_field) {
            i = i + 1;
            machine = machine + 0x30; // pointer arithmetic on int16_t*: advances 0x60 bytes
            if (0xf < i) {
                return;
            }
        }

        {
            uint8_t *entry = network_server + 0x3b8 + i * 0x60; // UNSURE: matching machine record
            if (entry != 0) {
                uint8_t flags = (uint8_t)*(uint16_t *)(entry + 0xe);
                if (((flags >> 1) & 1) != 0 && ((flags >> 2) & 1) != 0) {
                    // 0x4609a3..0x4609b3: EAX = the player's machine index (0x460959), ESI = the server (0x46094a)
                    network_session_send_to_machine((int32_t)(int8_t)player_machine_field, network_server, 1, network_message_scratch,
                                                    (uint32_t)encoded_size, 1, 0, 0, 3);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4608d0), from tools/pack.py 0x4608d0:

void FUN_004608d0(int *param_1,undefined4 param_2)

{
  uint in_EAX;
  int iVar1;
  byte bVar2;
  int in_ECX;
  int iVar3;
  short *psVar4;
  int local_c;
  void *local_8;
  undefined4 local_4;
  
  local_c = 0;
  if (in_ECX != -1) {
    local_c = hash_table_get();
    if (local_c == -1) {
      local_c = 0;
    }
  }
  local_8 = param_1;
  param_1 = &local_c;
  local_4 = param_2;
  param_2 = 0;
  iVar1 = message_delta_encode_message(0,0x18,0,&param_1,0,1,'\0');
  if (0 < iVar1) {
    iVar3 = 0;
    psVar4 = (short *)(DAT_0071c2d4 + 0x3c4);
    while ((int)*psVar4 !=
           (int)*(char *)((in_EAX & 0xffff) * 0x200 + 100 + *(int *)(DAT_0087a480 + 0x34))) {
      iVar3 = iVar3 + 1;
      psVar4 = psVar4 + 0x30;
      if (0xf < iVar3) {
        return;
      }
    }
    iVar3 = iVar3 * 0x60 + 0x3b8 + DAT_0071c2d4;
    if (((iVar3 != 0) && (bVar2 = (byte)*(undefined2 *)(iVar3 + 0xe), (bVar2 >> 1 & 1) != 0)) &&
       ((bVar2 >> 2 & 1) != 0)) {
      network_session_send_to_machine(1,&DAT_00871de0,iVar1,1,0,0,3);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
