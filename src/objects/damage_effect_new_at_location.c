// damage_effect_new_at_location
// address 0x4f0010, size 640 bytes
// name confidence: 0.9 (out/phase4/objects_functions.md, corroborated by the "normal"/"incident"/
// "negative incident"/"reflection"/"gravity" strings)
// rewrite confidence: 0.25
// evidence: types/objects.h object.position/forward/up (0x05c/0x074/0x080, via
// object_get_position); the five name strings themselves.
// UNSURE (function-wide): every register parameter here is implicit (in_EAX, in_ECX, unaff_EBX,
// unaff_EDI) with no visible assignment in the decompile, so their identities are inferred only
// from usage (in_EAX = normal vector, in_ECX = optional incident vector, unaff_EBX = impact
// position, unaff_EDI = object index). The 15-float block passed to effect_new_on_object_with_node_table as `&local_98`
// is reconstructed here as one contiguous struct spanning what Ghidra shows as five separate
// locals (local_98 through local_60), in stack order, on the theory that it is one flat array
// paired positionally with the five name strings — the fifth "vector" turns out to be the
// 3-float constant copied from PTR_DAT_0069672c at the top of the function, not a directly
// computed reflection/gravity vector, so this pairing is not independently confirmed.
// effect_new_on_object_with_node_table and effect_new_with_color are both outside this module (effects module).
// register convention: real_vector3d *normal in EAX (in_EAX); real_vector3d *incident in ECX
// (in_ECX, may be null); real_point3d *impact_position in EBX (unaff_EBX); uint32_t object_index
// in EDI (unaff_EDI); datum_index effect_tag on the stack (param_1); int16_t node_index on the
// stack (param_2).
// blam-cc: EAX=normal, ECX=incident, EBX=impact_position, EDI=object_index,
//   stack=(effect_tag, node_index)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern float g_0069672c[3];     // 0x0069672c, PTR_DAT_0069672c, UNSURE: a constant direction
extern real_vector3d object_placement_default_forward; // 0x00696718, PTR_DAT_00696718, the shared "forward" constant

extern real vector3d_normalize_with_length(real_vector3d *v); // math module, 0x401990
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900 (out of range)
extern void effect_new_on_object_with_node_table(); // effects module, 0x450870
    // The convention of this foreign callee is not established: different call sites in this
    // module pass different numbers of visible arguments, and it also takes values in EAX
    // and ECX that the decompiler never models. Declared with an empty parameter list so
    // every site in the module agrees on ONE declaration without fabricating arguments. // effects module, 0x450870
extern void effect_new_with_color(datum_index effect_tag); // effects module, 0x450980

void damage_effect_new_at_location(datum_index effect_tag, int16_t node_index, real_vector3d *normal,
    real_vector3d *incident, real_point3d *impact_position, uint32_t object_index)
{
    object_header *headers = (object_header *)object_data->data;
    real gravity_constant[3];
    real_vector3d n = *normal;
    char *names[5];
    real_point3d positions[5];
    damage_effect_vector_block vectors;
    int32_t i;

    gravity_constant[0] = g_0069672c[0];
    gravity_constant[1] = g_0069672c[1];
    gravity_constant[2] = g_0069672c[2];

    names[0] = "normal";
    names[1] = "incident";
    names[2] = "negative incident";
    names[3] = "reflection";
    names[4] = "gravity";

    if (vector3d_normalize_with_length(&n) == 0.0f) {
        n = object_placement_default_forward;
    }

    vectors.vector2.i = n.i; vectors.vector2.j = n.j; vectors.vector2.k = n.k;       // "normal"
    vectors.vector1.i = -n.i; vectors.vector1.j = -n.j; vectors.vector1.k = -n.k;    // -normal ("negative incident"?)

    if (incident == 0) {
        real_point3d object_position;
        real_vector3d to_object;

        object_get_position(&object_position, object_index);
        to_object.i = impact_position->x - object_position.x;
        to_object.j = impact_position->y - object_position.y;
        to_object.k = impact_position->z - object_position.z;

        if (vector3d_normalize_with_length(&to_object) == 0.0f) {
            object *obj = headers[object_index & 0xffff].data;
            to_object = obj->forward;
        }

        vectors.vector0.i = to_object.i; vectors.vector0.j = to_object.j; vectors.vector0.k = to_object.k; // "incident"

        {
            real dot2 = (n.i * to_object.i + n.k * to_object.k + to_object.j * n.j);
            dot2 = dot2 + dot2;
            vectors.vector0.i = to_object.i * dot2; // UNSURE: reused as "reflection" scratch, see original
            vectors.vector0.j = to_object.j * dot2;
            vectors.vector0.k = to_object.k * dot2;
        }
    } else {
        real dot2;

        vectors.vector0 = *incident;
        dot2 = n.i * incident->i + n.j * incident->j + n.k * incident->k;
        dot2 = dot2 + dot2;
        vectors.vector0.i = dot2 * incident->i;
        vectors.vector0.j = dot2 * incident->j;
        vectors.vector0.k = dot2 * incident->k;
    }

    vectors.vector3.i = n.i - vectors.vector0.i; // UNSURE: local_74/70/6c, "reflection"
    vectors.vector3.j = n.j - vectors.vector0.j;
    vectors.vector3.k = n.k - vectors.vector0.k;
    vectors.vector4.i = gravity_constant[0];
    vectors.vector4.j = gravity_constant[1];
    vectors.vector4.k = gravity_constant[2];

    for (i = 0; i < 5; i++) {
        positions[i] = *impact_position;
    }

    if (object_index != 0xffffffff && node_index != -1) {
        effect_new_on_object_with_node_table(node_index, 5, names, positions, &vectors.vector0, 0x3f800000, 0, 0, 0);
        return;
    }
    effect_new_with_color(effect_tag);
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
