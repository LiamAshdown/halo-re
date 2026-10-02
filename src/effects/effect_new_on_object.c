// effect_new_on_object  (Ghidra: FUN_004507a0; named from its own summary in
// out/phase4/effects_functions.md: "Creates a new particle system on an object with an explicit
// min/max scale range, binding it to the object's markers")
// address 0x4507a0, size 201 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: types/effects.h effect (object_index 0x3c, first_person_weapon_index 0x4c, flags
// _effect_first_person_bit 0x40, location_markers[32]); types/effects.h globals list
// first_person_effects_enabled (0x00687014).
// register convention: creator object index in EAX (in_EAX), effect definition index in ECX
// (in_ECX); object_index and an optional first_person_weapon_index override are Ghidra's own
// recognised stack parameters (param_1, param_2), alongside the a_scale/b_scale pair forwarded
// to effect_set_placement (param_3, param_4).
//   // blam-cc: EAX -> creator_object_index, ECX -> definition_index,
//   //   stack -> (object_index, first_person_weapon_override, a_scale, b_scale, color, tint_source)
// FIXED (objdump 0x4507a0..0x450868): SIX stack arguments (every caller cleans 0x18); args 5/6 are
//   effect_set_placement's ECX color / EDX tint_source; the two helpers take object_index in ESI / ECX.
// UNSURE: whether `object_index` (param_1, stored into effect+0x3c) is really always identical
// to the `creator_object_index` handed to effect_new (in_EAX) could not be confirmed from this
// decompile alone; both are preserved as distinct parameters per types/effects.h's own
// creator_object_index UNSURE note.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *effect_data;                 // 0x0087abdc
extern uint8_t first_person_effects_enabled;    // 0x00687014

extern datum_index effect_new(datum_index definition_index, datum_index creator_object_index,
    uint8_t force_create); // 0x451500, this module
extern void effect_set_placement(effect *self, const ColorRGB *color,
    const effect_tint_source *tint_source, real a_scale, real b_scale); // 0x451600, this module
extern int32_t local_player_index_for_object(datum_index object_index); // 0x4926f0, ESI object_index
extern uint8_t effect_first_person_screen_timer_active(datum_index object_index); // 0x450680, ECX object_index
extern void effect_rebuild_markers(effect *self,
    int32_t (*resolve_marker)(uint32_t, const char *, object_marker *, uint32_t)); // 0x451710, this module
extern int32_t object_get_node_local_transform(uint32_t object_index, const char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080, established
extern int32_t first_person_weapon_get_marker_data(uint32_t object_index, const char *location,
    object_marker *out, uint32_t max_count); // outside this batch's range
extern void effect_update(datum_index effect_handle, real delta_time); // 0x451a30, this module

// Creates an effect on `object_index` with an explicit A/B scale range, marking it first-person
// when first_person_effects_enabled and the local player's screen timer is active, and binding
// it to every matching object (and, if applicable, first-person weapon) marker.
datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source)
{
    datum_index handle = effect_new(definition_index, creator_object_index, 1);

    if (handle != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)handle];
        int i;

        effect_set_placement(self, color, tint_source, a_scale, b_scale); // 0x4507c6..0x4507eb: ECX arg5, EDX arg6
        self->object_index = object_index;
        self->first_person_weapon_index = (int16_t)local_player_index_for_object(object_index); // ESI = object_index

        if (first_person_effects_enabled != 0 && effect_first_person_screen_timer_active(object_index)) {
            self->flags = self->flags | _effect_first_person_bit;
        }

        for (i = 0; i < 32; i++) {
            self->location_markers[i] = k_datum_index_none;
        }

        effect_rebuild_markers(self, object_get_node_local_transform);
        if (self->first_person_weapon_index != -1) {
            effect_rebuild_markers(self, first_person_weapon_get_marker_data);
        }

        if (first_person_weapon_override != -1) {
            self->first_person_weapon_index = first_person_weapon_override;
        }

        effect_update(handle, 0.0f);
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x4507a0):

uint FUN_004507a0(undefined4 param_1,short param_2,undefined4 param_3,undefined4 param_4)

{
  char cVar1;
  undefined2 uVar2;
  uint in_EAX;
  uint particle_system_index;
  uint in_ECX;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;

  particle_system_index = particle_system_new(in_ECX,in_EAX,'\x01');
  if (particle_system_index != 0xffffffff) {
    iVar4 = (particle_system_index & 0xffff) * 0xfc + *(int *)(DAT_0087abdc + 0x34);
    FUN_00451600(param_3,param_4);
    *(undefined4 *)(iVar4 + 0x3c) = param_1;
    uVar2 = FUN_004926f0();
    *(undefined2 *)(iVar4 + 0x4c) = uVar2;
    if (DAT_00687014 != '\0') {
      cVar1 = FUN_00450680();
      if (cVar1 != '\0') {
        *(byte *)(iVar4 + 2) = *(byte *)(iVar4 + 2) | 0x40;
      }
    }
    puVar5 = (undefined4 *)(iVar4 + 0x5c);
    for (iVar3 = 0x20; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = 0xffffffff;
      puVar5 = puVar5 + 1;
    }
    FUN_00451710(iVar4,object_get_node_local_transform);
    if (*(short *)(iVar4 + 0x4c) != -1) {
      FUN_00451710(iVar4,first_person_weapon_get_marker_data);
    }
    if (param_2 != -1) {
      *(short *)(iVar4 + 0x4c) = param_2;
    }
    particle_system_update(particle_system_index,0.0);
  }
  return particle_system_index;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
