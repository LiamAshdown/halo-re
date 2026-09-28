// game_engine_spawn_or_replay_netgame_equipment  (Ghidra: FUN_0045f8f0; named per
// out/phase4/game_functions.md)
// address 0x45f8f0, size 248 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Spawns (or replays a network message spawning) a
// single netgame-equipment item at its configured position and orientation"); types/tags.h
// Scenario (netgame_equipment reflexive at +0x384/+0x388), ScenarioNetgameEquipment (0x90 bytes,
// position at +0x40, facing at +0x4c); types/objects.h object (flags +0x10).
// register convention: a message/param block in EAX (in_EAX); UNSURE of its layout beyond the
// one dword this function reads.
//   // blam-cc: EAX -> message
// UNSURE: message_delta_decode_compound_field, message_delta_decode_compound_field_staged, network_index_cache_insert_if_free, object_list_membership_set, object_type_override_call_0x68
// and the local_8c/local_94/local_98 object-creation buffers are all outside this batch's range
// and undocumented; their exact signatures are inferred only from this call shape.
// message_delta_decode_compound_field almost certainly writes the netgame-equipment index out through a hidden pointer
// argument (Ghidra reads that index, `local_90`, with no visible prior assignment) -- modelled
// here as an explicit out-parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern Scenario *global_scenario; // 0x00746f8c
extern data_array *object_data; // 0x008603b0

extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590, blam-cc:
    // EAX -> event, ECX -> out_values; UNSURE identity (a network message-delta decode)
extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670, blam-cc: EAX -> event; UNSURE identity
extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0, canonical form (src/items)
extern datum_index object_new_with_datum_role_control(object_placement_data *placement,
    uint32_t role); // 0x4f54b0
extern void network_index_cache_insert_if_free(uint32_t unknown); // 0x4e9cd0, not in this batch
extern void object_list_membership_set(int32_t unknown); // 0x4f7450, not in this batch
extern void object_type_override_call_0x68(uint32_t object_index); // 0x4f4560, objects module; handle in ESI

extern double fcos(double radians); // a single x87 FCOS instruction (see src/game/vector3d_clamp_length.c for the same sqrt idiom)
extern double fsin(double radians); // a single x87 FSIN instruction

// blam-cc: EAX -> message
// If `*message == 0`, spawns a netgame-equipment item locally (decoding which one via
// message_delta_decode_compound_field) at its scenario-configured position/facing; otherwise (`*message != 0`) treats
// this as a networked replay and defers entirely to message_delta_decode_compound_field_staged.
void game_engine_spawn_or_replay_netgame_equipment(int32_t *message)
{
    if (*message != 0) {
        message_delta_decode_compound_field_staged(message); // objdump: EAX == message
        return;
    }

    {
        int16_t equipment_index;
        // objdump 0x45f905: EAX == message, ECX == &the decode block whose +0x08 word is the
        // netgame-equipment index Ghidra shows as this call's only argument.
        char decoded = (char)message_delta_decode_compound_field(message, &equipment_index);
        ScenarioNetgameEquipment *equipment;

        if (!decoded || global_scenario->netgame_equipment.pointer == 0) {
            return;
        }
        equipment = &((ScenarioNetgameEquipment *)global_scenario->netgame_equipment.pointer)[equipment_index];

        {
            uint32_t placement[6]; // UNSURE: true object_placement_data layout
            uint32_t creation_data[6]; // 24 bytes, matches Ghidra's `local_8c`; UNSURE layout
            datum_index new_object;

            object_placement_data_initialize((object_placement_data *)placement, k_datum_index_none,
                                             k_datum_index_none); // UNSURE: role elided by Ghidra

            creation_data[0] = *(uint32_t *)&equipment->position.x;
            creation_data[1] = *(uint32_t *)&equipment->position.y;
            creation_data[2] = *(uint32_t *)&equipment->position.z;
            creation_data[3] = 0;
            *(float *)&creation_data[4] = (float)fcos(equipment->facing);
            *(float *)&creation_data[5] = (float)fsin(equipment->facing);

            new_object = object_new_with_datum_role_control((object_placement_data *)creation_data, 1);
            if (new_object != (datum_index)0xffffffff) {
                object *obj = ((object_header *)object_data->data)[new_object & 0xffff].data;

                network_index_cache_insert_if_free(placement[0]);
                object_list_membership_set(0);
                if ((*(uint8_t *)equipment & 1) != 0) {
                    obj->flags = obj->flags | 0x20;
                }
                object_type_override_call_0x68(new_object); // handle in ESI, elided by Ghidra
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x45f8f0), from tools/pack.py 0x45f8f0:

void FUN_0045f8f0(void)

{
  uint *puVar1;
  int iVar2;
  char cVar3;
  undefined4 *in_EAX;
  uint uVar4;
  byte *pbVar5;
  float10 fVar6;
  undefined4 local_98;
  undefined4 local_94;
  short local_90;
  undefined1 local_8c [24];
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  float local_58;
  float local_54;
  undefined4 local_50;

  if (*(int *)*in_EAX == 0) {
    cVar3 = FUN_004ec590();
    if ((cVar3 != '\0') &&
       (pbVar5 = (byte *)(local_90 * 0x90 + *(int *)(global_scenario + 0x388)),
       pbVar5 != (byte *)0x0)) {
      object_placement_data_initialize(local_94,0xffffffff);
      fVar6 = (float10)fcos((float10)*(float *)(pbVar5 + 0x4c));
      local_74 = *(undefined4 *)(pbVar5 + 0x40);
      local_70 = *(undefined4 *)(pbVar5 + 0x44);
      local_6c = *(undefined4 *)(pbVar5 + 0x48);
      local_50 = 0;
      local_58 = (float)fVar6;
      fVar6 = (float10)fsin((float10)*(float *)(pbVar5 + 0x4c));
      local_54 = (float)fVar6;
      uVar4 = object_new_with_datum_role_control(local_8c,1);
      if (uVar4 != 0xffffffff) {
        iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
        FUN_004e9cd0(local_98);
        FUN_004f7450(0);
        if ((*pbVar5 & 1) != 0) {
          puVar1 = (uint *)(iVar2 + 0x10);
          *puVar1 = *puVar1 | 0x20;
        }
        object_type_override_call_0x68(new_object); // handle in ESI, elided by Ghidra
        return;
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
