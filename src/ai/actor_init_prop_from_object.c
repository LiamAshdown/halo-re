// actor_init_prop_from_object  (Ghidra: actor_init_prop_from_object, renamed)
// address 0x43e640, size 504 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: types/ai.h prop (actor_index +0x04, pair_index +0x0c, owner_actor_index +0x1c,
// unknown_20 "copied from the object type definition at +0x284", object_index +0x18,
// unknown_4e "0x43e640 zeroes it", unknown_76 "0x43e640 sets 1000 for a vault prop", unknown_a0
// "0x43e640 sets -1", is_vault "Unit type definition byte +0x106 bit 2", is_parented "0x43e640
// sets it when the tracked object has a parent"); actor.first_prop(+0x50). phase-4 summary
// "initializes a firing-position node's fields from its target object (cluster, cover flags,
// timers) and links it into that object's node list." Calls teams_are_enemies/0x45bdb0/0x45be00
// (all outside this rewrite's range, object/unit classification helpers).
//
// Kept close to the Ghidra decompilation for the object/object-type-tag sub-fields (the
// puVar1[...] reads), which are not named at these specific offsets in types/objects.h or
// types/tags.h as read here.
//
// register convention: EAX -> object_index, EDX -> actor_index; stack -> prop_index.
//   // blam-cc: EAX -> object_index, EDX -> actor_index, stack -> prop_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#include "game.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX
extern uint8_t team_pair_flag_test(int16_t team_a, int16_t team_b); // 0x45bdb0, ECX, EDX
extern uint8_t team_pair_override_get_flag(int16_t index_a, int16_t index_b); // 0x45be00, EBX, stack

// blam-cc: EAX -> object_index, EDX -> actor_index, stack -> prop_index
void actor_init_prop_from_object(datum_index object_index, datum_index actor_index, datum_index prop_index)
{
    actor *self;
    prop *p;

    if (prop_index == (datum_index)0xffffffff) {
        return;
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    p = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));

    p->actor_index = actor_index;
    p->unknown_66 = -1;
    p->seen_state = -1;
    p->object_index = object_index;
    p->seen = 0;
    p->unknown_70 = 0.0f;
    p->unknown_b0 = -1;
    p->unknown_b8 = 0;
    p->unknown_b4 = -1;
    p->last_known_tick = -1;
    p->last_look_tick = -1;
    p->unknown_4e = 0;
    p->owner_actor_index = (datum_index)0xffffffff;
    p->pair_index = (datum_index)0xffffffff;
    p->unknown_6a = 0;
    p->unknown_a0 = -1;

    if (object_index != (datum_index)0xffffffff) {
        uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + 8 + (object_index & 0xffff) * 0xc);
        uint8_t *object_type = (uint8_t *)tag_instances[*(uint16_t *)object & 0xffff].data;
        uint8_t is_vault;

        p->object_type = ((struct object *)object)->owner_team; // 0x43e706: prop +0x12 = the object team (types/ai.h calls it object_type); the draft wrote +0x16
        // 0x43e6fb..0x43e731: CX = the object's team, DX = the actor's (+0x3e); then DX object, CX actor; then
        // BX actor, stack object
        p->is_unit = teams_are_enemies(p->object_type, ((struct actor *)self)->team);
        p->unknown_61 = team_pair_flag_test(((struct actor *)self)->team, p->object_type);
        p->unknown_62 = team_pair_override_get_flag(((struct actor *)self)->team, p->object_type);

        is_vault = (*(uint8_t *)&((struct object *)object)->vitality_flags >> 2) & 1; // 0x43e73d: the object's firing bit, not the tag's
        p->is_vault = is_vault;
        p->unknown_20 = *(float *)(object_type + 0x284);
        p->unknown_128 = (is_vault != 0) && (*(int16_t *)(object + 0x420) == 0);
        p->unknown_76 = (is_vault != 0) ? 1000 : 0;
        p->is_parented = *(int32_t *)&((struct object *)object)->owner_linkage != -1;

        if (*(int32_t *)(object + 0x1f8) == -1) {
            p->owner_actor_index = *(datum_index *)(object + 0x1f4);
        } else {
            p->has_parent = 1;
            p->owner_actor_index = *(datum_index *)(object + 0x1f8);
            p->unknown_28 = game_time->game_time;
        }

        if (p->is_parented != 0) {
            p->kind = 6;
            p->next_in_actor = self->first_prop;
            self->first_prop = prop_index;
            return;
        }
        if (p->owner_actor_index != (datum_index)0xffffffff) {
            p->kind = ((actor *)((uint8_t *)actor_data->data + (p->owner_actor_index & 0xffff) * sizeof(actor)))->type;
            p->next_in_actor = self->first_prop;
            self->first_prop = prop_index;
            return;
        }
        p->kind = -1;
    }

    p->next_in_actor = self->first_prop;
    self->first_prop = prop_index;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043e640 @ 0x43e640) ----
void FUN_0043e640(uint param_1)

{
  uint *puVar1;
  int iVar2;
  undefined1 uVar3;
  uint in_EAX;
  byte bVar4;
  uint in_EDX;
  int iVar5;
  int iVar6;

  if (param_1 != 0xffffffff) {
    iVar6 = (in_EDX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    iVar5 = (param_1 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
    *(uint *)(iVar5 + 4) = in_EDX;
    *(undefined2 *)(iVar5 + 0x66) = 0xffff;
    *(undefined2 *)(iVar5 + 0x6c) = 0xffff;
    *(uint *)(iVar5 + 0x18) = in_EAX;
    *(undefined1 *)(iVar5 + 0x74) = 0;
    *(undefined4 *)(iVar5 + 0x70) = 0;
    *(undefined2 *)(iVar5 + 0xb0) = 0xffff;
    *(undefined1 *)(iVar5 + 0xb8) = 0;
    *(undefined4 *)(iVar5 + 0xb4) = 0xffffffff;
    *(undefined4 *)(iVar5 + 0x7c) = 0xffffffff;
    *(undefined4 *)(iVar5 + 0x8c) = 0xffffffff;
    *(undefined1 *)(iVar5 + 0x4e) = 0;
    *(undefined4 *)(iVar5 + 0x1c) = 0xffffffff;
    *(undefined4 *)(iVar5 + 0xc) = 0xffffffff;
    *(undefined2 *)(iVar5 + 0x6a) = 0;
    *(undefined4 *)(iVar5 + 0xa0) = 0xffffffff;
    if (in_EAX != 0xffffffff) {
      puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
      iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      *(short *)(iVar5 + 0x12) = (short)puVar1[0x2e];
      uVar3 = FUN_0045bd50();
      *(undefined1 *)(iVar5 + 0x60) = uVar3;
      uVar3 = FUN_0045bdb0();
      *(undefined1 *)(iVar5 + 0x61) = uVar3;
      uVar3 = FUN_0045be00(*(undefined2 *)(iVar5 + 0x12));
      *(undefined1 *)(iVar5 + 0x62) = uVar3;
      bVar4 = *(byte *)((int)puVar1 + 0x106) >> 2 & 1;
      *(byte *)(iVar5 + 0x127) = bVar4;
      *(undefined4 *)(iVar5 + 0x20) = *(undefined4 *)(iVar2 + 0x284);
      if ((bVar4 == 0) || ((short)puVar1[0x108] != 0)) {
        uVar3 = 0;
      }
      else {
        uVar3 = 1;
      }
      *(undefined1 *)(iVar5 + 0x128) = uVar3;
      *(ushort *)(iVar5 + 0x76) = -(ushort)(bVar4 != 0) & 1000;
      *(bool *)(iVar5 + 0x12e) = puVar1[0x30] != 0xffffffff;
      iVar2 = DAT_006f1d6c;
      if (puVar1[0x7e] == 0xffffffff) {
        *(uint *)(iVar5 + 0x1c) = puVar1[0x7d];
      }
      else {
        *(undefined1 *)(iVar5 + 0x14) = 1;
        *(uint *)(iVar5 + 0x1c) = puVar1[0x7e];
        *(undefined4 *)(iVar5 + 0x28) = *(undefined4 *)(iVar2 + 0xc);
      }
      if (*(char *)(iVar5 + 0x12e) != '\0') {
        *(undefined2 *)(iVar5 + 0x10) = 6;
        *(undefined4 *)(iVar5 + 8) = *(undefined4 *)(iVar6 + 0x50);
        *(uint *)(iVar6 + 0x50) = param_1;
        return;
      }
      if (*(uint *)(iVar5 + 0x1c) != 0xffffffff) {
        *(undefined2 *)(iVar5 + 0x10) =
             *(undefined2 *)
              ((*(uint *)(iVar5 + 0x1c) & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34));
        *(undefined4 *)(iVar5 + 8) = *(undefined4 *)(iVar6 + 0x50);
        *(uint *)(iVar6 + 0x50) = param_1;
        return;
      }
      *(undefined2 *)(iVar5 + 0x10) = 0xffff;
    }
    *(undefined4 *)(iVar5 + 8) = *(undefined4 *)(iVar6 + 0x50);
    *(uint *)(iVar6 + 0x50) = param_1;
  }
  return;
}
#endif
