// game_engine_spawn_or_replay_netgame_equipment  (Ghidra: FUN_0045f8f0; named per
// out/phase4/game_functions.md)
// address 0x45f8f0, size 248 bytes
// VERIFIED against disassembly 0x45f8f0..0x45f9e8 (2026-09-30)
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: out/phase4/game_functions.md ("Spawns (or replays a network message spawning) a
// single netgame-equipment item at its configured position and orientation"); types/tags.h
// Scenario (netgame_equipment reflexive at +0x384/+0x388), ScenarioNetgameEquipment (0x90 bytes,
// position at +0x40, facing at +0x4c); types/objects.h object (flags +0x10).
// register convention: the network message record in EAX.
//   // blam-cc: EAX -> message
// FIXED 2026-09-30 (disassembly): the original decodes into a 12-byte block {object_hash, definition_tag, equipment_index (int16 at +8)}
// (the earlier draft decoded into a lone int16 and never used the decoded tag), initialises ONE object_placement_data
// with (decoded tag, role -1), fills position (scenario equipment +0x40), forward = (cos(facing), sin(facing), 0) into it, and
// passes that same placement to object_new_with_datum_role_control; the draft passed a second, unrelated 24-byte array.
// network_index_cache_insert_if_free takes EAX = 0x6870d8 (network_object_index_cache), ECX = the new object, stack = the
// decoded object_hash; object_list_membership_set(ECX = new object, stack 0).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Scenario *global_scenario; // 0x00746f8c
extern data_array *object_data; // 0x008603b0

extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590, blam-cc:
    // EAX -> event, ECX -> out_values; UNSURE identity (a network message-delta decode)
extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670, blam-cc: EAX -> event; UNSURE identity
extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0, canonical form (src/items)
extern datum_index object_new_with_datum_role_control(object_placement_data *placement,
    uint32_t role); // 0x4f54b0
extern uint8_t network_object_index_cache[]; // 0x006870d8
extern uint8_t network_index_cache_insert_if_free(uint8_t *container, int32_t slot, int32_t key); // 0x4e9cd0, EAX container, ECX key, stack slot
extern void object_list_membership_set(uint32_t object_index, char add); // 0x4f7450, ECX object_index, stack add
extern void object_type_override_call_0x68(uint32_t object_index); // 0x4f4560, objects module; handle in ESI

extern double fcos(double radians); // a single x87 FCOS instruction (see src/game/vector3d_clamp_length.c for the same sqrt idiom)
extern double fsin(double radians); // a single x87 FSIN instruction

// blam-cc: EAX -> message
// If `*message == 0`, decodes the message and spawns the netgame-equipment item it names locally at its
// scenario-configured position/facing; otherwise (`*message != 0`) treats this as a networked replay and defers
// entirely to message_delta_decode_compound_field_staged.
typedef struct netgame_equipment_spawn_message {
    int32_t object_hash;            // 0x00 the network id registered for the new object
    datum_index definition_tag;     // 0x04 the equipment tag to create
    int16_t equipment_index;        // 0x08 index into scenario netgame_equipment
    int16_t pad_0a;                 // 0x0a
} netgame_equipment_spawn_message;  // size 0x0c

void game_engine_spawn_or_replay_netgame_equipment(int32_t *message)
{
    netgame_equipment_spawn_message decoded;
    ScenarioNetgameEquipment *equipment;
    object_placement_data placement;
    datum_index new_object;

    if (*(int32_t *)*(int32_t **)message != 0) {
        message_delta_decode_compound_field_staged(message);
        return;
    }
    if (message_delta_decode_compound_field(message, &decoded) == 0) {
        return;
    }

    equipment = &((ScenarioNetgameEquipment *)global_scenario->netgame_equipment.pointer)[decoded.equipment_index];
    if (equipment == 0) { // the original tests the computed address (0x45f92f), not the base pointer
        return;
    }

    object_placement_data_initialize(&placement, decoded.definition_tag, k_datum_index_none);
    placement.position.x = equipment->position.x;
    placement.position.y = equipment->position.y;
    placement.position.z = equipment->position.z;
    placement.forward.i = (float)fcos(equipment->facing);
    placement.forward.j = (float)fsin(equipment->facing);
    placement.forward.k = 0.0f;

    new_object = object_new_with_datum_role_control(&placement, 1);
    if (new_object != (datum_index)0xffffffff) {
        object *obj = ((object_header *)object_data->data)[new_object & 0xffff].data;

        network_index_cache_insert_if_free(network_object_index_cache, decoded.object_hash, (int32_t)new_object);
        object_list_membership_set(new_object, 0);
        if ((*(uint8_t *)equipment & 1) != 0) {
            obj->flags = obj->flags | 0x20;
        }
        object_type_override_call_0x68(new_object); // handle in ESI
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
