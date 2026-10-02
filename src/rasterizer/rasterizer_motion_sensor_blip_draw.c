// rasterizer_motion_sensor_blip_draw  (Ghidra: FUN_0052bad0, unnamed; the earlier draft
//   called it rasterizer_debug_marker_draw_small)
// address 0x52bad0, size 362 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: the only caller is FUN_004b37a0, the per blip helper of motion_sensor_render
//   0x4b4120 (interface module), which calls it once per detected object. It only draws while
//   rasterizer_motion_sensor_begin 0x52b690 has redirected rendering into render target 5
//   (0x0071d205 set) and the 0x00689403 toggle is on. Rebuilt from the raw disassembly: Ghidra
//   lost the EDI color argument and read the second stack slot for both float arguments (the
//   first fld runs before push esi, the second after it).
// register convention: EAX -> position (float[2], sensor space), EDI -> color (real RGB, 0..1),
//   stack -> (brightness, size).
// VERIFIED against disassembly 0x52bad0..0x52bc3a (2026-09-30): argument slots (first stack arg brightness, second size), packed
//   ARGB (alpha forced 0xff), the four vertices {x,y,z,color,u,v} and DrawPrimitiveUP(6, 2, 0x18) via device vtable +0x14c.
// blam-cc: EAX -> position, EDI -> color, stack -> (brightness, size)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                    // 0x0071d174
extern uint8_t console_debug_toggle_689403;        // 0x00689403 motion sensor rendering enabled
extern uint8_t rasterizer_motion_sensor_ready;     // 0x0071d205 set by rasterizer_motion_sensor_begin

typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t type, uint32_t count, const void *data, uint32_t stride);

// Draws one blip as a two triangle fan quad of half size size/16 around (x, y) * -1/32, with the
// color scaled by brightness and forced opaque.
void rasterizer_motion_sensor_blip_draw(const float *position, const float *color, float brightness, float size)
{
    rasterizer_dynamic_screen_vertex vertices[4];
    float half_size;
    float x, y;
    uint32_t packed;
    int i;

    if (!console_debug_toggle_689403 || !rasterizer_motion_sensor_ready) {
        return;
    }
    half_size = size * 0.0625f;
    x = position[0] * -0.03125f;
    y = position[1] * -0.03125f;
    packed = (uint32_t)(int32_t)(brightness * color[0] * 255.0f);   // __ftol
    packed = (packed | 0xffffff00) << 8;
    packed = (packed | ((uint32_t)(int32_t)(brightness * color[1] * 255.0f) & 0xff)) << 8;
    packed = packed | ((uint32_t)(int32_t)(brightness * color[2] * 255.0f) & 0xff);

    vertices[0].x = x - half_size;
    vertices[0].y = y + half_size;
    vertices[0].u = 0.0f;
    vertices[0].v = 0.0f;
    vertices[1].x = x + half_size;
    vertices[1].y = y + half_size;
    vertices[1].u = 1.0f;
    vertices[1].v = 0.0f;
    vertices[2].x = x + half_size;
    vertices[2].y = y - half_size;
    vertices[2].u = 1.0f;
    vertices[2].v = 1.0f;
    vertices[3].x = x - half_size;
    vertices[3].y = y - half_size;
    vertices[3].u = 0.0f;
    vertices[3].v = 1.0f;
    for (i = 0; i < 4; i++) {
        vertices[i].z = 0.0f;
        vertices[i].color = packed;
    }
    ((d3d_draw_primitive_up_fn)(*(void ***)rasterizer_device)[0x14c / 4])(rasterizer_device, 6, 2, vertices,
                                                                          sizeof(rasterizer_dynamic_screen_vertex));
}

#if 0
Original Ghidra decompilation (0x52bad0):

void FUN_0052bad0(undefined4 param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float *in_EAX;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  float local_60;
  float local_5c;
  undefined4 local_58;
  uint local_54;
  undefined4 local_50;
  undefined4 local_4c;
  float local_48;
  float local_44;
  undefined4 local_40;
  uint local_3c;
  undefined4 local_38;
  undefined4 local_34;
  float local_30;
  float local_2c;
  undefined4 local_28;
  uint local_24;
  undefined4 local_20;
  undefined4 local_1c;
  float local_18;
  float local_14;
  undefined4 local_10;
  uint local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  if ((DAT_00689403 != '\0') && (DAT_0071d205 != '\0')) {
    param_2 = param_2 * 0.0625;
    fVar1 = *in_EAX;
    fVar2 = in_EAX[1];
    uVar3 = __ftol();
    uVar4 = __ftol();
    uVar5 = __ftol();
    local_60 = fVar1 * -0.03125 - param_2;
    local_54 = ((uVar3 | 0xffffff00) << 8 | uVar4 & 0xff) << 8 | uVar5 & 0xff;
    local_5c = fVar2 * -0.03125 + param_2;
    local_50 = 0;
    local_48 = fVar1 * -0.03125 + param_2;
    local_4c = 0;
    local_38 = 0x3f800000;
    local_34 = 0;
    local_20 = 0x3f800000;
    local_1c = 0x3f800000;
    local_8 = 0;
    local_2c = fVar2 * -0.03125 - param_2;
    local_4 = 0x3f800000;
    local_10 = 0;
    local_28 = 0;
    local_40 = 0;
    local_58 = 0;
    local_44 = local_5c;
    local_3c = local_54;
    local_30 = local_48;
    local_24 = local_54;
    local_18 = local_60;
    local_14 = local_2c;
    local_c = local_54;
    (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&local_60,0x18);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
