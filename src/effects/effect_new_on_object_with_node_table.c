// effect_new_on_object_with_node_table  (Ghidra: FUN_00450870, still unnamed there; named from its
// own summary in out/phase4/effects_functions.md: "Creates a new particle system on an object
// with an explicit orientation/placement and an associated node-table entry")
// address 0x450870, size 265 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump; first-person check argument FIXED) (LOW -- see UNSURE notes)
// evidence: types/effects.h effect (object_index 0x3c, flags _effect_first_person_bit,
// location_markers[32]) and effect_marker_callback_context (0x006b0adc, "the scratch block the
// marker resolution thunk at 0x00451850 reads").
// register convention: creator object index in EAX (in_EAX), effect definition index in ECX
// (in_ECX), attach object index in EDX (in_EDX); the node index and five placement dwords
// forwarded to effect_set_placement/the marker context are Ghidra's own recognised stack
// parameters.
//   // blam-cc: EAX -> creator_object_index, ECX -> definition_index, EDX -> object_index,
//   //   stack -> (node_index, ctx_08, ctx_0c, ctx_10, ctx_14, a_scale, b_scale, color, tint_source)
// FIXED (objdump 0x450870..0x450978): nine stack arguments; the last two are effect_set_placement's ECX/EDX.
// OPEN: damage_effect_new_at_location, object_damage_effect_dispatch and projectile_response declare their own
//   externs of this without the EAX/ECX/EDX arguments.
// UNSURE (structural, TYPES-GAP): FUN_00451710 is called here with a resolver
// (&LAB_00451850) that is a jump target/thunk, not a function Ghidra split out on its own; its
// real signature and behaviour (it reads effect_marker_callback_context, per types/effects.h's
// own note) are not recoverable from this pack. The local scratch context this function builds
// (local_18/local_14/local_10/local_c/local_8, five caller-owned dwords keyed by a node index
// derived from object+0x1f2) is reconstructed only well enough to preserve its layout, not its
// meaning.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *effect_data;              // 0x0087abdc
extern data_array *object_data;              // 0x008603b0
extern uint8_t first_person_effects_enabled; // 0x00687014
extern void *effect_marker_callback_context; // 0x006b0adc

// effect_marker_node_context now lives in types/effects.h: the phase-4 integration pass folded
// it there out of this file and effect_new_with_color.c, which build the same 0x18 byte block.

extern datum_index effect_new(datum_index definition_index, datum_index creator_object_index,
    uint8_t force_create); // 0x451500, this module
extern void effect_set_placement(effect *self, const ColorRGB *color,
    const effect_tint_source *tint_source, real a_scale, real b_scale); // 0x451600, this module
extern uint8_t effect_first_person_screen_timer_active(datum_index object_index); // 0x450680, ECX
extern void effect_rebuild_markers(effect *self,
    int32_t (*resolve_marker)(uint32_t, const char *, object_marker *, uint32_t)); // 0x451710, this module
extern int32_t effect_marker_node_table_resolver(uint32_t object_index, const char *location,
    object_marker *out, uint32_t max_count); // 0x451850, a thunk/label Ghidra did not split out;
    // UNSURE signature, see file header
extern void effect_update(datum_index effect_handle, real delta_time); // 0x451a30, this module

// Creates an effect on `object_index` bound to a specific per-object-type node table entry,
// with an explicit A/B scale range.
// Stack parameter order is Ghidra's own: node_index, then the four opaque marker context
// dwords, then the two scales. creator_object_index / definition_index / object_index arrive
// in EAX / ECX / EDX.
datum_index effect_new_on_object_with_node_table(datum_index creator_object_index,
    datum_index definition_index, datum_index object_index, uint16_t node_index,
    uint16_t ctx_08, uint32_t ctx_0c, uint32_t ctx_10, uint32_t ctx_14, real a_scale,
    real b_scale, const ColorRGB *color, const effect_tint_source *tint_source)
{
    datum_index handle = effect_new(definition_index, creator_object_index, 1);

    if (handle != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)handle];
        effect_marker_node_context context;
        object *attach_object;
        int i;

        effect_set_placement(self, color, tint_source, a_scale, b_scale); // 0x4508bb: ECX = stack arg 7,
            // EDX = stack arg 8 (FIXED: the draft had no such parameters and passed 0, 0)
        self->object_index = object_index;

        if (first_person_effects_enabled != 0 && effect_first_person_screen_timer_active(object_index)) { // FIXED: ECX = EDI (0x4508d5)
            self->flags = self->flags | _effect_first_person_bit;
        }

        context.unknown_08 = ctx_08;
        context.unknown_0c = ctx_0c;
        context.unknown_14 = ctx_14;
        context.unknown_10 = ctx_10;
        // The original is ((node_index == 0xffff) - 1) & node_index, so 0xffff -- the "no node
        // table" marker -- collapses to index 0 and every other value passes through. An
        // earlier draft of this file kept 0xffff, which inverted the mask.
        context.node_index = (node_index == 0xffff) ? 0 : node_index;

        attach_object = ((object_header *)object_data->data)[(uint16_t)object_index].data;
        context.node_table_entry = (int16_t)context.node_index * 0x34 +
            ((struct object *)attach_object)->nodes.offset + (int32_t)(long)attach_object;

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
Original Ghidra decompilation (0x450870):

uint FUN_00450870(ushort param_1,undefined2 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6,undefined4 param_7)

{
  char cVar1;
  uint in_EAX;
  uint particle_system_index;
  uint in_ECX;
  int iVar2;
  uint in_EDX;
  int iVar3;
  undefined4 *puVar4;
  ushort local_18 [2];
  int local_14;
  undefined2 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  particle_system_index = particle_system_new(in_ECX,in_EAX,'\x01');
  if (particle_system_index != 0xffffffff) {
    iVar3 = (particle_system_index & 0xffff) * 0xfc + *(int *)(DAT_0087abdc + 0x34);
    FUN_00451600(param_6,param_7);
    *(uint *)(iVar3 + 0x3c) = in_EDX;
    if (DAT_00687014 != '\0') {
      cVar1 = FUN_00450680();
      if (cVar1 != '\0') {
        *(byte *)(iVar3 + 2) = *(byte *)(iVar3 + 2) | 0x40;
      }
    }
    local_8 = param_4;
    local_10 = param_2;
    local_4 = param_5;
    local_c = param_3;
    local_18[0] = (param_1 == 0xffff) - 1 & param_1;
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc);
    local_14 = (short)local_18[0] * 0x34 + *(short *)(iVar2 + 0x1f2) + iVar2;
    DAT_006b0adc = local_18;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
