// object_new_with_datum_role_control
// address 0x4f54b0, size 1308 bytes
// name confidence: 0.9 (CEA/PDB match via the string "OUT OF OBJECTS: cannot create %s")
// rewrite confidence: 0.55
// evidence: types/objects.h's own "object" struct section names this exact function as the
//   source of almost every common-header field (0x000/0x004/0x008/0x00c/0x010/0x014/0x018,
//   position/velocity/forward/up/angular_velocity, cluster_stamp/cluster_index, type, name_index,
//   render_cache_slot, animation_graph/animation_index, damage_owner, placement_id,
//   parent_object/next_object/first_child_object, forced_shader_permutation,
//   node_function_values/defaults/nodes); object_placement_data (every field's offset matches
//   the param_1[n] indices used here field-for-field); types/tags.h Object (object_type, flags,
//   model/animation_graph/collision_model/creation_effect TagDependency.tag_id,
//   forced_shader_permutation_index) and GBXModel.nodes.count; types/cache.h tag_instance.
// UNSURE: the call `FUN_004f3e30(param_1)` passes the placement_data pointer, but the
//   established rewrite of 0x4f3e30 (object_type_definitions_notify_0x24, elsewhere in this
//   batch) only ever reads an object index out of a register Ghidra could not see as a
//   parameter; here it is called with the new object's index (already valid at this point in
//   the constructor) rather than with param_1, on the theory that Ghidra mis-attributed this
//   call site's visible argument. Likewise FUN_004f4080 (object_type_definitions_notify_0x38)
//   and FUN_004f3f90 (object_type_definitions_notify_0x30) are called here with no visible
//   argument, exactly like their other call sites elsewhere in this batch, and are passed the
//   object index by the same established contract. RESOLVED (was UNSURE): the two visible
//   arguments of the FUN_004f44f0 call (&DAT_00871de0, 0x7ff8) are REAL -- the disassembly at
//   0x4f4540 shows object_type_override_get_0x64 reloading both stack slots and forwarding them,
//   with the object handle in ESI, to the type's 0x64 vtable slot. They were previously dropped
//   as a decompiler artifact. ESI is loaded once at 0x4f58d2 (mov esi,ebx) and survives the
//   intervening call to 0x4f4560, so both override helpers act on the new object. UNSURE: object_set_collision_enabled's argument is
//   built with a Ghidra CONCAT3/1 trick (high 24 bits reused from an unrelated live register,
//   low byte a genuine boolean); only the boolean (whether the Object tag has a model) is
//   passed through here. UNSURE: object+0xbe (the upper half of the undocumented
//   object.unknown_0bc field) and object+0xc4 (object.creator_object) are written with
//   placement->permutation_group and placement->role respectively, on raw offsets rather than
//   named fields since types/objects.h does not split those dwords further.
// reconciled: R28 object.unknown_0c4 -> datum_index creator_object (same offset 0xc4)
// reconciled: R30 object.unknown_170 -> datum_index cached_render_state_index (same offset 0x170)
// reconciled: R27 object.unknown_00c (datum_index) -> int32_t network_update_tick (game tick stamp, -1 = never)
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)
// reconciled: R26 raw object +0x18 store -> network_position_valid
// reconciled: R28 raw object +0xc4 store -> creator_object
// reconciled: R04 0x006f1d20 uint8_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"

// TagID {index;id} is bit-identical in memory to a datum_index (low 16 bits index, high 16
// bits salt/identifier), so a TagID is reinterpreted in place wherever the object header wants
// a plain datum_index.
#define TAG_ID_AS_DATUM_INDEX(field) (*(datum_index *)&(field))

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern int32_t object_cluster_stamp; // 0x008603cc
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern char *out_of_objects_error_prefix; // 0x0065efec, the printf-style error tag argument
extern uint8_t object_new_server_broadcast_gate; // 0x0071c2c0, UNSURE: foreign global
extern int16_t network_game_mode; // 0x00719720 (also used in this batch's
                                                     //   objects_update_control_bindings)
extern uint8_t object_network_message_scratch[0x7ff8]; // 0x00871de0, the 0x7ff8-byte network
    // broadcast scratch buffer; the size is the literal pushed at 0x4f58d9

extern uint32_t game_engine_remap_placement_by_type(uint32_t handle); // 0x4630b0, EAX handle; returns it, or the remapped tag
extern datum_index object_block_data_new(data_array *array, int16_t element_size); // 0x4f7d50
extern char object_block_data_grow(int16_t field_offset, int32_t byte_count); // 0x4f7e50
extern void object_type_definitions_notify_0x24(uint32_t object_index); // 0x4f3e30, this batch
extern uint8_t object_type_definitions_query_0x28(uint32_t object_index); // 0x4f3ea0, this batch
extern void object_type_definitions_notify_0x30(uint32_t object_index); // 0x4f3f90, this batch
extern void object_type_definitions_notify_0x38(uint32_t object_index); // 0x4f4080, this batch
extern int object_type_override_get_0x64(uint32_t object_index, void *buffer, int32_t buffer_size); // 0x4f44f0, this batch; handle in ESI
extern void object_type_override_call_0x68(uint32_t object_index); // 0x4f4560, this batch; handle in ESI
extern void object_delete(uint32_t object_index); // 0x4f5bd0, this batch
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30, this batch; NULL location probes it
extern void object_set_collision_enabled(uint8_t has_model); // 0x4f6850, UNSURE: unexamined in this batch.
    // The original builds the argument with CONCAT31, so only the low byte is meaningful.
extern void object_block_data_free(data_array *array, datum_index object_index); // 0x4f7de0, UNSURE: unexamined in this batch
extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310
extern void object_notify_node_array_if_animated(void); // 0x4f8b10, UNSURE: unexamined
extern void object_apply_network_placement(real_vector3d *network_vectors); // 0x4f8b70, UNSURE: unexamined;
                                                            //   likely object_set_position_network
extern void object_refresh_region_permutations(void); // 0x4f8f50, UNSURE: unexamined
extern void object_initialize_shield_stun_thresholds(uint32_t object_index,
    float *override_max_body_vitality, float *override_max_shield_vitality); // 0x4ed440.
    // Ghidra shows a bare call here; the two override pointers are passed as NULL because that
    // is the only shape consistent with the definition. UNSURE: not visible at this call site.
extern void object_update_functions(uint32_t object_index); // 0x4f92f0, EAX -> object_index
extern void object_update_change_colors(uint32_t object_index); // 0x4f9110, EAX -> object_index
    // PHASE-4 REVIEW: both were declared (void) here because Ghidra shows no argument.
    // objdump 0x4f5835..0x4f583e shows `mov eax,ebx / call 0x4f92f0 / mov eax,ebx /
    // call 0x4f9110`, and EBX is the new object index; 0x4f92f0 and 0x4f9110 both open by
    // masking EAX with 0xffff into the object_data stride. The index is now passed.
extern void widget_new(void); // 0x4ffa80, UNSURE: called with no visible args here
extern void object_create_attachments(uint32_t object_index); // 0x4f9750
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source); // 0x4507a0, EAX creator, ECX definition
    // PHASE-4 REVIEW: declared with the six arguments this call site passes, while the other five
    // files in this module that reach 0x4507a0 declare it with an empty parameter list because
    // their own call sites show none. Unified on the empty list -- it asserts no prototype and so
    // does not contradict either set of call sites.
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, ECX server
    // 0x4e1a80, UNSURE: unexamined
extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, AL clear_first
    // src/hs/hs_compile_source.c and src/hs/hs_sound_get_gain_reference.c
extern char *strrchr(const char *str, int ch); // 0x623bc0 _strrchr
extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 _sprintf

// FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads EAX, ECX; placement, role arrive(s) on the stack (2 stack argument(s)).
// blam-cc: stack -> placement, role
datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role)
{
    datum_index definition_tag;
    datum_index new_index;
    object_header *header;
    object *obj;
    tag_instance *tag_inst;
    Object *object_tag;
    int active;
    char grew_nodes;
    uint32_t node_count;
    char out_of_objects_message[516];
    char *tag_path;
    char *last_slash;

    definition_tag = placement->definition_tag;

    if (current_game_engine != 0) {
        if (definition_tag == k_datum_index_none) {
            return k_datum_index_none;
        }
        if (role != 1 && role != 2) {
            definition_tag = game_engine_remap_placement_by_type(definition_tag); // 0x4f54ef: EAX = definition_tag
        }
    }

    if (definition_tag == k_datum_index_none) {
        return k_datum_index_none;
    }

    tag_inst = &tag_instances[definition_tag & 0xffff];
    object_tag = (Object *)tag_inst->data;

    new_index = object_block_data_new(object_data,
        object_type_definitions[object_tag->object_type]->object_size);
    if (new_index == k_datum_index_none) {
        goto out_of_objects;
    }

    header = (object_header *)object_data->data + (new_index & 0xffff);
    header->flags |= _object_header_in_pvs_pass_bit | _object_header_needs_update_bit;
    header->type = (uint8_t)object_tag->object_type;
    obj = header->data;
    obj->definition_tag = definition_tag;
    active = 1;
    obj->type = object_tag->object_type;

    object_type_definitions_notify_0x24(new_index); // UNSURE: see file header

    obj->network_role = role;
    obj->network_position_valid = 0;
    obj->network_update_tick = -1;

    obj->position = placement->position;
    obj->forward = placement->forward;
    obj->up = placement->up;
    obj->velocity = placement->velocity;
    obj->angular_velocity = placement->angular_velocity;

    obj->position.x += placement->height_above_origin * obj->up.i;
    obj->position.y += placement->height_above_origin * obj->up.j;
    obj->position.z += placement->height_above_origin * obj->up.k;

    if ((placement->flags & 1) == 0) {
        obj->flags &= ~(uint32_t)_object_mirrored_geometry_bit;
    } else {
        obj->flags |= _object_mirrored_geometry_bit;
    }

    obj->location_cluster_index = -1;
    header->cluster_index = -1;
    obj->cluster_stamp = object_cluster_stamp - 1;
    obj->damage_owner = k_datum_index_none;
    obj->placement_id = k_datum_index_none;
    obj->animation_index = -1;
    obj->animation_graph = TAG_ID_AS_DATUM_INDEX(object_tag->animation_graph.tag_id);
    obj->cached_render_state_index = -1;
    obj->parent_object = k_datum_index_none;
    obj->next_object = k_datum_index_none;
    obj->first_child_object = k_datum_index_none;
    obj->render_cache_slot = -1;
    obj->shield_damage_ticks = -1;
    obj->body_damage_ticks = -1;

    if ((object_tag->flags & 1) != 0) {
        obj->flags |= _object_definition_flag0_bit;
    }
    if (TAG_ID_AS_DATUM_INDEX(object_tag->collision_model.tag_id) == k_datum_index_none) {
        obj->flags &= ~(uint32_t)_object_has_collision_model_bit;
    } else {
        obj->flags |= _object_has_collision_model_bit;
    }

    object_set_collision_enabled(TAG_ID_AS_DATUM_INDEX(object_tag->model.tag_id) != k_datum_index_none); // UNSURE: see file header

    obj->owner_team = (int16_t)placement->owner_team;
    obj->owner_linkage = placement->owner_linkage;
    obj->creator_object = placement->role; // 0x4f5705
    *(int16_t *)((uint8_t *)obj + 0xbe) = placement->permutation_group; // UNSURE: see file header
    obj->forced_shader_permutation = (uint16_t)object_tag->forced_shader_permutation_index;

    if (TAG_ID_AS_DATUM_INDEX(object_tag->model.tag_id) == k_datum_index_none) {
        node_count = 1;
    } else {
        GBXModel *model = (GBXModel *)tag_instances[
            TAG_ID_AS_DATUM_INDEX(object_tag->model.tag_id) & 0xffff].data;
        node_count = model->nodes.count;
    }

    grew_nodes = object_block_data_grow(0x1f0, node_count * 0x34);
    if (grew_nodes == 0) {
        active = 0;
    } else if (((1 << (object_tag->object_type & 0x1f)) & _object_mask_no_node_functions) == 0) {
        grew_nodes = object_block_data_grow(0x1ec, node_count << 5);
        if (grew_nodes == 0 || (grew_nodes = object_block_data_grow(0x1e8, node_count << 5),
                                 grew_nodes == 0)) {
            active = 0;
        }
    }

    // 0x4f57c8: the original re-reads the object's data pointer here, because the
    // object_block_data_grow calls above can move the block. Everything below this point uses
    // the reloaded pointer.
    header = (object_header *)object_data->data + (new_index & 0xffff);
    obj = header->data;

    if (active && object_type_definitions_query_0x28(new_index) != 0) {
        int was_connected_to_map = (obj->flags & _object_connected_to_map_bit) != 0;

        if (was_connected_to_map && (placement->flags & 2) != 0) {
            obj->flags &= ~(uint32_t)_object_connected_to_map_bit;
        }

        object_apply_network_placement(placement->network_vectors); // UNSURE: see file header
        object_refresh_region_permutations(); // UNSURE: see file header
        object_initialize_shield_stun_thresholds(new_index, 0, 0);
        object_recalculate_bounding_radius(new_index);
        object_set_cluster_and_parent(new_index, 0);
        object_notify_node_array_if_animated(); // UNSURE: see file header
        object_type_definitions_notify_0x38(new_index); // UNSURE: see file header
        object_update_functions(new_index);
        object_update_change_colors(new_index);
        widget_new(); // UNSURE: see file header
        object_create_attachments(new_index);

        if (!was_connected_to_map) {
            obj->flags &= ~(uint32_t)_object_connected_to_map_bit;
        } else {
            obj->flags |= _object_connected_to_map_bit;
        }

        if ((header->flags & _object_header_active_bit) == 0 &&
            (obj->flags & _object_connected_to_map_bit) != 0 &&
            ((placement->flags & 2) == 0 || obj->location_cluster_index != -1)) {
            object_delete(new_index);
        }
    } else {
        active = 0;
    }

    if (object_new_server_broadcast_gate == 0 && active) {
        if (network_game_mode == 2 && obj->network_role == 0) {
            int32_t override_count;
            object_type_override_call_0x68(new_index);
            override_count = object_type_override_get_0x64(new_index, object_network_message_scratch,
                                                           sizeof object_network_message_scratch);
            if (override_count > 0) {
                network_session_broadcast_to_flagged(network_server_pointer, 1, object_network_message_scratch, 1, 0, 0, 3);
            }
        }
    } else if (!active) {
        object_type_definitions_notify_0x30(new_index); // UNSURE: see file header
        object_block_data_free(object_data, new_index); // 0x4f5956 mov eax,ds:0x8603b0 / mov edx,ebx
        new_index = k_datum_index_none;
out_of_objects:
        tag_path = tag_instances[(uint16_t)(uint32_t)definition_tag].path;
        last_slash = strrchr(tag_path, '\\');
        if (last_slash != 0) {
            tag_path = last_slash + 1;
        }
        sprintf(out_of_objects_message, "OUT OF OBJECTS: cannot create %s", tag_path);
        console_print_error_va(0, out_of_objects_error_prefix, out_of_objects_message);
        return new_index;
    }

    if (TAG_ID_AS_DATUM_INDEX(object_tag->creation_effect.tag_id) != k_datum_index_none) {
        // 0x4f591d..0x4f592a: EAX = the new object (ebx), ECX = [definition+0xac], push ebx, -1, 0, 0, 0, 0
        effect_new_on_object(new_index, TAG_ID_AS_DATUM_INDEX(object_tag->creation_effect.tag_id), new_index, -1,
            0.0f, 0.0f, (const ColorRGB *)0, (const effect_tint_source *)0);
        return new_index;
    }
    return new_index;
}

#if 0
Original Ghidra decompilation (0x4f54b0):

uint object_new_with_datum_role_control(uint *param_1,uint param_2)

{
  float *pfVar1;
  float fVar2;
  byte *pbVar3;
  uint *puVar4;
  bool bVar5;
  char cVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  uint uVar10;
  int iVar11;
  char *pcVar12;
  char *_Str;
  uint local_20c;
  char acStack_204 [516];

  local_20c = *param_1;
  if (DAT_006f1d20 != 0) {
    if (local_20c == 0xffffffff) {
      return 0xffffffff;
    }
    if ((param_2 != 1) && (param_2 != 2)) {
      local_20c = FUN_004630b0();
    }
  }
  iVar11 = DAT_008603b0;
  if (local_20c == 0xffffffff) {
    return 0xffffffff;
  }
  pbVar3 = *(byte **)((local_20c & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar7 = FUN_004f7d50(DAT_008603b0,(int)*(short *)((&PTR_PTR_0069bfdc)[*(short *)pbVar3] + 8));
  if (uVar7 == 0xffffffff) goto LAB_004f5965;
  iVar8 = (uVar7 & 0xffff) * 0xc;
  puVar4 = *(uint **)(iVar8 + 8 + *(int *)(iVar11 + 0x34));
  iVar9 = iVar8 + *(int *)(iVar11 + 0x34);
  *(byte *)(iVar9 + 2) = *(byte *)(iVar9 + 2) | 0x44;
  *(byte *)(iVar9 + 3) = *pbVar3;
  *puVar4 = local_20c;
  bVar5 = true;
  *(undefined2 *)(puVar4 + 0x2d) = *(undefined2 *)pbVar3;
  FUN_004f3e30(param_1);
  puVar4[1] = param_2;
  *(undefined1 *)(puVar4 + 6) = 0;
  puVar4[3] = 0xffffffff;
  pfVar1 = (float *)(puVar4 + 0x17);
  *pfVar1 = (float)param_1[6];
  puVar4[0x18] = param_1[7];
  puVar4[0x19] = param_1[8];
  puVar4[0x1d] = param_1[0xd];
  puVar4[0x1e] = param_1[0xe];
  puVar4[0x1f] = param_1[0xf];
  puVar4[0x20] = param_1[0x10];
  puVar4[0x21] = param_1[0x11];
  puVar4[0x22] = param_1[0x12];
  puVar4[0x1a] = param_1[10];
  puVar4[0x1b] = param_1[0xb];
  puVar4[0x1c] = param_1[0xc];
  puVar4[0x23] = param_1[0x13];
  puVar4[0x24] = param_1[0x14];
  puVar4[0x25] = param_1[0x15];
  fVar2 = (float)param_1[9];
  *pfVar1 = fVar2 * (float)puVar4[0x20] + *pfVar1;
  puVar4[0x18] = (uint)(fVar2 * (float)puVar4[0x21] + (float)puVar4[0x18]);
  puVar4[0x19] = (uint)(fVar2 * (float)puVar4[0x22] + (float)puVar4[0x19]);
  iVar11 = DAT_008603cc;
  if ((param_1[1] & 1) == 0) {
    uVar10 = puVar4[4] & 0xffffefff;
  }
  else {
    uVar10 = puVar4[4] | 0x1000;
  }
  puVar4[4] = uVar10;
  *(undefined2 *)(puVar4 + 0x27) = 0xffff;
  *(undefined2 *)(iVar9 + 4) = 0xffff;
  puVar4[5] = iVar11 - 1U;
  puVar4[0x3c] = 0xffffffff;
  puVar4[0x43] = 0xffffffff;
  *(undefined2 *)(puVar4 + 0x34) = 0xffff;
  puVar4[0x33] = *(uint *)(pbVar3 + 0x44);
  puVar4[0x5c] = 0xffffffff;
  puVar4[0x47] = 0xffffffff;
  puVar4[0x45] = 0xffffffff;
  puVar4[0x46] = 0xffffffff;
  *(undefined2 *)((int)puVar4 + 0xba) = 0xffff;
  puVar4[0x3f] = 0xffffffff;
  puVar4[0x40] = 0xffffffff;
  if ((pbVar3[2] & 1) != 0) {
    puVar4[4] = puVar4[4] | 0x40000;
  }
  if (*(int *)(pbVar3 + 0x7c) == -1) {
    uVar10 = puVar4[4] & 0xfdffffff;
  }
  else {
    uVar10 = puVar4[4] | 0x2000000;
  }
  puVar4[4] = uVar10;
  FUN_004f6850(CONCAT31((int3)(iVar11 - 1U >> 8),*(int *)(pbVar3 + 0x34) != -1));
  *(short *)(puVar4 + 0x2e) = (short)param_1[5];
  puVar4[0x30] = param_1[2];
  puVar4[0x31] = param_1[3];
  *(undefined2 *)((int)puVar4 + 0xbe) = *(undefined2 *)((int)param_1 + 0x16);
  *(undefined2 *)((int)puVar4 + 0x176) = *(undefined2 *)(pbVar3 + 0x13e);
  if (*(uint *)(pbVar3 + 0x34) == 0xffffffff) {
    uVar10 = 1;
  }
  else {
    uVar10 = (uint)*(ushort *)
                    (*(int *)((*(uint *)(pbVar3 + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                    0xb8);
  }
  cVar6 = FUN_004f7e50(0x1f0,uVar10 * 0x34);
  if (cVar6 == '\0') {
LAB_004f57a6:
    bVar5 = false;
  }
  else if ((1 << (*pbVar3 & 0x1f) & 0xfe0U) == 0) {
    cVar6 = FUN_004f7e50(0x1ec,uVar10 << 5);
    if ((cVar6 == '\0') || (cVar6 = FUN_004f7e50(0x1e8,uVar10 << 5), cVar6 == '\0'))
    goto LAB_004f57a6;
  }
  iVar11 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
  if ((bVar5) && (cVar6 = FUN_004f3ea0(uVar7), cVar6 != '\0')) {
    uVar10 = *(uint *)(iVar11 + 0x10) >> 0x13;
    if (((uVar10 & 1) != 0) && ((param_1[1] & 2) != 0)) {
      *(uint *)(iVar11 + 0x10) = *(uint *)(iVar11 + 0x10) & 0xfff7ffff;
    }
    FUN_004f8b70(param_1 + 0x16);
    FUN_004f8f50();
    FUN_004ed440();
    object_recalculate_bounding_radius(uVar7);
    FUN_004f5c30(uVar7,0);
    FUN_004f8b10();
    FUN_004f4080();
    object_update_functions();
    object_update_change_colors();
    widget_new();
    object_create_attachments(uVar7);
    if ((uVar10 & 1) == 0) {
      *(uint *)(iVar11 + 0x10) = *(uint *)(iVar11 + 0x10) & 0xfff7ffff;
    }
    else {
      *(uint *)(iVar11 + 0x10) = *(uint *)(iVar11 + 0x10) | 0x80000;
    }
    if ((((*(byte *)(iVar9 + 2) & 1) == 0) && ((*(uint *)(iVar11 + 0x10) & 0x80000) != 0)) &&
       (((param_1[1] & 2) == 0 || (*(short *)(iVar11 + 0x9c) != -1)))) {
      FUN_004f5bd0();
    }
  }
  else {
    bVar5 = false;
  }
  if ((DAT_0071c2c0 == '\0') && (bVar5)) {
    if ((DAT_00719720 == 2) && (*(int *)(iVar11 + 4) == 0)) {
      FUN_004f4560();
      iVar11 = FUN_004f44f0(&DAT_00871de0,0x7ff8);
      if (0 < iVar11) {
        FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
      }
    }
  }
  else if (!bVar5) {
    FUN_004f3f90();
    FUN_004f7de0();
    uVar7 = 0xffffffff;
LAB_004f5965:
    _Str = *(char **)((short)local_20c * 0x20 + 0x10 + DAT_0087bc14);
    pcVar12 = _strrchr(_Str,0x5c);
    if (pcVar12 != (char *)0x0) {
      _Str = pcVar12 + 1;
    }
    _sprintf(acStack_204,"OUT OF OBJECTS: cannot create %s",_Str);
    console_print_error_va(&DAT_0065efec,acStack_204);
    return uVar7;
  }
  if (*(int *)(pbVar3 + 0xac) != -1) {
    FUN_004507a0(uVar7,0xffffffff,0,0,0,0);
    return uVar7;
  }
  return uVar7;
}
#endif
