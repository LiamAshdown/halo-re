// rasterizer_light_set  (Ghidra: already named)
// address 0x526760, size 452 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: globals resolve against types/rasterizer.h (pixel_shader_version, max_active_lights,
//   rasterizer_fixed_function_light_count, rasterizer_device); the source fields read through
//   unaff_ESI (`*unaff_ESI+0x70 == -1` for "no cube map", `unaff_ESI[1..3]` position,
//   `unaff_ESI[4..6]` forward, `unaff_ESI[10..12]` color, `unaff_ESI[0xd]` radius) match
//   rasterizer_light exactly (definition+0x70, position+0x04, forward+0x10, color+0x28,
//   radius+0x34), and the flag test `*(byte*)*unaff_ESI & 0x10` matches Light.flags bit 4
//   (first_person_flashlight) per types/rasterizer.h's own struct note. Device vtable+0xcc is
//   SetLight(DWORD Index, const D3DLIGHT9*) and +0xd4 is LightEnable, the same offsets used by
//   rasterizer_light_disable_all.c (indices 51 and 53 of IDirect3DDevice9). The local D3DLIGHT9
//   staging buffer is built field by field in the order Type(0x00), Diffuse RGB(0x04..0x10),
//   Position(0x34), Direction(0x40), Range(0x4c), Falloff(0x50), Attenuation1(0x58), Theta(0x60),
//   Phi(0x64), matching the SDK D3DLIGHT9 layout exactly (the 0x1a=26 dword zero-fill matches this
//   module's field layout, which omits any trailing padding); the field is therefore modeled here
//   as a raw float array rather than a named struct, since no D3DLIGHT9 typedef exists in
//   types/rasterizer.h.
// Phase 4 review fixes (raw disassembly 0x526760..0x526923): the point light test reads
//   Light.primary_cube_map.tag_id (definition +0x70), not the definition pointer itself, and the
//   flashlight offset is up x forward (ECX = up, stack = forward), not forward x up.
// register convention: the source rasterizer_light* is register-passed in ESI (Ghidra's
//   unaff_ESI); no other parameters recognized.
//   // blam-cc: ESI -> light (UNSURE: no other register explicitly observed, but this is the only
//   plausible source per the field reads above)
// UNSURE: the first_person_flashlight branch (`vector3d_cross_product(unaff_ESI + 4)` followed by
//   `vector3d_normalize_with_length()`) is called with its EAX (destination) and ECX (first
//   operand) implicit and not shown by the decompiler; by elimination (the explicit stack operand
//   is `light->forward`) this is modeled as `cross(light->forward, light->up)` written into, then
//   normalized in place within, a 3-float scratch vector, with the light's position offset by
//   -0.3 along each of that vector's components. The true operand/destination registers are not
//   confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern d3d_caps9 rasterizer_caps;                      // 0x007c10c0
extern void *rasterizer_device;                        // 0x0071d174
extern int32_t rasterizer_fixed_function_light_count;  // 0x007c3084

extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x004052c0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, blam-cc: ECX -> v

typedef int32_t (__stdcall *d3d_set_light_fn)(void *device, uint32_t index, const float *light);
typedef int32_t (__stdcall *d3d_light_enable_fn)(void *device, uint32_t index, int32_t enable);

// D3DLIGHTTYPE
#define D3DLIGHT_POINT 1
#define D3DLIGHT_SPOT  2

// Populates and enables one fixed-function Direct3D light from a game light object, for the
// pre-pixel-shader lighting fallback used when the device has no ps_1_1 support.
void rasterizer_light_set(rasterizer_light *light)
{
    uint32_t index;
    float d3dlight[26]; // D3DLIGHT9 staging buffer: Type, Diffuse, Specular, Ambient, Position,
                         // Direction, Range, Falloff, Attenuation0-2, Theta, Phi (see file header)
    void **vtable;

    index = rasterizer_fixed_function_light_count;
    if (rasterizer_caps.pixel_shader_version < 0xffff0101 && rasterizer_caps.max_active_lights != 0 &&
        rasterizer_fixed_function_light_count < (int32_t)rasterizer_caps.max_active_lights) {
        int i;
        for (i = 0; i < 26; i++) {
            d3dlight[i] = 0.0f;
        }

        d3dlight[0x58 / 4] = 1.4f;  // Attenuation1 (0x3fb33333), common to both branches
        d3dlight[0x54 / 4] = 0.0f;  // Attenuation0

        if (*(int32_t *)((uint8_t *)(uintptr_t)light->definition + 0x70) == -1) { // Light.primary_cube_map.tag_id
            // no cube map: an omni (point) light
            d3dlight[0x34 / 4] = light->position.x;
            d3dlight[0x04 / 4] = light->color.red * 10.0f;
            d3dlight[0x38 / 4] = light->position.y;
            d3dlight[0x3c / 4] = light->position.z;
            *(int32_t *)&d3dlight[0x00 / 4] = D3DLIGHT_POINT;
            d3dlight[0x5c / 4] = 0.0f; // Attenuation2
            d3dlight[0x08 / 4] = light->color.green * 10.0f;
            d3dlight[0x0c / 4] = light->color.blue * 10.0f;
        } else {
            float radius_scale;
            real_vector3d flashlight_offset;

            d3dlight[0x40 / 4] = light->forward.i;
            d3dlight[0x44 / 4] = light->forward.j;
            d3dlight[0x34 / 4] = light->position.x;
            d3dlight[0x38 / 4] = light->position.y;
            radius_scale = light->radius * 2.5f;
            d3dlight[0x3c / 4] = light->position.z;
            *(int32_t *)&d3dlight[0x00 / 4] = D3DLIGHT_SPOT;
            d3dlight[0x60 / 4] = 1.0f;   // Theta
            d3dlight[0x04 / 4] = radius_scale * light->color.red;
            d3dlight[0x64 / 4] = 3.14f;  // Phi
            d3dlight[0x50 / 4] = 2.0f;   // Falloff
            d3dlight[0x5c / 4] = 1.0f;   // Attenuation2
            d3dlight[0x08 / 4] = radius_scale * light->color.green;
            d3dlight[0x0c / 4] = radius_scale * light->color.blue;
            d3dlight[0x48 / 4] = light->forward.k;

            if ((*(uint8_t *)(uintptr_t)light->definition & 0x10) != 0) {
                // first_person_flashlight: offset the light position sideways so it doesn't sit
                // directly on the camera axis. UNSURE, see file header.
                vector3d_cross_product(&flashlight_offset, &light->up, &light->forward); // ECX = up, stack = forward
                vector3d_normalize_with_length(&flashlight_offset);
                d3dlight[0x34 / 4] -= flashlight_offset.i * 0.3f;
                d3dlight[0x38 / 4] -= flashlight_offset.j * 0.3f;
                d3dlight[0x3c / 4] -= flashlight_offset.k * 0.3f;
            }
        }

        d3dlight[0x4c / 4] = light->radius;

        vtable = *(void ***)rasterizer_device;
        ((d3d_set_light_fn)vtable[0xcc / 4])(rasterizer_device, index, d3dlight);
        ((d3d_light_enable_fn)vtable[0xd4 / 4])(rasterizer_device, rasterizer_fixed_function_light_count, 1);
        rasterizer_fixed_function_light_count = rasterizer_fixed_function_light_count + 1;
    }
}

#if 0
Original Ghidra decompilation (0x526760):

void rasterizer_light_set(void)

{
  float fVar1;
  uint uVar2;
  int iVar3;
  undefined4 *unaff_ESI;
  float *pfVar4;
  float local_74;
  float local_70;
  float local_68 [13];
  float local_34;
  float local_30;
  float local_2c;
  undefined4 local_28;
  undefined4 local_24;
  float local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  uVar2 = DAT_007c3084;
  if (((DAT_007c118c < 0xffff0101) && (DAT_007c1160 != 0)) && (DAT_007c3084 < DAT_007c1160)) {
    pfVar4 = local_68;
    for (iVar3 = 0x1a; iVar3 != 0; iVar3 = iVar3 + -1) {
      *pfVar4 = 0.0;
      pfVar4 = pfVar4 + 1;
    }
    local_10 = 0x3fb33333;
    local_14 = 0;
    if (*(int *)((byte *)*unaff_ESI + 0x70) == -1) {
      local_34 = (float)unaff_ESI[1];
      local_68[1] = (float)unaff_ESI[10] * 10.0;
      local_30 = (float)unaff_ESI[2];
      local_2c = (float)unaff_ESI[3];
      local_68[0] = 1.4013e-45;
      local_c = 0;
      local_68[2] = (float)unaff_ESI[0xb] * 10.0;
      local_68[3] = (float)unaff_ESI[0xc] * 10.0;
    }
    else {
      local_28 = unaff_ESI[4];
      fVar1 = (float)unaff_ESI[6];
      local_24 = unaff_ESI[5];
      local_34 = (float)unaff_ESI[1];
      local_30 = (float)unaff_ESI[2];
      local_68[3] = (float)unaff_ESI[0xd] * 2.5;
      local_2c = (float)unaff_ESI[3];
      local_68[0] = 2.8026e-45;
      local_8 = 0x3f800000;
      local_68[1] = local_68[3] * (float)unaff_ESI[10];
      local_4 = 0x4048f5c3;
      local_18 = 0x40000000;
      local_c = 0x3f800000;
      local_68[2] = local_68[3] * (float)unaff_ESI[0xb];
      local_68[3] = local_68[3] * (float)unaff_ESI[0xc];
      local_20 = fVar1;
      if ((*(byte *)*unaff_ESI & 0x10) != 0) {
        vector3d_cross_product(unaff_ESI + 4);
        vector3d_normalize_with_length();
        local_34 = local_34 - local_74 * 0.3;
        local_30 = local_30 - local_70 * 0.3;
        local_2c = local_2c - fVar1 * 0.3;
      }
    }
    local_1c = unaff_ESI[0xd];
    (**(code **)(*DAT_0071d174 + 0xcc))(DAT_0071d174,uVar2,local_68);
    (**(code **)(*DAT_0071d174 + 0xd4))(DAT_0071d174,DAT_007c3084,1);
    DAT_007c3084 = DAT_007c3084 + 1;
  }
  return;
}
#endif
