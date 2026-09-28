// object_notify_pickup_or_refresh_probe
// address 0x4ee3c0, size 260 bytes
// name confidence: 0.3 (still FUN_004ee3c0 in Ghidra; named from out/phase4/objects_functions.md's
// summary, "Validates a target object then queues a UI/network event (message type 0x32),
// falling back to a lighting-probe refresh...", though the fallback call is now known to be
// object_throttled_multiplayer_sound_event, not a lighting probe)
// rewrite confidence: 0.8 (network lookup and send arguments fixed against objdump 0x4ee451..0x4ee4ab)
// evidence: types/objects.h object.type (0xb4), object.network_role (0x04); the player-record
// reads at DAT_0087a480 (+0/+2) mirror the generic data_array element header shape used
// throughout this module, but the players module is not owned here so they are kept as raw
// offsets.
// UNSURE: object_try_and_get is called here with only its type-mask argument visible (3, the
// unit mask); the object index it operates on arrives through a register this decompile does
// not show. The unit-extension field at object+0x218 and the player-record field at +2 are also
// unresolved. network_index_cache_get, message_delta_encode_message and network_session_send_to_machine are
// all outside this module.
// register convention: datum_index player_index in EDI (unaff_EDI); the single stack argument is
//   the object handle (0x4ee415 mov ebx,[esp+0x14] / mov ecx,ebx into object_try_and_get).
// (previous wording kept below) param_1 (undefined4) is
// accepted but never read before being reused as a local scratch value.
// blam-cc: EDI=player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"


extern data_array *player_data;         // 0x0087a480, players module (not owned here)
extern void *network_object_index_cache;                // 0x006870d8, UNSURE
extern uint8_t network_message_scratch[0x7ff8];                // 0x00871de0, UNSURE

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
    // 0x4f6ec0; object handle in ECX, type mask on the stack. Verified against the body at
    // 0x4f6ec0 (cmp ecx,-1 / test cx,cx / and param_1 & 1 << header->type) and against the
    // call site in this file.
extern void object_throttled_multiplayer_sound_event(void); // this module, 0x4ee370 // this module, 0x4ee370 (object_throttled_multiplayer_sound_event)
extern int32_t network_index_cache_get(hash_table *table, int32_t key); // 0x4e9d20, stack table, ECX key
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern network_server_globals *network_server;
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *server,
    uint32_t param_1, void *data, uint32_t param_3, uint32_t reliable, uint32_t unknown_a,
    char force, uint32_t priority); // 0x4e1930, EAX, ESI, stack

void object_notify_pickup_or_refresh_probe(uint32_t object_index, datum_index player_index)
    // blam-cc: EDI -> player_index; stack -> object_index
{
    int16_t index = (int16_t)player_index;

    if (player_index == (datum_index)0xffffffff || index < 0 || index >= player_data->maximum_count) {
        return;
    }

    {
        uint8_t *record = (uint8_t *)player_data->data + (int32_t)player_data->size * index;
        int16_t identifier = *(int16_t *)record;

        if (identifier == 0) {
            return;
        }

        {
            int16_t salt = (int16_t)(player_index >> 0x10);

            if (salt != 0 && identifier != salt) {
                return;
            }
        }

        {
            object *unit = object_try_and_get(object_index, _object_mask_unit);
            // 0x4ee415 mov ebx,[esp+0x14] / mov ecx,ebx -- the function's stack argument

            if (unit == 0 || unit->type != _object_type_biped || unit->network_role != 0 ||
                *(int32_t *)((uint8_t *)unit + 0x218) == (int32_t)player_index) {
                return;
            }

            if (*(int16_t *)(record + 2) == -1) {
                int32_t encoded_value;
                void *field_list[2]; // local_8/local_4: a value pointer followed by a 0 terminator
                int32_t encoded;

                encoded_value = network_index_cache_get((hash_table *)&network_object_index_cache, (int32_t)object_index); // 0x4ee451: ECX = object
                field_list[0] = &encoded_value;
                field_list[1] = 0;
                encoded = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x32, 0, field_list, 0, 1, 0);
                network_session_send_to_machine((int8_t)record[0x64], network_server, 1, network_message_scratch,
                    (uint32_t)encoded, 0, 0, 0, 9); // 0x4ee49a: EAX = the player machine, ESI = network_server
                return;
            }

            object_throttled_multiplayer_sound_event();
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ee3c0):

void FUN_004ee3c0(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  short sVar3;
  int iVar4;
  short sVar5;
  int unaff_EDI;
  undefined4 *local_8;
  undefined4 local_4;

  if (((unaff_EDI != -1) && (sVar5 = (short)unaff_EDI, -1 < sVar5)) &&
     (sVar5 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar2 = *(int *)(DAT_0087a480 + 0x34);
    iVar4 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar5;
    sVar5 = *(short *)(iVar4 + iVar2);
    if (((sVar5 != 0) &&
        ((sVar3 = (short)((uint)unaff_EDI >> 0x10), sVar3 == 0 || (sVar5 == sVar3)))) &&
       ((iVar1 = object_try_and_get(3), iVar1 != 0 &&
        (((*(short *)(iVar1 + 0xb4) == 0 && (*(int *)(iVar1 + 4) == 0)) &&
         (*(int *)(iVar1 + 0x218) != unaff_EDI)))))) {
      if (*(short *)(iVar4 + iVar2 + 2) == -1) {
        param_1 = FUN_004e9d20(&DAT_006870d8);
        local_8 = &param_1;
        local_4 = 0;
        iVar2 = message_delta_encode_message(0,0x32,0,&local_8,0,1,'\0');
        network_session_send_to_machine(1,&DAT_00871de0,iVar2,0,0,0,9);
        return;
      }
      FUN_004ee370();
    }
  }
  return;
}
#endif
