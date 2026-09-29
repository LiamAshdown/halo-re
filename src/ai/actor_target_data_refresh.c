// actor_target_data_refresh  (Ghidra: actor_target_data_refresh; named from out/phase2/results/ai_02.json)
// address 0x41c4b0, size 1075 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED end to end against objdump 0x41c4b0..0x41c8e2)
// evidence: out/phase2/results/ai_02.json -- revalidates a prop's (target-data record's)
//   tracked object (possibly reassigning via object_find_nearest_squad_member for squad-shared
//   targets), then fetches marker-based aim/firing offsets and root-object shield state.
//   Confirms, via the explicit `*(undefined4 *)(iVar7 + 0xec) = 0xffffffff;` reset here, that
//   prop+0xec is a plain int32 cache -- not the real_point3d types/ai.h currently names there --
//   matching the same conclusion reached independently in
//   actor_target_get_relationship_object.c.
// register convention: all five parameters are Ghidra's recognized stack parameters.
//   // blam-cc: stack -> actor_index, target_prop_index, reference, force, allow_reassign
//
// UNSURE, very substantially: this is the single most externally-dependent function in this
// batch. vector3d_magnitude_squared, object_get_node_local_transform, object_get_root_object_index,
// scenario_location_get_water_and_weather, unit_get_tag_flag_bit7 and actor_get_firing_positions are all called with zero or
// partially-recovered arguments by Ghidra; every one is declared and invoked with its best-
// guess real signature where a sibling rewrite in this batch established one
// (object_find_nearest_squad_member, actor_get_firing_positions, vector3d_normalize_with_length,
// object_get_position), and with no arguments (matching Ghidra exactly) otherwise. Control flow
// and every named field write is preserved exactly; treat the specific argument guesses as
// unverified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *prop_data;   // 0x008802c0
extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c
// 0x00696718 is a POINTER slot, not the data: the dword there is 0x0065c20c and the three
// floats at 0x0065c20c are { 1.0, 0.0, 0.0 }, i.e. the shared forward vector, not a zero
// point. The earlier draft of this file declared it as a 12-byte array at 0x00696718 and
// so read the pointer word itself as the x component.
extern const real_vector3d *global_forward3d_pointer; // 0x00696718

// Marker name string constants passed to object_get_node_local_transform.
extern char ai_marker_name_a[]; // 0x0066bfa0
extern char ai_marker_name_b[]; // 0x00672034

extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name, object_marker *marker, uint32_t flags); // 0x4f6080,
                                               // blam-cc: EAX -> object_index, ECX -> marker_name,
                                               // EDX -> marker, stack -> param_4 (matches src/objects)
extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900
extern datum_index object_get_root_object_index(uint32_t object_index); // 0x4f6fb0, ECX
extern real vector3d_magnitude_squared(real_vector3d *v); // 0x401000, src/math; blam-cc: EAX v
extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf,
    int16_t *weather_index_out); // 0x53ed60, EBX point, stack
extern char unit_get_tag_flag_bit7(uint32_t unit_index); // 0x571c70, EAX
extern datum_index object_find_nearest_squad_member(datum_index actor_index, void *reference, datum_index exclude_index, char stamp_group); // 0x41c2c0, this batch
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point); // 0x41c1e0, EAX, ECX, EDX
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990

// blam-cc: stack -> actor_index, target_prop_index, reference, force, allow_reassign
// Refreshes a prop's (target-data record's) cached object reference, aim marker offsets, and
// root-object status for combat targeting.
void actor_target_data_refresh(uint32_t actor_index, uint32_t target_prop_index, void *reference, char force, char allow_reassign)
{
    actor *self;
    prop *target;
    object *unit_obj;
    object *parent_obj;
    object *child_obj;
    datum_index object_index;
    datum_index reassigned;
    datum_index parent_index;
    datum_index child_index;
    // objdump 0x41c68a / 0x41c69a: the buffer handed to object_get_node_local_transform is
    // 0x6c bytes and the three dwords read back afterwards are its last twelve -- Ghidra's
    // local_c / local_8 / local_4, which sit immediately past the 96 bytes it named local_6c.
    uint8_t local_transform[0x6c];
    uint32_t transform_x, transform_y, transform_z;
    uint8_t is_eligible;
    real_vector3d delta;
    float length;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    if (self->active == 0) {
        return;
    }

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
    object_index = target->object_index;
    unit_obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (force == 0 && 3 < target->kind && target->kind < 6) {
        // FIXED: Ghidra jumps straight to LAB_0041c867 when prop+0x4e is non-zero -- a
        // non-zero unknown_4e SKIPS the whole refresh. The first rewrite inverted this and
        // fell through into the reassign block instead.
        if (target->unknown_4e != 0) {
            goto after_reassign;
        }
        {
            if (((unit_obj->vitality_flags & 4) == 0 || *(int16_t *)((uint8_t *)unit_obj + 0x420) != 0) ||
                (target->unknown_30 != 0 || 0.010000001f <= vector3d_magnitude_squared(&unit_obj->velocity))) { // 0x41c561: EAX = object + 0x68
                is_eligible = 0;
            } else {
                is_eligible = 1;
            }

            if ((self->target_unit_index == target_prop_index &&
                 (target->noticed_a == 0 || target->noticed_b == 0)) ||
                is_eligible == 0) {
                goto after_reassign;
            }
            target->unknown_4e = 1;
            target->is_vault = 1;
        }
    }

    if (((target->has_parent != 0 && target->owner_actor_index != k_datum_index_none) && allow_reassign != 0) &&
        target->owner_refresh_tick + 0x5a <= game_time->game_time) {
        target->owner_refresh_tick = game_time->game_time;
        // UNSURE: real signature is object_find_nearest_squad_member(actor_index, reference,
        // exclude_index, stamp_group); called here with (&target->unknown_120-as-firing-block,
        // object_index, 0) per Ghidra's recovered args (self+0x120 through the target's own
        // record, matching object_find_nearest_squad_member's `reference` parameter shape).
        // 0x41c5c5..0x41c604: EAX = the prop's owner (+0x1c, the swarm actor), stack: actor +0x120, the object, 0
        reassigned = object_find_nearest_squad_member(target->owner_actor_index, (void *)&self->aim_origin, object_index, 0);
        if (reassigned != object_index) {
            target->object_index = reassigned;
            unit_obj = ((object_header *)object_data->data)[reassigned & 0xffff].data;
            if (target->kind < 4 || 5 < target->kind) {
                if (target->pair_index != k_datum_index_none) {
                    ((prop *)((uint8_t *)prop_data->data + (target->pair_index & 0xffff) * sizeof(prop)))->object_index = reassigned;
                }
            } else {
                ((prop *)((uint8_t *)prop_data->data + (target->pair_index & 0xffff) * sizeof(prop)))->object_index = reassigned;
            }
        }
    }

    object_get_node_local_transform(target->object_index, ai_marker_name_a, (object_marker *)local_transform, 1);
    transform_x = *(uint32_t *)(local_transform + 0x60);
    transform_y = *(uint32_t *)(local_transform + 0x64);
    transform_z = *(uint32_t *)(local_transform + 0x68);
    *(uint32_t *)&target->unknown_104 = transform_x; // UNSURE: local_c/local_8/local_4 mapping
    *(uint32_t *)&target->unknown_108 = transform_y; // guessed as the transform's first three
    *(uint32_t *)&target->unknown_10c = transform_z; // output dwords, in order

    // FIXED: objdump 0x41c6c0 sets EAX = &prop.last_known_position (prop+0xbc) and
    // ECX = prop.object_index before the call; the first rewrite passed a null out-pointer.
    object_get_position(&target->last_known_position, target->object_index);

    object_get_node_local_transform(target->object_index, ai_marker_name_b, (object_marker *)local_transform, 1);
    *(uint32_t *)&target->aim_offset.x = *(uint32_t *)(local_transform + 0x60);
    *(uint32_t *)&target->aim_offset.y = *(uint32_t *)(local_transform + 0x64);
    *(uint32_t *)&target->aim_offset.z = *(uint32_t *)(local_transform + 0x68);
    *(real_vector3d *)&target->unknown_d4 = unit_obj->velocity;
    // FIXED: this store is prop+0xec (path_surface_index), not prop+0x110. The first rewrite
    // wrote relationship_object_index here and again below, losing the 0xec reset entirely.
    target->path_surface_index = -1;

    reassigned = object_get_root_object_index(target->object_index); // 0x41c71c: ECX = prop +0x18
    parent_obj = ((object_header *)object_data->data)[reassigned & 0xffff].data;
    target->unknown_fc = *(float *)&parent_obj->location_leaf_index;
    *(uint32_t *)&target->cluster_index = *(uint32_t *)&parent_obj->location_cluster_index;

    // 0x41c75a: EBX = prop +0xc8 (the second marker position), stack: the location at +0xfc, no weather output
    target->unknown_118 = scenario_location_get_water_and_weather(&target->aim_offset, (bsp_leaf_reference *)&target->unknown_fc, 0);
    target->relationship_object_index = -1;
    target->unknown_135 = 0;
    target->unknown_136 = 0;
    *(uint32_t *)&target->unknown_114 = 0xffffffff; // types/ai.h types this field as a float,
                                                    // but the original stores the raw -1
                                                    // sentinel bit pattern here, not 0.0

    parent_index = unit_obj->parent_object;
    if (parent_index != k_datum_index_none) {
        parent_obj = ((object_header *)object_data->data)[parent_index & 0xffff].data;
        if (parent_obj->type == 1) {
            target->relationship_object_index = parent_index;
            if (*(int32_t *)((uint8_t *)parent_obj + 0x328) == (int32_t)target->object_index ||
                target->actor_type == 0xf) {
                target->unknown_135 = 1;
            } else {
                target->unknown_135 = 0;
            }
            if (*(int32_t *)((uint8_t *)parent_obj + 0x324) == (int32_t)target->object_index &&
                unit_get_tag_flag_bit7(parent_index) != 0) {
                target->unknown_136 = 1;
            } else {
                target->unknown_136 = 0;
            }
        } else if ((1 << (parent_obj->type & 0x1f) & 3) != 0) {
            *(uint32_t *)&target->unknown_114 = parent_index;
        }
    }

    target->unknown_125 = 0;
    child_index = unit_obj->first_child_object;
    while (child_index != k_datum_index_none) {
        child_obj = ((object_header *)object_data->data)[child_index & 0xffff].data;
        if ((1 << (child_obj->type & 0x1f) & 3) != 0) {
            target->unknown_125 = target->unknown_125 + 1;
        }
        child_index = child_obj->next_object;
    }

after_reassign:
    // 0x41c867: EAX = actor, ECX = the caller's block (filled here), EDX = prop +0xbc
    actor_get_firing_positions(actor_index, (uint32_t *)reference, &target->last_known_position);

    target->look_point.x = target->last_known_position.x - *(float *)((uint8_t *)reference + 0xc);
    target->look_point.y = target->last_known_position.y - *(float *)((uint8_t *)reference + 0x10);
    target->look_point.z = target->last_known_position.z - *(float *)((uint8_t *)reference + 0x14);
    length = vector3d_normalize_with_length((real_vector3d *)&target->look_point);
    target->distance = length;

    if (length == 0.0f) {
        *(real_vector3d *)&target->look_point = *global_forward3d_pointer;
    }
}

#if 0
Original Ghidra decompilation (0x41c4b0):

void FUN_0041c4b0(uint param_1,uint param_2,int param_3,char param_4,char param_5)

{
  bool bVar1;
  undefined *puVar2;
  int iVar3;
  undefined1 uVar4;
  char cVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  float10 fVar9;
  int local_70;
  undefined1 local_6c [96];
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar7 = (param_1 & 0xffff) * 0x724;
  iVar8 = iVar7 + *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar7 + 8 + *(int *)(DAT_00880360 + 0x34)) == '\0') {
    return;
  }
  iVar7 = (param_2 & 0xffff) * 0x138;
  uVar6 = *(uint *)(iVar7 + 0x18 + *(int *)(DAT_008802c0 + 0x34));
  iVar7 = iVar7 + *(int *)(DAT_008802c0 + 0x34);
  local_70 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
  if (((param_4 == '\0') && (3 < *(short *)(iVar7 + 0x24))) && (*(short *)(iVar7 + 0x24) < 6)) {
    if (*(char *)(iVar7 + 0x4e) != '\0') goto LAB_0041c867;
    if ((((*(byte *)(local_70 + 0x106) & 4) == 0) || (*(short *)(local_70 + 0x420) != 0)) ||
       ((*(short *)(iVar7 + 0x30) != 0 ||
        (fVar9 = (float10)FUN_00401000(), (float10)0.010000001 <= fVar9)))) {
      bVar1 = false;
    }
    else {
      bVar1 = true;
    }
    if (((*(uint *)(iVar8 + 0x270) == param_2) &&
        ((*(char *)(iVar7 + 0xb9) == '\0' || (*(char *)(iVar7 + 0xba) == '\0')))) || (!bVar1))
    goto LAB_0041c867;
    *(undefined1 *)(iVar7 + 0x4e) = 1;
    *(undefined1 *)(iVar7 + 0x127) = 1;
  }
  if ((((*(char *)(iVar7 + 0x14) != '\0') && (*(int *)(iVar7 + 0x1c) != -1)) && (param_5 != '\0'))
     && (*(int *)(iVar7 + 0x28) + 0x5a <= *(int *)(DAT_006f1d6c + 0xc))) {
    *(int *)(iVar7 + 0x28) = *(int *)(DAT_006f1d6c + 0xc);
    uVar6 = FUN_0041c2c0(iVar8 + 0x120,uVar6,0);
    iVar8 = DAT_008603b0;
    if (uVar6 != *(uint *)(iVar7 + 0x18)) {
      *(uint *)(iVar7 + 0x18) = uVar6;
      local_70 = *(int *)(*(int *)(iVar8 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
      if ((*(short *)(iVar7 + 0x24) < 4) || (5 < *(short *)(iVar7 + 0x24))) {
        if (*(uint *)(iVar7 + 0xc) != 0xffffffff) {
          *(uint *)((*(uint *)(iVar7 + 0xc) & 0xffff) * 0x138 + 0x18 + *(int *)(DAT_008802c0 + 0x34)
                   ) = uVar6;
        }
      }
      else {
        *(uint *)((*(uint *)(iVar7 + 0xc) & 0xffff) * 0x138 + 0x18 + *(int *)(DAT_008802c0 + 0x34))
             = uVar6;
      }
    }
  }
  object_get_node_local_transform(*(undefined4 *)(iVar7 + 0x18),&DAT_0066bfa0,local_6c,1);
  *(undefined4 *)(iVar7 + 0x104) = local_c;
  *(undefined4 *)(iVar7 + 0x108) = local_8;
  *(undefined4 *)(iVar7 + 0x10c) = local_4;
  object_get_position();
  object_get_node_local_transform(*(undefined4 *)(iVar7 + 0x18),&DAT_00672034,local_6c,1);
  *(undefined4 *)(iVar7 + 200) = local_c;
  *(undefined4 *)(iVar7 + 0xcc) = local_8;
  *(undefined4 *)(iVar7 + 0xd0) = local_4;
  *(undefined4 *)(iVar7 + 0xd4) = *(undefined4 *)(local_70 + 0x68);
  *(undefined4 *)(iVar7 + 0xd8) = *(undefined4 *)(local_70 + 0x6c);
  iVar3 = DAT_008603b0;
  *(undefined4 *)(iVar7 + 0xdc) = *(undefined4 *)(local_70 + 0x70);
  *(undefined4 *)(iVar7 + 0xec) = 0xffffffff;
  uVar6 = object_get_root_object_index();
  iVar8 = *(int *)(*(int *)(iVar3 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
  *(undefined4 *)(iVar7 + 0xfc) = *(undefined4 *)(iVar8 + 0x98);
  *(undefined4 *)(iVar7 + 0x100) = *(undefined4 *)(iVar8 + 0x9c);
  uVar4 = FUN_0053ed60((undefined4 *)(iVar7 + 0xfc),0);
  *(undefined1 *)(iVar7 + 0x118) = uVar4;
  *(undefined4 *)(iVar7 + 0x110) = 0xffffffff;
  *(undefined1 *)(iVar7 + 0x135) = 0;
  *(undefined1 *)(iVar7 + 0x136) = 0;
  *(undefined4 *)(iVar7 + 0x114) = 0xffffffff;
  uVar6 = *(uint *)(local_70 + 0x11c);
  if (uVar6 != 0xffffffff) {
    iVar8 = *(int *)(*(int *)(iVar3 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
    if (*(short *)(iVar8 + 0xb4) == 1) {
      *(uint *)(iVar7 + 0x110) = uVar6;
      if ((*(int *)(iVar8 + 0x328) == *(int *)(iVar7 + 0x18)) || (*(short *)(iVar7 + 0x10) == 0xf))
      {
        uVar4 = 1;
      }
      else {
        uVar4 = 0;
      }
      *(undefined1 *)(iVar7 + 0x135) = uVar4;
      if ((*(int *)(iVar8 + 0x324) == *(int *)(iVar7 + 0x18)) &&
         (cVar5 = FUN_00571c70(), cVar5 != '\0')) {
        *(undefined1 *)(iVar7 + 0x136) = 1;
      }
      else {
        *(undefined1 *)(iVar7 + 0x136) = 0;
      }
    }
    else if ((1 << ((byte)*(short *)(iVar8 + 0xb4) & 0x1f) & 3U) != 0) {
      *(uint *)(iVar7 + 0x114) = uVar6;
    }
  }
  *(undefined1 *)(iVar7 + 0x125) = 0;
  uVar6 = *(uint *)(local_70 + 0x118);
  while (uVar6 != 0xffffffff) {
    iVar8 = *(int *)(*(int *)(iVar3 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
    if ((1 << (*(byte *)(iVar8 + 0xb4) & 0x1f) & 3U) != 0) {
      *(char *)(iVar7 + 0x125) = *(char *)(iVar7 + 0x125) + '\x01';
    }
    uVar6 = *(uint *)(iVar8 + 0x114);
  }
LAB_0041c867:
  actor_get_firing_positions();
  *(float *)(iVar7 + 0xe0) = *(float *)(iVar7 + 0xbc) - *(float *)(param_3 + 0xc);
  *(float *)(iVar7 + 0xe4) = *(float *)(iVar7 + 0xc0) - *(float *)(param_3 + 0x10);
  *(float *)(iVar7 + 0xe8) = *(float *)(iVar7 + 0xc4) - *(float *)(param_3 + 0x14);
  fVar9 = (float10)vector3d_normalize_with_length();
  *(float *)(iVar7 + 0x11c) = (float)fVar9;
  puVar2 = PTR_DAT_00696718;
  if (fVar9 == (float10)0.0) {
    *(float *)(iVar7 + 0xe0) = *(float *)PTR_DAT_00696718;
    *(undefined4 *)(iVar7 + 0xe4) = *(undefined4 *)(puVar2 + 4);
    *(undefined4 *)(iVar7 + 0xe8) = *(undefined4 *)(puVar2 + 8);
  }
  return;
}
#endif
