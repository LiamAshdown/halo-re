// effect_new_at_texture_coordinate  (Ghidra: FUN_004506d0; named per out/phase4/effects_types_notes.md,
// which refers to this address directly: "unknown_08, unknown_0a 0x08, 0x0a written only by
// 0x4506d0")
// address 0x4506d0, size 199 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: types/effects.h effect (object_index 0x3c, first_person_weapon_index 0x4c,
// unknown_08/0x0a, change_color_index 0x0c, tint_source 0x30, color 0x18, flags
// _effect_looping_bit, location_markers[32]).
// register convention: definition_index in EAX (in_EAX), object_index in EDX (in_EDX),
// change_color_index in CX (in_CX); the two texture coordinate shorts are Ghidra's own
// recognised stack parameters (param_1, param_2).
//   // blam-cc: EAX -> definition_index, EDX -> object_index, CX -> change_color_index,
//   //   stack -> (u, v)
// UNSURE: local_player_index_for_object (resolves first_person_weapon_index) is outside this batch's address
// range and is called here with every argument elided.
// UNSURE: PTR_DAT_00686b04 is a pointer-to-pointer to a default 3-float colour, by analogy with
// types/math.h's global_origin3d_pointer/global_forward3d_pointer indirection pattern, but its
// own address is not documented anywhere in this batch.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *effect_data; // 0x0087abdc
extern const ColorRGB *default_effect_color_pointer; // 0x00686b04, UNSURE, see file header

extern datum_index effect_new(datum_index definition_index, datum_index creator_object_index,
    uint8_t force_create); // 0x451500, this module
extern int16_t local_player_index_for_object(void); // 0x4926f0, outside this batch; resolves first_person_weapon_index
extern void effect_rebuild_markers(effect *self,
    int32_t (*resolve_marker)(uint32_t, const char *, object_marker *, uint32_t)); // 0x451710, this module
extern int32_t object_get_node_local_transform(uint32_t object_index, const char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080, established (also used as a resolver here)
extern int32_t first_person_weapon_get_marker_data(uint32_t object_index, const char *location,
    object_marker *out, uint32_t max_count); // outside this batch's range
extern void effect_update(datum_index effect_handle, real delta_time); // 0x451a30, this module

// Creates an effect attached to `object_index` at an explicit 2D marker/texture coordinate,
// binding it to every object marker the effect's locations name.
datum_index effect_new_at_texture_coordinate(datum_index definition_index, datum_index object_index,
    int16_t change_color_index, int16_t u, int16_t v)
{
    datum_index handle = effect_new(definition_index, object_index, 1);

    if (handle != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)handle];
        int i;

        self->object_index = object_index;
        self->first_person_weapon_index = local_player_index_for_object();
        self->unknown_08 = u;
        self->unknown_0a = v;
        self->change_color_index = change_color_index;
        self->tint_source.proc = 0;
        self->tint_source.unknown_08 = 0;

        if (change_color_index == -1) {
            self->color = *default_effect_color_pointer; // one load: the global holds the pointer (mov eax,ds:0x686b04; mov ecx,[eax])
        }
        self->flags = self->flags | _effect_looping_bit;

        for (i = 0; i < 32; i++) {
            self->location_markers[i] = k_datum_index_none;
        }

        effect_rebuild_markers(self, object_get_node_local_transform);
        if (self->first_person_weapon_index != -1) {
            effect_rebuild_markers(self, first_person_weapon_get_marker_data);
        }

        effect_update(handle, 0.0f);
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x4506d0):

uint FUN_004506d0(undefined2 param_1,undefined2 param_2)

{
  undefined *puVar1;
  undefined2 uVar2;
  uint in_EAX;
  uint particle_system_index;
  short in_CX;
  int iVar3;
  uint in_EDX;
  int iVar4;
  undefined4 *puVar5;

  particle_system_index = particle_system_new(in_EAX,in_EDX,'\x01');
  if (particle_system_index != 0xffffffff) {
    iVar4 = (particle_system_index & 0xffff) * 0xfc + *(int *)(DAT_0087abdc + 0x34);
    *(uint *)(iVar4 + 0x3c) = in_EDX;
    uVar2 = FUN_004926f0();
    *(undefined2 *)(iVar4 + 0x4c) = uVar2;
    *(undefined2 *)(iVar4 + 8) = param_1;
    *(undefined2 *)(iVar4 + 10) = param_2;
    *(short *)(iVar4 + 0xc) = in_CX;
    *(undefined4 *)(iVar4 + 0x34) = 0;
    *(undefined4 *)(iVar4 + 0x38) = 0;
    puVar1 = PTR_DAT_00686b04;
    if (in_CX == -1) {
      *(undefined4 *)(iVar4 + 0x18) = *(undefined4 *)PTR_DAT_00686b04;
      *(undefined4 *)(iVar4 + 0x1c) = *(undefined4 *)(puVar1 + 4);
      *(undefined4 *)(iVar4 + 0x20) = *(undefined4 *)(puVar1 + 8);
    }
    *(byte *)(iVar4 + 2) = *(byte *)(iVar4 + 2) | 2;
    puVar5 = (undefined4 *)(iVar4 + 0x5c);
    for (iVar3 = 0x20; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = 0xffffffff;
      puVar5 = puVar5 + 1;
    }
    FUN_00451710(iVar4,object_get_node_local_transform);
    if (*(short *)(iVar4 + 0x4c) != -1) {
      FUN_00451710(iVar4,first_person_weapon_get_marker_data);
    }
    particle_system_update(particle_system_index,0.0);
  }
  return particle_system_index;
}
#endif
