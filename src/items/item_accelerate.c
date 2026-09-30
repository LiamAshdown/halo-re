// item_accelerate  (Ghidra: item_accelerate, already named via cea-pdb hint on the shared
// "ground point" string)
// address 0x4bd080, size 961 bytes
// name confidence: 0.7   rewrite confidence: 0.4
// evidence: types/items.h item_flags (_item_does_not_accelerate_bit 0x20,
//   _item_at_rest_on_structure_bit 0x08), item_data (resting_surface_index 0x1fa,
//   ignore_object_index 0x200); types/objects.h object (parent_object 0x11c, velocity 0x068,
//   angular_velocity 0x08c, flags 0x010 with _object_at_rest_bit); types/tags.h Item.item_flags
//   (bit 0x02 = destroyed_by_explosions, per its own bitfield comment); types/math.h
//   real_plane3d (normal + d, matching src/units/unit_find_nearest_valid_surface_plane.c's own
//   note: "+0x10 plane array, stride 0x10 = normal.xyz + d"); global 0x00696720
//   global_up3d_pointer and its cross/normalize/random-jitter idiom, copied verbatim from
//   src/units/unit_apply_impulse.c (same author's own UNSURE caveat on the cross product's
//   ecx_operand applies here too); callees item_detonation_timer_start (0x4bd450),
//   item_compute_rotation (0x4bd500), object_get_node_local_transform,
//   object_set_position_and_relink, random_real_range, random_get_table_point.
// register convention: item index in EAX (in_EAX); the impulse vector and the
//   "start detonation timer" flag are Ghidra's own two recognized stack parameters
//   (`item_accelerate(float *param_1, char param_2)`).
//   // blam-cc: EAX -> item_index, stack -> delta, apply_detonation_timer
// resolved from disassembly (objdump -d -M intel bin/halo.exe, 0x4bd080..0x4bd440), because
// Ghidra's decompile elides every argument to structure_bsp_plane_fetch_signed, object_get_node_local_transform,
// object_set_position_and_relink and the two vector3d_* calls used to snap a woken item back
// onto its resting surface. Tracing ESP through that block (the "ground point" marker is
// refetched, its node_transform.position dotted against a plane structure_bsp_plane_fetch_signed fills in from
// item_data.resting_surface_index, then nudged 0.05 units off the plane along its normal) gives
// a coherent penetration-correction pass; see the UNSURE notes below for the pieces that stay
// approximate (structure_bsp_plane_fetch_signed's exact signature, and the exact register/stack split at the
// object_set_position_and_relink call site -- this codebase already flags that same function's
// calling convention as uncertain at several other call sites).
// reconciled: R04 0x006f1d20 uint8_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern uint8_t *global_structure_collision_bsp; // 0x00746f98 (a pointer: 0x4bd155 loads it, then +0x40 surfaces), UNSURE shape (collision/BSP module);
    // +0x40 is the per-surface table (stride 0x0c, first dword is a plane index into +0x10's
    // plane array, stride 0x10) per src/units/unit_find_nearest_valid_surface_plane.c
extern random_seed random_seed_global; // 0x00719cd0
extern real_vector3d *global_up3d_pointer; // 0x00696720, indirect pointer to math.h global_up3d

extern void item_detonation_timer_start(uint32_t object_index); // 0x4bd450, this batch, EAX -> object_index
extern void item_compute_rotation(uint32_t object_index); // 0x4bd500, this batch, EAX -> object_index
extern void object_list_membership_set(uint32_t object_index, char add); // 0x4f7450, objects module
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index,
    bsp_leaf_reference *location); // 0x4f5350, ESI, EDI, stack (location may be 0)
extern void structure_bsp_plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index); // 0x44dad0, src/structures;
    // blam-cc: EAX out, EDX signed_index, stack planes_owner
    // UNSURE signature (EAX -> out, EDX -> plane_index, stack -> bsp_globals, per disassembly);
    // almost certainly a plane-table lookup, see file header
extern real random_real_range(real min, real max); // 0x401050, math module
extern void random_get_table_point(real_vector3d *out); // 0x473560, out in EAX
extern double sqrt(double x); // a single x87 FSQRT instruction
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0

// Applies a translational impulse to an item, adding it into velocity, waking it from a resting
// surface (snapping it back to a legal clearance above that surface first) when the impulse is
// non-trivial, and inducing a matching angular jitter or wobble.
void item_accelerate(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer)
    // blam-cc: EAX -> item_index, stack -> delta, apply_detonation_timer
{
    object *obj = ((object_header *)object_data->data)[item_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if ((item->flags & _item_does_not_accelerate_bit) != 0) {
        return;
    }
    if (obj->parent_object != (datum_index)0xffffffff) {
        return;
    }

    if (apply_detonation_timer != 0 && current_game_engine == 0) {
        Item *tag = (Item *)tag_instances[obj->definition_tag & 0xffff].data;
        if ((tag->item_flags & 0x02) != 0) { // destroyed_by_explosions, see types/tags.h ItemFlags
            item_detonation_timer_start(item_index);
        }
    }

    if ((item->flags & _item_at_rest_on_structure_bit) == 0) {
        obj->flags &= ~(uint32_t)_object_at_rest_bit;
    } else if (0.0001f <= delta->i * delta->i + delta->j * delta->j + delta->k * delta->k) {
        object_marker marker;
        if (object_get_node_local_transform(item_index, "ground point", &marker, 1) != 0) {
            // UNSURE: structure_bsp_plane_fetch_signed's exact signature; see file header
            real_plane3d plane;
            // 0x4bd155..0x4bd16f: surfaces pointer at bsp+0x40, 0xc stride, the index sign-extended
            int32_t surface_plane_ref = *(int32_t *)((uint8_t *)((ModelCollisionGeometryBSP *)global_structure_collision_bsp)->surfaces.pointer
                + (int32_t)(int16_t)item->resting_surface_index * 0x0c);
            real_point3d marker_position = marker.node_transform.position;
            real correction;
            real_point3d corrected_position;

            structure_bsp_plane_fetch_signed(&plane, global_structure_collision_bsp, surface_plane_ref);
            correction = 0.05f - ((plane.normal.i * marker_position.x +
                plane.normal.j * marker_position.y + plane.normal.k * marker_position.z) - plane.d);
            corrected_position.x = plane.normal.i * correction + marker_position.x;
            corrected_position.y = plane.normal.j * correction + marker_position.y;
            corrected_position.z = plane.normal.k * correction + marker_position.z;

            object_set_position_and_relink(&corrected_position, item_index, 0);
        }
        obj->flags &= ~(uint32_t)_object_at_rest_bit;
        item->flags &= ~(uint32_t)_item_at_rest_on_structure_bit;
    }

    obj->velocity.i += delta->i;
    obj->velocity.j += delta->j;
    obj->velocity.k += delta->k;

    if (item->ignore_object_index != (datum_index)0xffffffff ||
        (item->flags & _item_at_rest_on_structure_bit) == 0 ||
        0.0001f <= delta->i * delta->i + delta->j * delta->j + delta->k * delta->k) {
        real_vector3d cross_axis;
        real magnitude = (real)sqrt((double)(delta->i * delta->i + delta->j * delta->j + delta->k * delta->k));
        uint32_t seed_snapshot;
        real length;
        real angle;

        if (magnitude < 0.0001f) {
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            magnitude = (real)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f;
        }
        seed_snapshot = random_seed_global;

        // VERIFIED against disassembly 0x4bd373..0x4bd37f (2026-09-30): EAX=cross_axis, ECX=ebx=delta, stack=[0x696720] global up
        vector3d_cross_product(&cross_axis, delta, global_up3d_pointer);
        length = vector3d_normalize_with_length(&cross_axis);
        if (length <= 0.0f) {
            random_get_table_point(&cross_axis);
            seed_snapshot = random_seed_global;
        }

        random_seed_global = seed_snapshot * 0x19660d + 0x3c6ef35f;
        angle = (real)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f * magnitude * 1.5707964f;
        obj->angular_velocity.i += cross_axis.i * angle;
        obj->angular_velocity.j += cross_axis.j * angle;
        obj->angular_velocity.k += cross_axis.k * angle;
    } else {
        object_marker marker;
        real_vector3d axis;
        real angle;

        if (object_get_node_local_transform(item_index, "ground point", &marker, 1) != 0) {
            axis = marker.node_transform.up;
        } else {
            axis = *global_up3d_pointer;
        }
        angle = random_real_range(-1.5707964f, 1.5707964f);
        obj->angular_velocity.i += axis.i * angle;
        obj->angular_velocity.j += axis.j * angle;
        obj->angular_velocity.k += axis.k * angle;
    }

    item_compute_rotation(item_index);
    object_list_membership_set(item_index, 0);
}

#if 0
Original Ghidra decompilation (0x4bd080):

void item_accelerate(float *param_1,char param_2)

{
  uint *puVar1;
  float fVar2;
  short sVar3;
  uint in_EAX;
  uint uVar4;
  float10 fVar5;
  float fVar6;
  float local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (((puVar1[0x7d] & 0x20) == 0) && (puVar1[0x47] == 0xffffffff)) {
    if ((param_2 != '\0') &&
       ((DAT_006f1d20 == 0 &&
        ((*(byte *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c) & 2) != 0))))
    {
      FUN_004bd450();
    }
    if ((puVar1[0x7d] & 8) == 0) {
      puVar1[4] = puVar1[4] & 0xffffffdf;
    }
    else if (0.0001 <= param_1[2] * param_1[2] + param_1[1] * param_1[1] + *param_1 * *param_1) {
      sVar3 = object_get_node_local_transform();
      if (sVar3 != 0) {
        FUN_0044dad0(DAT_00746f98);
        fVar2 = 0.05 - ((local_80 * local_c + local_7c * local_8 + local_78 * local_4) - local_74);
        local_90 = local_80 * fVar2 + local_c;
        local_8c = local_7c * fVar2 + local_8;
        local_88 = local_78 * fVar2 + local_4;
        object_set_position_and_relink(0);
      }
      puVar1[4] = puVar1[4] & 0xffffffdf;
      puVar1[0x7d] = puVar1[0x7d] & 0xfffffff7;
    }
    puVar1[0x1a] = (uint)((float)puVar1[0x1a] + *param_1);
    puVar1[0x1b] = (uint)((float)puVar1[0x1b] + param_1[1]);
    puVar1[0x1c] = (uint)((float)puVar1[0x1c] + param_1[2]);
    if (((puVar1[0x80] != 0xffffffff) || ((puVar1[0x7d] & 8) == 0)) ||
       (0.0001 <= *param_1 * *param_1 + param_1[2] * param_1[2] + param_1[1] * param_1[1])) {
      local_84 = SQRT(*param_1 * *param_1 + param_1[2] * param_1[2] + param_1[1] * param_1[1]);
      if (local_84 < 0.0001) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        local_84 = (float)(random_seed_global >> 0x10) * 1.5259022e-05;
      }
      uVar4 = random_seed_global;
      vector3d_cross_product(PTR_DAT_00696720);
      fVar5 = (float10)vector3d_normalize_with_length();
      if (fVar5 <= (float10)0.0) {
        random_get_table_point();
        uVar4 = random_seed_global;
      }
      random_seed_global = uVar4 * 0x19660d + 0x3c6ef35f;
      fVar6 = (float)(random_seed_global >> 0x10) * 1.5259022e-05 * local_84 * 1.5707964;
      fVar2 = local_88 * fVar6;
      puVar1[0x23] = (uint)(local_90 * fVar6 + (float)puVar1[0x23]);
      puVar1[0x24] = (uint)(local_8c * fVar6 + (float)puVar1[0x24]);
    }
    else {
      sVar3 = object_get_node_local_transform();
      if (sVar3 == 0) {
        local_90 = *(float *)PTR_DAT_00696720;
        local_8c = *(float *)(PTR_DAT_00696720 + 4);
        local_88 = *(float *)(PTR_DAT_00696720 + 8);
      }
      else {
        local_90 = local_18;
        local_8c = local_14;
        local_88 = local_10;
      }
      fVar6 = random_real_range(-1.5707964,1.5707964);
      fVar2 = local_88 * fVar6;
      puVar1[0x23] = (uint)(local_90 * fVar6 + (float)puVar1[0x23]);
      puVar1[0x24] = (uint)(local_8c * fVar6 + (float)puVar1[0x24]);
    }
    puVar1[0x25] = (uint)(fVar2 + (float)puVar1[0x25]);
    item_compute_ground_alignment_rotation();
    FUN_004f7450(0);
  }
  return;
}
#endif
