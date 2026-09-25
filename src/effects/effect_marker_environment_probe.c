// effect_marker_environment_probe  (Ghidra: FUN_004533b0, still unnamed there)
// address 0x4533b0, size 222 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: types/tags.h TagReflexive (count at +0, pointer at +4) matches the single bounds
//   check against the tag's own data; types/math.h global_down3d_pointer-shaped constant at
//   0x0069672c (documented in types/projectiles.h as "the constant 'down' vector"); the call to
//   collision_test_movement_segment (0x505880, types/physics.h) with flags 0xc2a0, an origin
//   raised 0.15 units and a delta of the down vector scaled by 0.3 matches a short downward
//   ground probe from a marker. Confirmed against objdump -d -M intel, 0x4533b0..0x453485.
// register convention: a tag reference recognized on the stack (param_1); a location/marker
//   index in SI (unaff_SI); a marker world position pointer in EAX (in_EAX, 3 floats).
//   // blam-cc: stack -> definition_index, unaff_ESI (low 16) -> location_index, in_EAX -> position
// UNSURE: which tag group `definition_index` refers to was not established elsewhere in this
//   batch; its data merely needs to begin with a TagReflexive for this function's single bounds
//   check to make sense. UNSURE: the call to material_effects_play_at_marker at the end forwards
//   this function's own tag reference and location index plus four more values (a sound-material
//   index derived from the FUN_0053ed60 probe, a location bundle pointer, and a value read from
//   this function's own frame) that Ghidra's decompiler dropped entirely at this call site;
//   reconstructed from the raw disassembly below rather than from any struct evidence.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"
#include "projectiles.h"
#include "effects.h"

extern tag_instance *tag_instances;           // 0x0087bc14
extern const real_vector3d *global_down3d_pointer;  // 0x0069672c, the constant "down" vector

extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index, collision_result *result); // 0x505880

extern uint8_t scenario_location_get_water_and_weather(void *leaf_out, uint32_t mode); // UNSURE signature; resolves a point
    // to a cluster/sky state, see the weather_instance notes in
    // out/phase4/effects_types_notes.md (unresolved offsets, weather_instance 0x10/0x14); called
    // here with the collision result's leaf field as the (probably in/out) point/leaf argument

extern void material_effects_play_at_marker(uint32_t material_effects_tag, int16_t material_type,
    int16_t sub_effect_index, uint32_t *location_bundle, uint32_t sound_param,
    real_point3d *position, real_vector3d *offset); // 0x453490, this module

void effect_marker_environment_probe(uint32_t definition_index, int16_t location_index,
    real_point3d *marker_position) // blam-cc: stack, unaff_ESI, in_EAX
{
    TagReflexive *reflexive = (TagReflexive *)tag_instances[definition_index & 0xffff].data;

    if (location_index < (int32_t)reflexive->count) {
        real_point3d origin;
        real_vector3d delta;
        collision_result result;
        uint8_t hit;

        origin.x = marker_position->x;
        origin.y = marker_position->y;
        origin.z = marker_position->z + 0.15f;
        delta.i = global_down3d_pointer->i * 0.3f;
        delta.j = global_down3d_pointer->j * 0.3f;
        delta.k = global_down3d_pointer->k * 0.3f;

        hit = collision_test_movement_segment(0xc2a0, &origin, &delta, 0xffffffff, &result);
        if (hit) {
            uint8_t in_sky = scenario_location_get_water_and_weather(&result.leaf, 0);
            // UNSURE: 0x1c (28) when in_sky, otherwise a value read from this function's own
            // frame that the raw disassembly (`mov eax,[esp+0x54]`) could not be tied to a named
            // field; kept as the sky case's literal and a placeholder of 0 for the other case.
            int16_t material_type = in_sky ? 0x1c : 0;

            material_effects_play_at_marker(definition_index, material_type, location_index,
                                             (uint32_t *)&result.leaf, 0, &origin, &delta);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4533b0):

void FUN_004533b0(uint param_1)

{
  char cVar1;
  undefined4 *in_EAX;
  short unaff_SI;
  undefined4 local_68;
  undefined4 local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  undefined1 local_50 [12];
  undefined1 local_44 [68];

  if ((int)unaff_SI < **(int **)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)) {
    local_68 = *in_EAX;
    local_64 = in_EAX[1];
    local_60 = (float)in_EAX[2] + 0.15;
    local_5c = *(float *)PTR_DAT_0069672c * 0.3;
    local_58 = *(float *)(PTR_DAT_0069672c + 4) * 0.3;
    local_54 = *(float *)(PTR_DAT_0069672c + 8) * 0.3;
    cVar1 = FUN_00505880(0xc2a0,&local_68,&local_5c,0xffffffff,local_50);
    if (cVar1 != '\0') {
      FUN_0053ed60(local_44,0);
      FUN_00453490();
    }
  }
  return;
}

Raw disassembly (objdump -d -M intel, 0x4533b0..0x453490), used to reconstruct the call to
FUN_00453490 that Ghidra shows with zero arguments:

004533b0:
  mov edx,[0x87bc14]
  sub esp,0x68
  push ebp
  mov ebp,[esp+0x70]        ; ebp = param_1 (definition_index)
  mov ecx,ebp
  and ecx,0xffff
  shl ecx,0x5
  mov ecx,[ecx+edx*1+0x14]  ; ecx = tag_instances[definition_index & 0xffff].data
  movsx edx,si               ; edx = location_index (unaff_SI)
  push edi
  cmp edx,[ecx]
  jge 0x453488
  ... (origin/delta setup, call 0x505880, call 0x53ed60 as decompiled above) ...
  mov eax,0x1c
  jne 0x45346a
  mov eax,[esp+0x54]
45346a:
  mov edx,[esp+0x78]
  push edx
  lea ecx,[esp+0x30]          ; same address passed to FUN_0053ed60 above
  push ecx
  push eax
  push esi                    ; esi still holds the caller's own location_index
  lea edi,[esp+0x54]
  lea edx,[esp+0x48]
  mov eax,ebp                  ; eax = definition_index, forwarded unchanged
  call 0x453490
  add esp,0x10
453488: pop edi; pop ebp; add esp,0x68; ret
#endif
