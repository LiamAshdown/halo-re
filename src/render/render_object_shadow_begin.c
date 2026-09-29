// render_object_shadow_begin  (Ghidra: FUN_0050f830; CEA render_object_shadow_begin(data, lod),
// hint only)
// address 0x50f830, size 327 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: objdump -d -M intel 0x50f830..0x50f976, written from the disassembly because the
//   Ghidra decompile drops the colour blend and the active camouflage scale entirely (it shows
//   the call to 0x530ff0 with only the radius and the out pointer). Traced:
//   - the object (EAX = object_render_data) supplies bounding_center (+0xa0) and bounding_radius
//     (+0xac);
//   - vector3d_build_perpendicular 0x4cd670 (ECX out, EDX = lighting.shadow_vector +0x5c), then
//     vector3d_normalize_with_length 0x401990 on the same ECX buffer (0x4cd670 leaves ECX alone);
//   - matrix4x3_from_forward_up 0x4cb970 (EAX up = shadow_vector, ECX forward = the
//     perpendicular, stack out = data->shadow_matrix at +0x0c), then the position row
//     (+0x28 of the matrix, data +0x34) = the object centre;
//   - fade = the stack argument, scaled by (1 - unit +0x37c) when the object is a biped or vehicle
//     ((1 << object.type) & 3) and unit +0x37c > 0.0 (0x672ac0);
//   - colour = lighting.shadow_color (+0x68) * fade + (1 - fade), componentwise, in a stack
//     buffer passed in EBX;
//   - rasterizer_object_shadow_begin 0x530ff0 (EAX matrix, EBX colour, stack radius, &data->
//     shadow_radius), whose AL result is returned (render_object 0x50eba0 tests AL at 0x50ed04).
// register convention: EAX = object_render_data*, one stack float (fade); returns AL.
//   // blam-cc: EAX=data, stack=fade
// UNSURE: unit +0x37c is the 1/120 ramped unit value render.h calls unit_37c (most likely the
//   active camouflage amount); the dead store of the object index into the argument slot at
//   0x50f943 is a register-allocation artefact and is not reproduced.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "objects.h"
#include "units.h"
#include "render.h"

extern data_array *object_data; // 0x008603b0, objects module

extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir);
    // 0x4cd670, math module; blam-cc: ECX -> out, EDX -> dir
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, math; blam-cc: ECX -> v
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out);
    // 0x4cb970, math module; blam-cc: EAX -> up, ECX -> forward, stack -> out
extern uint8_t rasterizer_object_shadow_begin(real_matrix4x3 *projection, ColorRGB *color,
                                              float radius, float *out_radius);
    // 0x530ff0, rasterizer module; blam-cc: EAX -> projection, EBX -> color,
    // stack -> (radius, out_radius); always returns 1

// Builds the shadow projection of the object in data: a basis whose up axis is the cached
// lighting's shadow vector, centred on the object, and a shadow colour that fades from white
// toward lighting.shadow_color by `fade` (reduced by the unit camouflage amount), then hands both
// to the rasterizer, which writes the shadow radius into data->shadow_radius.
uint8_t render_object_shadow_begin(object_render_data *data, float fade) // blam-cc: EAX=data, stack=fade
{
    object *o = ((object_header *)object_data->data)[(uint16_t)data->object_index].data;
    render_lighting *lighting = (render_lighting *)data->lighting;
    real_point3d center = o->bounding_center;
    float radius = o->bounding_radius;
    real_vector3d forward;
    ColorRGB color;
    float t;

    vector3d_build_perpendicular(&forward, &lighting->shadow_vector);
    vector3d_normalize_with_length(&forward);
    matrix4x3_from_forward_up(&lighting->shadow_vector, &forward, &data->shadow_matrix);
    data->shadow_matrix.position = center;

    color = lighting->shadow_color;

    // re-fetched through the object header, as the binary does
    o = ((object_header *)object_data->data)[(uint16_t)data->object_index].data;
    if (((1 << (uint8_t)o->type) & 3) != 0) {
        unit_data *unit = (unit_data *)((uint8_t *)o + k_unit_data_offset);
        if (unit->active_camouflage_power > 0.0f) {
            t = (1.0f - unit->active_camouflage_power) * fade;
        } else {
            t = fade;
        }
    } else {
        t = fade;
    }

    color.red = color.red * t + (1.0f - t);
    color.green = color.green * t + (1.0f - t);
    color.blue = color.blue * t + (1.0f - t);

    return rasterizer_object_shadow_begin(&data->shadow_matrix, &color, radius, &data->shadow_radius);
}

#if 0
Original Ghidra decompilation (0x50f830):

void FUN_0050f830(void)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  undefined4 uVar5;
  uint *in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*in_EAX & 0xffff) * 0xc);
  uVar2 = *(uint *)(iVar1 + 0xa0);
  uVar3 = *(uint *)(iVar1 + 0xa4);
  uVar4 = *(uint *)(iVar1 + 0xa8);
  uVar5 = *(undefined4 *)(iVar1 + 0xac);
  vector3d_build_perpendicular();
  vector3d_normalize_with_length();
  matrix4x3_from_forward_up(in_EAX + 3);
  in_EAX[0xd] = uVar2;
  in_EAX[0xe] = uVar3;
  in_EAX[0xf] = uVar4;
  FUN_00530ff0(uVar5,in_EAX + 0x10);
  return;
}
#endif
