// material_effects_play_at_marker  (Ghidra: FUN_00453490, still unnamed there; other modules
//   that reference this address opaquely, e.g. src/items/item_update.c, keep calling it
//   FUN_00453490 -- this file is the real definition)
// address 0x453490, size 287 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED against objdump 0x453490..0x4535ae)
// evidence: types/tags.h MaterialEffects { TagReflexive effects; } (0x8c) ->
//   MaterialEffectsMaterialEffect { TagReflexive materials; } (0x1c) ->
//   MaterialEffectsMaterialEffectMaterial { TagDependency effect; TagDependency sound; } (0x30):
//   the two nested `count`-then-`pointer` reads and the 0x1c/0x30 element strides match this
//   three-level chain exactly, and TagDependency.tag_id sits at +0xc, matching the two "!= -1"
//   checks at +0xc and +0x1c of the resolved material row. src/hs/hs_effect_spawn_at_location.c
//   and src/items/item_update.c already establish effect_new_with_color (spawn an effect at a position)
//   and sound_start_at_location (play a sound from a {position, normal, reference, leaf} bundle,
//   src/items/item_update.c's `sound_args`) as the two effects/sound dispatch points this
//   function's two branches lead to.
// register convention: material_effects tag reference in EAX (in_EAX); world position in EDX
//   (in_EDX, 3 floats); a local-space offset/normal vector in EDI (unaff_EDI, 3 floats, scaled by
//   0.01 and added to the position). material_type, sub_effect_index, a location bundle pointer
//   and a value forwarded to sound_start_at_location are Ghidra-recognized stack parameters.
//   // blam-cc: in_EAX -> material_effects_tag, stack -> material_type/sub_effect_index/
//   location_bundle/sound_param, in_EDX -> position, unaff_EDI -> offset
// UNSURE: `location_bundle` (param_3) is only known to be 8 bytes copied verbatim into the
//   sound_start_at_location bundle at +0x24; src/items/item_update.c's own sound_args has a zero pad dword
//   there followed by a leaf/cluster pair, so this is plausibly the same {pad, bsp_leaf_reference}
//   shape, but no header proves it, so it is kept as a raw two-dword copy. effect_new_with_color is called
//   here with only the 6 arguments Ghidra shows, out of the 12-argument signature
//   src/hs/hs_effect_spawn_at_location.c establishes elsewhere; the remaining six are not
//   reconstructed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"
#include "sound.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern const real_point3d *global_origin3d_pointer; // 0x00696714

extern datum_index effect_new_with_color(datum_index definition_index, datum_index creator_object_index,
    const real_vector3d *velocity, uint16_t ctx_08, uint32_t ctx_0c, real_point3d *position,
    uint32_t ctx_14, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source,
    uint8_t force_create); // 0x450980, 12 stack arguments
extern datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale);
    // 0x543d80, EDX definition_index, EAX placement, stack scale

void material_effects_play_at_marker(uint32_t material_effects_tag, int16_t material_type,
    int16_t sub_effect_index, uint32_t *location_bundle, uint32_t sound_param,
    real_point3d *position, real_vector3d *offset)
{
    MaterialEffects *definition = (MaterialEffects *)tag_instances[material_effects_tag & 0xffff].data;

    if (material_type < (int32_t)definition->effects.count) {
        MaterialEffectsMaterialEffect *material =
            &((MaterialEffectsMaterialEffect *)definition->effects.pointer)[material_type];

        if (sub_effect_index != -1 && sub_effect_index < (int32_t)material->materials.count) {
            MaterialEffectsMaterialEffectMaterial *entry =
                &((MaterialEffectsMaterialEffectMaterial *)
                      material->materials.pointer)[sub_effect_index];
            real_point3d spawn_position;

            spawn_position.x = offset->i * 0.01f + position->x;
            spawn_position.y = offset->j * 0.01f + position->y;
            spawn_position.z = offset->k * 0.01f + position->z;

            if (*(uint32_t *)&entry->effect.tag_id != 0xffffffffu) {
                // 0x453520..0x453538: (tag, -1, NULL velocity, 1, 0, &spawn_position, EDI = offset as ctx_14,
                // EBX = this function's 4th stack argument as the a scale, 0, NULL color, NULL tint, 0)
                effect_new_with_color(*(uint32_t *)&entry->effect.tag_id, 0xffffffff, (const real_vector3d *)0, 1, 0,
                    &spawn_position, (uint32_t)offset, *(real *)&sound_param, 0.0f, (const ColorRGB *)0,
                    (const effect_tint_source *)0, 0);
            }

            if (*(uint32_t *)&entry->sound.tag_id != 0xffffffffu) {
                struct {
                    real_point3d position;
                    real_vector3d normal;
                    real_point3d reference;
                    uint32_t bundle_word0; // == location_bundle[0], UNSURE, see file header
                    uint32_t bundle_word1; // == location_bundle[1], UNSURE, see file header
                } sound_args;

                sound_args.position = spawn_position;
                sound_args.normal = *offset;
                sound_args.reference = *global_origin3d_pointer;
                sound_args.bundle_word0 = location_bundle[0];
                sound_args.bundle_word1 = location_bundle[1];

                // 0x453540..0x4535a0: EDX = entry's sound tag, EAX = &sound_args (a sound_placement), push the scale
                sound_start_at_location(*(datum_index *)&entry->sound.tag_id, (sound_placement *)&sound_args,
                    *(float *)&sound_param);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x453490):

void FUN_00453490(short param_1,short param_2,undefined4 *param_3,undefined4 param_4)

{
  int iVar1;
  uint in_EAX;
  int *piVar2;
  float *in_EDX;
  int iVar3;
  float *unaff_EDI;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  piVar2 = *(int **)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((((int)param_1 < *piVar2) && (piVar2 = (int *)(param_1 * 0x1c + piVar2[1]), param_2 != -1)) &&
     ((int)param_2 < *piVar2)) {
    iVar3 = param_2 * 0x30 + piVar2[1];
    local_38 = *unaff_EDI * 0.01 + *in_EDX;
    local_34 = unaff_EDI[1] * 0.01 + in_EDX[1];
    local_30 = unaff_EDI[2] * 0.01 + in_EDX[2];
    iVar1 = *(int *)(iVar3 + 0xc);
    if (iVar1 != -1) {
      FUN_00450980(iVar1,0xffffffff,0,1,0,&local_38);
    }
    if (*(int *)(iVar3 + 0x1c) != -1) {
      local_2c = local_38;
      local_24 = local_30;
      local_1c = unaff_EDI[1];
      local_28 = local_34;
      local_20 = *unaff_EDI;
      local_18 = unaff_EDI[2];
      local_14 = *(undefined4 *)PTR_DAT_00696714;
      local_10 = *(undefined4 *)(PTR_DAT_00696714 + 4);
      local_c = *(undefined4 *)(PTR_DAT_00696714 + 8);
      local_4 = param_3[1];
      local_8 = *param_3;
      FUN_00543d80(param_4);
    }
  }
  return;
}
#endif
