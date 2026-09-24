// unit_drop_object_from_hand  (Ghidra: already named unit_drop_object_from_hand)
// address 0x56ed00, size 596 bytes
// name confidence: 0.55 (cea-pdb hint via the "left hand" string; functions.md summary matches)
// rewrite confidence: 0.25 -- the randomized-toss velocity block (the bulk of the arithmetic
//   between the object_reorient_relative_to_marker call and item_accelerate) could not be
//   pinned to named fields; its Ghidra locals are preserved verbatim rather than renamed. See
//   UNSURE notes.
// evidence: types/objects.h object.parent_object (0x11c), .flags (0x010), .velocity (0x068),
//   .angular_velocity (0x08c), object_header.flags (0x02); types/units.h unit_data.flags
//   (0x204, bit 0x100000 = delete_when_dropped); types/tags.h Object.model (a TagDependency at
//   0x28, whose TagID lands at relative +0xc = absolute 0x34); callees
//   object_snap_to_parent_marker_and_detach (0x4f6610), object_for_each_light_attachment
//   (0x4f9a20, established EAX=object_index), object_reorient_relative_to_marker (0x4f6180,
//   established stack args parent_index/parent_marker_name), vector3d_randomize_direction
//   (0x4cd1b0, established stack args lo/hi), item_accelerate (0x4bd080, established), and
//   object_delete_recursive (0x4f59d0, established).
// register convention: unit object index in EAX (param_1), dropped object index on the stack
//   (param_2).
//   // blam-cc: EAX -> unit_index, stack -> dropped_object_index
// UNSURE: object_for_each_light_attachment's and object_reorient_relative_to_marker's register
//   arguments (object_index / object_marker_name) are not visible at these call sites; read as
//   the dropped object and a null (own-origin) marker name respectively.
// UNSURE: vector3d_randomize_direction's direction/out/seed registers are not visible; direction
//   is guessed as the dropped object's own forward vector, out and the arithmetic that follows
//   are preserved as literal Ghidra locals (local_24/local_20/local_1c/local_18/local_14/
//   local_10) rather than invented named fields, since the intended meaning of the resulting
//   blended vector could not be determined from this decompile alone.
// UNSURE: object_get_root_object_velocities (a zero-argument float helper, presumably a random distance/scalar) and
//   the object_delete_unparented / object_delete register arguments are guessed as the dropped
//   object index, matching every other call in this function.
// reconciled: R04 0x006f1d20 int32_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern real_point3d *global_origin3d_pointer; // 0x00696714
extern random_seed random_seed_global;    // 0x00719cd0
extern game_engine_definition *current_game_engine;  // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)

extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
                                              int32_t invoke_callback); // 0x4f9a20
extern void object_reorient_relative_to_marker(uint32_t parent_index, char *parent_marker_name,
                                                uint32_t object_index, char *object_marker_name); // 0x4f6180
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // 0x4f6610
extern real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out,
    random_seed *seed, real lo, real hi); // 0x4cd1b0
extern float object_get_root_object_velocities(void); // 0x4f6aa0, UNSURE: presumably a random distance/scalar  // real signature (object_get_root_object_velocities.c): void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity, real_vector3d *out_angular_velocity); Ghidra recovered 0 of 3 args at this call site
extern void item_accelerate(real_vector3d *impulse, int32_t param_2); // 0x4bd080
extern void unit_get_camera_position(uint32_t unit_index, real_point3d *out); // 0x568f80, UNSURE signature
extern uint8_t object_reposition_to_spawn_location(uint32_t object_index, real_point3d *target_position); // 0x4f7b70, UNSURE args
extern void object_delete(uint32_t object_index);            // 0x4f5bd0, UNSURE exact signature
extern void object_delete_unparented(uint32_t object_index); // 0x4f5aa0, UNSURE exact signature
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0

// Detaches a held object (typically a weapon or grenade) from the unit's hand and releases it
// into the world with a small randomized toss. If the object was already unparented on entry,
// first registers it into a cluster, tears down any light attachment its tag defines, and
// re-orients it relative to the unit's "left hand" marker. Then unconditionally clears the low
// two bits of the dropped item's own state word, detaches it from the hand marker, zeroes its
// velocity and angular velocity, applies a randomized toss impulse, notifies the network layer
// of the reposition, and deletes the unit's own object if the reposition failed. Finally, if the
// unit is flagged to delete dropped items, deletes the dropped object (recursively, if its
// network role calls for that).
void unit_drop_object_from_hand(uint32_t unit_index, uint32_t dropped_object_index)
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    object *dropped = ((object_header *)object_data->data)[dropped_object_index & 0xffff].data;

    if (dropped->parent_object == k_datum_index_none) {
        Object *dropped_tag;

        object_set_cluster_and_parent(dropped_object_index, 0);
        dropped = ((object_header *)object_data->data)[dropped_object_index & 0xffff].data;
        dropped_tag = (Object *)tag_instances[dropped->definition_tag & 0xffff].data;

        if (*(int32_t *)&dropped_tag->model.tag_id != -1) {
            if ((dropped->flags & 1) != 0) {
                object_for_each_light_attachment(dropped_object_index, 0, 1);
            }
            if (*(int32_t *)&dropped_tag->model.tag_id != -1) {
                dropped->flags &= ~1u;
                ((object_header *)object_data->data)[dropped_object_index & 0xffff].flags |= 2;
            }
        }
        object_reorient_relative_to_marker(unit_index, "left hand", dropped_object_index, 0);
    }

    {
        // UNSURE: this clears the low two bits of the dropped item's own struct at the same
        // byte offset unit_data starts at (0x1f4); the dropped object is not a unit, so no
        // header field names it.
        uint32_t *item_field_1f4 = (uint32_t *)((uint8_t *)dropped + 0x1f4);
        *item_field_1f4 &= 0xfffffffc;
    }

    object_snap_to_parent_marker_and_detach(dropped_object_index);

    dropped = ((object_header *)object_data->data)[dropped_object_index & 0xffff].data;
    dropped->velocity = *(real_vector3d *)global_origin3d_pointer;
    dropped->angular_velocity = *(real_vector3d *)global_origin3d_pointer;

    {
        // UNSURE: see file header -- the randomized toss vector's construction is preserved
        // literally from the Ghidra locals rather than renamed.
        float local_24, local_20, local_1c, local_18, local_14, local_10;
        real_vector3d out;

        vector3d_randomize_direction((real_point3d *)&dropped->forward, &out, &random_seed_global, 0.0f, 0.39269908f);
        local_24 = out.i; local_20 = out.j; local_1c = out.k;
        local_18 = out.i; local_14 = out.j; local_10 = out.k; // UNSURE: see header

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        {
            float fVar6 = (float)(random_seed_global >> 0x10) * 1.5259022e-05f * 0.0133333355f + 0.026666667f;
            local_20 = fVar6 * local_20;
            local_1c = local_1c * fVar6;
        }

        local_24 = object_get_root_object_velocities() + local_18;
        *(uint32_t *)((uint8_t *)dropped + 0x200) = unit_index; // UNSURE: item-specific field, owner reference
        local_20 = local_14 + local_20;
        local_1c = local_10 + local_1c;

        {
            real_vector3d impulse = { local_24, local_20, local_1c };
            item_accelerate(&impulse, 0);
        }
    }

    {
        real_point3d discard;
        unit_get_camera_position(unit_index, &discard); // UNSURE: result unused by the original
    }

    if (object_reposition_to_spawn_location(dropped_object_index, (real_point3d *)0xffffffff) == 0 && current_game_engine == 0) {
        object_delete(dropped_object_index);
    }

    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    if ((unit->flags & _unit_flag_delete_when_dropped) != 0) {
        dropped = ((object_header *)object_data->data)[dropped_object_index & 0xffff].data;
        if (dropped->network_role == 0) {
            object_delete_unparented(dropped_object_index);
        } else if (dropped->network_role != 3) {
            return;
        }
        object_delete_recursive(dropped_object_index, 0);
    }
}

#if 0
Original Ghidra decompilation (0x56ed00):

void unit_drop_object_from_hand(uint param_1,uint param_2)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  int iVar5;
  float fVar6;
  undefined *puVar7;
  char cVar8;
  int iVar9;
  float10 fVar10;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar9 = (param_2 & 0xffff) * 0xc;
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9);
  if (*(int *)(iVar3 + 0x11c) == -1) {
    object_set_cluster_and_parent(param_2,0);
    puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9);
    iVar5 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (*(int *)(iVar5 + 0x34) != -1) {
      if ((puVar4[4] & 1) != 0) {
        object_for_each_light_attachment(0,1);
      }
      if (*(int *)(iVar5 + 0x34) != -1) {
        iVar5 = *(int *)(DAT_008603b0 + 0x34);
        puVar4[4] = puVar4[4] & 0xfffffffe;
        pbVar1 = (byte *)(iVar5 + iVar9 + 2);
        *pbVar1 = *pbVar1 | 2;
      }
    }
    object_reorient_relative_to_marker(param_1,"left hand");
  }
  iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9);
  *(uint *)(iVar5 + 500) = *(uint *)(iVar5 + 500) & 0xfffffffc;
  FUN_004f6610(param_2);
  puVar7 = PTR_DAT_00696714;
  *(undefined4 *)(iVar3 + 0x68) = *(undefined4 *)PTR_DAT_00696714;
  *(undefined4 *)(iVar3 + 0x6c) = *(undefined4 *)(puVar7 + 4);
  *(undefined4 *)(iVar3 + 0x70) = *(undefined4 *)(puVar7 + 8);
  *(undefined4 *)(iVar3 + 0x8c) = *(undefined4 *)puVar7;
  *(undefined4 *)(iVar3 + 0x90) = *(undefined4 *)(puVar7 + 4);
  *(undefined4 *)(iVar3 + 0x94) = *(undefined4 *)(puVar7 + 8);
  vector3d_randomize_direction(0,0x3ec90fdb);
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  fVar6 = (float)(random_seed_global >> 0x10) * 1.5259022e-05 * 0.0133333355 + 0.026666667;
  local_20 = fVar6 * local_20;
  local_1c = local_1c * fVar6;
  fVar10 = (float10)FUN_004f6aa0();
  local_24 = (float)(fVar10 + (float10)local_18);
  *(uint *)(iVar3 + 0x200) = param_1;
  local_20 = local_14 + local_20;
  local_1c = local_10 + local_1c;
  item_accelerate(&local_24,0);
  unit_get_camera_position();
  cVar8 = FUN_004f7b70(param_2,0xffffffff);
  if ((cVar8 == '\0') && (DAT_006f1d20 == 0)) {
    object_delete();
  }
  if ((*(uint *)(iVar2 + 0x204) & 0x100000) != 0) {
    iVar2 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9) + 4);
    if (iVar2 == 0) {
      object_delete_unparented();
    }
    else if (iVar2 != 3) {
      return;
    }
    object_delete_recursive(param_2,0);
  }
  return;
}
#endif
