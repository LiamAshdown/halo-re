// damage_effect_new_at_location
// address 0x4f0010, size 640 bytes
// name confidence: 0.5
// rewrite confidence: 0.85
// REWRITTEN from objdump 0x4f0010..0x4f02c7 and its caller object_apply_body_damage (0x4ef7ac). EAX: the
//   damage direction, ECX: the hit plane or 0, EBX: the impact point, EDI: the object; stack (effect, node).
//   Builds the five named effect vectors -- "normal" (the plane normal, or the impact point's direction from
//   the object, else its forward), "incident" (-direction), "negative incident" (direction), "reflection"
//   (direction reflected about the normal) and "gravity" (*0x69672c) -- all at the impact point, and spawns the
//   effect on the object's node (0x450870: EAX creator, ECX effect, EDX object, scale 1 / 0) or, with no object
//   or node, in the world (0x450980, velocity *0x696714). A zero direction falls back to *0x696718.
// blam-cc: EAX=normal, ECX=incident, EBX=impact_position, EDI=object_index, stack=(effect_tag, node_index)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include <stdint.h>
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern real_vector3d *global_down3d_pointer;    // 0x0069672c, the gravity direction
extern real_vector3d *global_forward3d_pointer; // 0x00696718
extern real_vector3d *global_origin3d_pointer;  // 0x00696714

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX, ECX
extern datum_index effect_new_on_object_with_node_table(datum_index creator_object_index,
    datum_index definition_index, datum_index object_index, uint16_t node_index,
    uint16_t ctx_08, uint32_t ctx_0c, uint32_t ctx_10, uint32_t ctx_14, real a_scale,
    real b_scale, const ColorRGB *color, const effect_tint_source *tint_source); // 0x450870, EAX, ECX, EDX, stack
extern datum_index effect_new_with_color(datum_index definition_index, datum_index creator_object_index,
    const real_vector3d *velocity, uint16_t ctx_08, uint32_t ctx_0c, real_point3d *position,
    uint32_t ctx_14, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source,
    uint8_t force_create); // 0x450980

void damage_effect_new_at_location(datum_index effect_tag, int16_t node_index, real_vector3d *normal,
    real_vector3d *incident, real_point3d *impact_position, uint32_t object_index)
{
    static const char *const k_names[5] = { "normal", "incident", "negative incident", "reflection", "gravity" };
    const char *names[5];
    real_vector3d gravity = *global_down3d_pointer;
    real_vector3d direction = *normal;
    real_vector3d vectors[5];   // normal, incident, negative incident, reflection, gravity
    real_point3d positions[5];
    int32_t i;

    for (i = 0; i < 5; i++) {
        names[i] = k_names[i];
    }
    if (vector3d_normalize_with_length(&direction) == 0.0f) {
        direction = *global_forward3d_pointer;
    }
    vectors[1].i = direction.i * -1.0f;
    vectors[1].j = direction.j * -1.0f;
    vectors[1].k = direction.k * -1.0f;
    vectors[2] = direction;
    if (incident == 0) {
        real_point3d object_position;
        real_vector3d away;
        float dot2;

        object_get_position(&object_position, object_index);
        away.i = impact_position->x - object_position.x;
        away.j = impact_position->y - object_position.y;
        away.k = impact_position->z - object_position.z;
        if (vector3d_normalize_with_length(&away) == 0.0f) {
            away = *(real_vector3d *)((uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data + 0x74);
        }
        vectors[0] = away;
        dot2 = away.j * direction.j + direction.k * away.k + direction.i * away.i;
        dot2 = dot2 + dot2;
        vectors[3].i = direction.i - away.i * dot2;
        vectors[3].j = direction.j - away.j * dot2;
        vectors[3].k = direction.k - away.k * dot2;
    } else {
        float dot2;

        vectors[0] = *incident;
        dot2 = direction.k * incident->k + direction.j * incident->j + direction.i * incident->i;
        dot2 = dot2 + dot2;
        vectors[3].i = direction.i - dot2 * incident->i;
        vectors[3].j = direction.j - dot2 * incident->j;
        vectors[3].k = direction.k - dot2 * incident->k;
    }
    vectors[4] = gravity;
    for (i = 0; i < 5; i++) {
        positions[i] = *impact_position;
    }
    if (object_index != 0xffffffff && node_index != -1) {
        effect_new_on_object_with_node_table(object_index, effect_tag, object_index, (uint16_t)node_index, 5,
            (uint32_t)(uintptr_t)names, (uint32_t)(uintptr_t)positions, (uint32_t)(uintptr_t)vectors,
            1.0f, 0.0f, 0, 0);
        return;
    }
    effect_new_with_color(effect_tag, object_index, global_origin3d_pointer, 5, (uint32_t)(uintptr_t)names,
        positions, (uint32_t)(uintptr_t)vectors, 1.0f, 0.0f, 0, 0, 0);
}

#if 0
Original Ghidra decompilation (0x4f0010):

void damage_effect_new_at_location(undefined4 param_1,undefined4 param_2)

{
  float fVar1;
  float *in_EAX;
  float *pfVar2;
  float *in_ECX;
  int iVar3;
  float *unaff_EBX;
  uint unaff_EDI;
  float10 fVar4;
  float local_b0;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  float local_98;
  float local_94;
  float local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  float local_70;
  float local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  char *local_5c;
  char *local_58;
  char *local_54;
  char *local_50;
  char *local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c [15];

  local_68 = *(undefined4 *)PTR_DAT_0069672c;
  local_64 = *(undefined4 *)(PTR_DAT_0069672c + 4);
  local_60 = *(undefined4 *)(PTR_DAT_0069672c + 8);
  local_b0 = *in_EAX;
  local_ac = in_EAX[1];
  local_a8 = in_EAX[2];
  local_5c = "normal";
  local_58 = "incident";
  local_54 = "negative incident";
  local_50 = "reflection";
  local_4c = "gravity";
  fVar4 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 == fVar4) {
    local_b0 = *(float *)PTR_DAT_00696718;
    local_ac = *(float *)(PTR_DAT_00696718 + 4);
    local_a8 = *(float *)(PTR_DAT_00696718 + 8);
  }
  local_8c = local_b0 * -1.0;
  local_80 = local_b0;
  local_7c = local_ac;
  local_88 = local_ac * -1.0;
  local_78 = local_a8;
  local_84 = local_a8 * -1.0;
  if (in_ECX == (float *)0x0) {
    object_get_position();
    local_a4 = *unaff_EBX - local_48;
    local_a0 = unaff_EBX[1] - local_44;
    local_9c = unaff_EBX[2] - local_40;
    fVar4 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar4) {
      iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
      local_a4 = *(float *)(iVar3 + 0x74);
      local_a0 = *(float *)(iVar3 + 0x78);
      local_9c = *(float *)(iVar3 + 0x7c);
    }
    local_98 = local_a4;
    local_94 = local_a0;
    local_90 = local_9c;
    fVar1 = local_b0 * local_a4 + local_a8 * local_9c + local_a0 * local_ac;
    fVar1 = fVar1 + fVar1;
    local_a4 = local_a4 * fVar1;
    local_a0 = local_a0 * fVar1;
    local_9c = local_9c * fVar1;
  }
  else {
    local_98 = *in_ECX;
    local_94 = in_ECX[1];
    local_90 = in_ECX[2];
    local_9c = local_b0 * *in_ECX + local_ac * in_ECX[1] + local_a8 * in_ECX[2];
    local_9c = local_9c + local_9c;
    local_a4 = local_9c * *in_ECX;
    local_a0 = local_9c * in_ECX[1];
    local_9c = local_9c * in_ECX[2];
  }
  local_6c = local_a8 - local_9c;
  local_70 = local_ac - local_a0;
  local_74 = local_b0 - local_a4;
  iVar3 = 5;
  pfVar2 = local_3c;
  do {
    *pfVar2 = *unaff_EBX;
    fVar1 = unaff_EBX[2];
    pfVar2[1] = unaff_EBX[1];
    iVar3 = iVar3 + -1;
    pfVar2[2] = fVar1;
    pfVar2 = pfVar2 + 3;
  } while (iVar3 != 0);
  if ((unaff_EDI != 0xffffffff) && ((short)param_2 != -1)) {
    FUN_00450870(param_2,5,&local_5c,local_3c,&local_98,0x3f800000,0,0,0);
    return;
  }
  FUN_00450980(param_1);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
