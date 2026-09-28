// effect_new_with_color  (Ghidra: FUN_00450980, still unnamed there; named from its own summary in
// out/phase4/effects_functions.md: "Creates a new particle system with an explicit (or default)
// tint color and resolved lightmap index, binding it to the object's markers")
// address 0x450980, size 282 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump) (LOW -- see UNSURE notes)
// evidence: types/effects.h effect (object_index 0x3c set to k_datum_index_none -- this is the
// one wrapper that creates a free-standing, unattached effect; location 0x10; color 0x24 --
// wait: this function writes offset 0x24, which is `velocity`, not `color` (0x18); see UNSURE);
// types/math.h global_origin3d_pointer (0x00696714); src/objects/antenna_apply_marker_delta.c
// establishes FUN_005013a0's (globals, point, index) signature and the leaf/cluster lookup
// idiom.
// register convention: definition_index and creator_object_index are Ghidra's own recognised
// stack parameters (param_1, param_2, both forwarded to effect_new's force_create as param_12);
// the position pointer, marker context fields and A/B scale are further stack parameters.
// UNSURE: `*(undefined4 *)(iVar3 + 0x24) = *param_3` writes into effect+0x24, which
// types/effects.h documents as `velocity` (a real_vector3d), not a colour -- despite this
// function's own summary calling it a tint colour. Preserved at the literal offset with the
// header's own field name (`velocity`) rather than silently reinterpreting it as colour.
// UNSURE (structural, TYPES-GAP): as with FUN_00450870, the marker resolver `&LAB_00451850` is a
// thunk Ghidra did not split into its own function, and the scratch context this function builds
// (local_1c/local_18/local_14/local_10/local_c/local_8) is reconstructed by layout only.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *effect_data;         // 0x0087abdc
extern ModelCollisionGeometryBSP *global_collision_bsp;            // 0x00746f90, passed to FUN_005013a0 in ECX
extern ScenarioStructureBSP *global_structure_bsp;
extern const real_point3d *global_origin3d_pointer; // 0x00696714 -> 0x0065c230, math module
extern void *effect_marker_callback_context; // 0x006b0adc

// effect_marker_node_context now lives in types/effects.h. The earlier local copy here had one
// field fewer than the one in effect_new_on_object_with_node_table.c, so the last forwarded
// dword (Ghidra's local_8, param_7) was dropped; it is written below.

extern datum_index effect_new(datum_index definition_index, datum_index creator_object_index,
    uint8_t force_create); // 0x451500, this module
extern void effect_set_placement(effect *self, const ColorRGB *color,
    const effect_tint_source *tint_source, real a_scale, real b_scale); // 0x451600, this module
extern void effect_rebuild_markers(effect *self,
    int32_t (*resolve_marker)(uint32_t, const char *, object_marker *, uint32_t)); // 0x451710, this module
extern int32_t effect_marker_node_table_resolver(uint32_t object_index, const char *location,
    object_marker *out, uint32_t max_count); // 0x451850, UNSURE, see effect_new_on_object_with_node_table.c
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point
    // 0x5013a0; blam-cc: ECX -> globals, EDX -> point, EAX -> index
extern void effect_update(datum_index effect_handle, real delta_time); // 0x451a30, this module

// Creates a free-standing effect (not attached to any object) at `position`, probing its BSP
// location and defaulting its velocity to the global origin vector when `velocity` is NULL.
// FIXED (objdump 0x450980..0x450a99): TWELVE stack arguments (every caller cleans 0x30). Arg 6 is the position:
//   it goes to bsp3d_node_find_leaf in EDX (0x4509e4 -> 0x450a18) and is also stored as marker-context +0x10;
//   args 8..11 are effect_set_placement's a/b scale (stack) and color (ECX) / tint_source (EDX) (0x4509b2..0x4509d6);
//   arg 12 is effect_new's force_create (0x450980).
datum_index effect_new_with_color(datum_index definition_index, datum_index creator_object_index,
    const real_vector3d *velocity, uint16_t ctx_08, uint32_t ctx_0c, real_point3d *position,
    uint32_t ctx_14, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source,
    uint8_t force_create)
{
    datum_index handle = effect_new(definition_index, creator_object_index, force_create);

    if (handle != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)handle];
        effect_marker_node_context context;
        int32_t leaf;
        int i;

        effect_set_placement(self, color, tint_source, a_scale, b_scale);
        self->object_index = k_datum_index_none;

        context.unknown_08 = ctx_08;
        context.unknown_0c = ctx_0c;
        context.unknown_10 = (uint32_t)position;
        context.unknown_14 = ctx_14;
        context.node_index = 0xffff; // this wrapper has no object, so no node table
        context.node_table_entry = 0;

        leaf = bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)global_collision_bsp, position);
        self->location.leaf_index = leaf;
        self->location.cluster_index = (leaf == -1) ? -1 :
            *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);

        if (velocity == 0) {
            // types/math.h declares this as const real_point3d *; same three floats.
            velocity = (const real_vector3d *)global_origin3d_pointer;
        }
        self->velocity = *velocity; // UNSURE: see file header -- this offset is `velocity`, not
            // colour, despite this function's own summary

        effect_marker_callback_context = &context;

        for (i = 0; i < 32; i++) {
            self->location_markers[i] = k_datum_index_none;
        }

        effect_rebuild_markers(self, effect_marker_node_table_resolver);

        effect_update(handle, 0.0f);
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x450980):

uint FUN_00450980(uint param_1,uint param_2,undefined4 *param_3,undefined2 param_4,
                 undefined4 param_5,undefined4 param_6,undefined4 param_7,undefined4 param_8,
                 undefined4 param_9,undefined4 param_10,undefined4 param_11,char param_12)

{
  undefined2 uVar1;
  uint particle_system_index;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined2 local_1c [2];
  undefined4 local_18;
  undefined2 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;

  particle_system_index = particle_system_new(param_1,param_2,param_12);
  if (particle_system_index != 0xffffffff) {
    iVar3 = (particle_system_index & 0xffff) * 0xfc + *(int *)(DAT_0087abdc + 0x34);
    FUN_00451600(param_8,param_9);
    *(undefined4 *)(iVar3 + 0x3c) = 0xffffffff;
    local_14 = param_4;
    local_10 = param_5;
    local_8 = param_7;
    local_c = param_6;
    local_1c[0] = 0xffff;
    local_18 = 0;
    iVar2 = FUN_005013a0();
    *(int *)(iVar3 + 0x10) = iVar2;
    if (iVar2 == -1) {
      uVar1 = 0xffff;
    }
    else {
      uVar1 = *(undefined2 *)(iVar2 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
    }
    *(undefined2 *)(iVar3 + 0x14) = uVar1;
    if (param_3 == (undefined4 *)0x0) {
      param_3 = (undefined4 *)PTR_DAT_00696714;
    }
    *(undefined4 *)(iVar3 + 0x24) = *param_3;
    *(undefined4 *)(iVar3 + 0x28) = param_3[1];
    *(undefined4 *)(iVar3 + 0x2c) = param_3[2];
    DAT_006b0adc = local_1c;
    puVar4 = (undefined4 *)(iVar3 + 0x5c);
    for (iVar2 = 0x20; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = 0xffffffff;
      puVar4 = puVar4 + 1;
    }
    FUN_00451710(iVar3,&LAB_00451850);
    particle_system_update(particle_system_index,0.0);
  }
  return particle_system_index;
}
#endif
