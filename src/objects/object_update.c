// object_update  (Ghidra: object_update, already named)
// address 0x4f7ef0, size 465 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Performs the per-tick update of an object and recursively updates
//   every object attached to it")
// rewrite confidence: 0.45
// evidence: types/objects.h object_header (flags 0x02 with _object_header_just_created_bit),
//   object (flags 0x10 with _object_in_tracked_list_bit, unknown_0d4 0x0d4,
//   node_function_count 0x0d6, first_child_object 0x118, next_object 0x114,
//   parent_object 0x11c, velocity 0x068, angular_velocity 0x08c, unknown_008 0x008);
//   types/tags.h Object (model, collision_model TagDependency); global 0x008603b0 object_data,
//   0x0087bc14 tag_instances, 0x006b8cbc object_globals_pointer, 0x00696714 the shared zero
//   vector (types/math.h); callees object_type_definitions_query_0x34 (0x4f4000, established:
//   EAX -> object_index), object_type_definitions_notify_0x38 (0x4f4080, established:
//   EBX -> object_index), object_update_vitality_and_regeneration (established),
//   object_recalculate_bounding_radius (established), object_update_functions and
//   object_update_change_colors (0x4f92f0/0x4f9110, this batch, EAX assumed pending their own
//   rewrite), object_for_each_light_attachment (0x4f9a20, this batch), object_notify_node_array_if_animated (0x4f8b10,
//   this batch).
// register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
//   "object_update(uint param_1)"), reused as EAX/EBX at various callee call sites within the
//   body per each callee's own established convention.
// UNSURE: object flags bits 0x00800000 and 0x00002000 used here are not in
//   types/objects.h's object_flags enum (which only documents the bits this module's OTHER
//   functions read/write); preserved as raw hex. UNSURE: DAT_00719720 (a foreign game-mode/role
//   global) and the byte at object+0x008 (an "at rest to within epsilon of zero velocity" cache
//   flag, distinct from the documented _object_at_rest_bit) are both left unnamed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern object_globals *object_globals_pointer; // 0x006b8cbc
extern int16_t game_mode_or_role; // 0x00719720 (a WORD; 0x719722 is the screenshot counter), UNSURE: foreign global
extern real_vector3d *shared_zero_vector; // 0x00696714, a pointer variable per types/math.h
    // ("global_origin3d_pointer"); matches src/objects/flag_new.c's declaration style for the
    // same address.
extern float fabsf(float x); // x87 FABS

extern uint8_t object_type_definitions_query_0x34(uint32_t object_index); // 0x4f4000, EAX -> object_index
extern void object_type_definitions_notify_0x38(uint32_t object_index); // 0x4f4080, EBX -> object_index
extern void object_update_vitality_and_regeneration(uint32_t object_index); // established
extern void object_recalculate_bounding_radius(uint32_t object_index); // established
extern void object_update_functions(uint32_t object_index); // 0x4f92f0, this batch, UNSURE: EAX assumed
extern void object_update_change_colors(uint32_t object_index); // 0x4f9110, this batch, UNSURE: EAX assumed
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); // 0x4f9a20, this batch
extern void object_notify_node_array_if_animated(uint32_t object_index); // 0x4f8b10, this batch, EAX -> object_index

uint8_t object_update(uint32_t object_index)
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    if ((header->flags & _object_header_just_created_bit) != 0) {
        return 1;
    }

    if ((obj->flags & _object_in_tracked_list_bit) != 0) {
        object_globals_pointer->unknown_04 = object_globals_pointer->unknown_04 + 1;
    }

    if (obj->node_function_count != 0) {
        obj->unknown_0d4 = obj->unknown_0d4 + 1;
        if (obj->node_function_count <= obj->unknown_0d4) {
            obj->node_function_count = 0;
        }
    }

    object_type_definitions_query_0x34(object_index);

    if (definition->collision_model.tag_id.index != 0xffff) {
        object_update_vitality_and_regeneration(object_index);
    }

    object_type_definitions_notify_0x38(object_index);

    if ((obj->flags & 0x800000) == 0) { // UNSURE: undocumented bit, see file header
        object_recalculate_bounding_radius(object_index);
    }

    object_update_functions(object_index);
    object_update_change_colors(object_index);

    if (((obj->flags & 0x2000) != 0) && // UNSURE: undocumented bit, see file header
        (((obj->flags & _object_no_collision_bit) == 0) || (definition->model.tag_id.index == 0xffff))) {
        object_for_each_light_attachment(object_index, 1, 1);
    }

    if (obj->first_child_object != k_datum_index_none) {
        object_update(obj->first_child_object);
    }
    if ((obj->parent_object != k_datum_index_none) && (obj->next_object != k_datum_index_none)) {
        object_update(obj->next_object);
    }

    object_notify_node_array_if_animated(object_index);

    if (game_mode_or_role == 2) {
        uint8_t *at_rest_flag = (uint8_t *)&obj->unknown_008;
        if ((fabsf(obj->velocity.i - shared_zero_vector->i) < 0.0001f) &&
            (fabsf(obj->velocity.j - shared_zero_vector->j) < 0.0001f) &&
            (fabsf(obj->velocity.k - shared_zero_vector->k) < 0.0001f) &&
            (fabsf(obj->angular_velocity.i - shared_zero_vector->i) < 0.0001f) &&
            (fabsf(obj->angular_velocity.j - shared_zero_vector->j) < 0.0001f) &&
            (fabsf(obj->angular_velocity.k - shared_zero_vector->k) < 0.0001f)) {
            *at_rest_flag = 1;
            return 1;
        }
        *at_rest_flag = 0;
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x4f7ef0):

undefined4 object_update(uint param_1)

{
  int iVar1;
  uint *puVar2;
  int iVar3;

  iVar1 = *(int *)(DAT_008603b0 + 0x34) + (param_1 & 0xffff) * 0xc;
  puVar2 = *(uint **)(iVar1 + 8);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((*(byte *)(iVar1 + 2) & 0x10) == 0) {
    if ((puVar2[4] & 0x10000) != 0) {
      *(short *)(DAT_006b8cbc + 4) = *(short *)(DAT_006b8cbc + 4) + 1;
    }
    if ((*(short *)((int)puVar2 + 0xd6) != 0) &&
       (*(short *)(puVar2 + 0x35) = (short)puVar2[0x35] + 1,
       *(short *)((int)puVar2 + 0xd6) <= (short)puVar2[0x35])) {
      *(undefined2 *)((int)puVar2 + 0xd6) = 0;
    }
    FUN_004f4000(param_1);
    if (*(int *)(iVar3 + 0x7c) != -1) {
      object_update_vitality_and_regeneration(param_1);
    }
    FUN_004f4080();
    if ((puVar2[4] & 0x800000) == 0) {
      object_recalculate_bounding_radius(param_1);
    }
    object_update_functions();
    object_update_change_colors();
    if (((puVar2[4] & 0x2000) != 0) &&
       (((puVar2[4] & 1) == 0 ||
        (*(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x34) == -1)))) {
      object_for_each_light_attachment(1,1);
    }
    if (puVar2[0x46] != 0xffffffff) {
      object_update(puVar2[0x46]);
    }
    if ((puVar2[0x47] != 0xffffffff) && (puVar2[0x45] != 0xffffffff)) {
      object_update(puVar2[0x45]);
    }
    FUN_004f8b10();
    if (DAT_00719720 == 2) {
      if (ABS((float)puVar2[0x1a] - *(float *)PTR_DAT_00696714) < 0.0001) {
        if (ABS((float)puVar2[0x1b] - *(float *)(PTR_DAT_00696714 + 4)) < 0.0001) {
          if (ABS((float)puVar2[0x1c] - *(float *)(PTR_DAT_00696714 + 8)) < 0.0001) {
            if (ABS((float)puVar2[0x23] - *(float *)PTR_DAT_00696714) < 0.0001) {
              if (ABS((float)puVar2[0x24] - *(float *)(PTR_DAT_00696714 + 4)) < 0.0001) {
                if (ABS((float)puVar2[0x25] - *(float *)(PTR_DAT_00696714 + 8)) < 0.0001) {
                  *(undefined1 *)(puVar2 + 2) = 1;
                  return 1;
                }
              }
            }
          }
        }
      }
      *(undefined1 *)(puVar2 + 2) = 0;
    }
  }
  return 1;
}
#endif
