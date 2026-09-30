// rasterizer_set_fog_constants  (Ghidra: FUN_005176d0, unnamed)
// address 0x5176d0, size 1201 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: called only by rasterizer_begin_frame 0x5175c0 with EDX = &window->fog (lea
//   edx,[ebp+0x1e8]); copies the 0x14 dword render_fog block to rasterizer_window.fog
//   (0x007c1408), sanitises it, uploads four vec4 fog constants at vertex shader register 6
//   (SetVertexShaderConstantF, device +0x178) and sets the fixed-function fog render states
//   (SetRenderState +0xe4: 0x1c FOGENABLE, 0x22 FOGCOLOR, 0x23 FOGTABLEMODE, 0x8c FOGVERTEXMODE
//   = 3 LINEAR, 0x24 FOGSTART, 0x25 FOGEND). The pre ps_1_1 FOGEND value, which Ghidra lost
//   (it printed a denormal literal), is taken from the raw code at 0x517a7e..0x517a94.
// register convention: EDX = render_fog* (live-in), no stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern d3d_caps9 rasterizer_caps;                      // 0x007c10c0
extern void *rasterizer_device;                        // 0x0071d174
extern uint8_t console_debug_toggle_689407;            // 0x00689407 atmospheric fog enable
extern uint8_t console_debug_toggle_689408;            // 0x00689408 planar fog enable
extern uint8_t console_debug_toggle_6893fc;            // 0x006893fc fixed-function fog enable
extern uint8_t rasterizer_fog_enabled;                              // 0x0069c6a8 latched by rasterizer_set_fog_constants
extern const ColorRGB *global_white_color;                          // 0x00686b04 -> 0x0065513c (1, 1, 1)
extern uint32_t color_rgb_float_to_int(const ColorRGB *color); // 0x4ab5d0
// blam-cc: normal in ECX, point in EDX, plane on the stack
extern void plane3d_from_point_and_normal(real_plane3d *plane, const real_vector3d *normal, const real_point3d *point); // 0x44d9e0

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);
typedef int32_t (__stdcall *d3d_set_vertex_shader_constant_f_fn)(void *device, uint32_t start_register, const float *data, uint32_t vector4f_count);

static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    void **vtable = *(void ***)rasterizer_device;
    ((d3d_set_render_state_fn)vtable[0xe4 / 4])(rasterizer_device, state, value);
}

static float real_pin_unit(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static uint32_t real_bits(float value)
{
    return *(uint32_t *)&value;
}

// blam-cc: EDX -> fog
void rasterizer_set_fog_constants(const render_fog *fog)
{
    render_fog *window_fog = &rasterizer_window.fog;
    const real_point3d *camera_position = &rasterizer_window.camera.position;
    const real_vector3d *camera_forward = &rasterizer_window.camera.forward;
    float constants[16];
    float atmospheric_scale;
    float camera_depth;
    float planar_distance_scale;
    float planar_depth_scale;
    void **vtable;

    *window_fog = *fog;

    // atmospheric fog
    if (window_fog->atmospheric_maximum_density <= 0.0f) {
        window_fog->atmospheric_maximum_density = 1.0f;
    }
    if (window_fog->atmospheric_maximum_distance == 0.0f || console_debug_toggle_689407 == 0) {
        window_fog->atmospheric_maximum_distance = rasterizer_window.camera.z_far + rasterizer_window.camera.z_far;
        window_fog->atmospheric_maximum_density = 0.0f;
        window_fog->atmospheric_minimum_distance = rasterizer_window.camera.z_far;
    }

    // planar fog
    if (window_fog->planar_maximum_density <= 0.0f) {
        window_fog->planar_maximum_density = 1.0f;
    }
    if (window_fog->planar_mode == 0 || (fog->flags & _render_fog_no_planar_bit) != 0 ||
        console_debug_toggle_689408 == 0) {
        // no planar fog: a camera aligned plane with zero density
        window_fog->planar_mode = 0;
        window_fog->planar_maximum_density = 0.0f;
        window_fog->planar_maximum_distance = 1.0f;
        window_fog->planar_maximum_depth = 1.0f;
        window_fog->planar_color = *global_white_color;
        window_fog->plane.normal = *camera_forward;
        window_fog->plane.d = camera_position->x * camera_forward->i +
                              camera_position->y * camera_forward->j +
                              camera_position->z * camera_forward->k;
    } else if (window_fog->planar_mode == 2) {
        // plane through the camera, pushed out to the far plane
        window_fog->planar_maximum_depth = 1.0f;
        plane3d_from_point_and_normal(&window_fog->plane, camera_forward, camera_position);
        window_fog->plane.d = window_fog->plane.d + rasterizer_window.camera.z_far;
    }

    atmospheric_scale = 1.0f / (window_fog->atmospheric_maximum_distance - window_fog->atmospheric_minimum_distance);
    camera_depth = camera_position->x * camera_forward->i + camera_position->y * camera_forward->j +
                   camera_position->z * camera_forward->k;
    planar_distance_scale = 1.0f / window_fog->planar_maximum_distance;
    planar_depth_scale = 1.0f / window_fog->planar_maximum_depth;

    // c6: atmospheric distance ramp along the view axis
    constants[0] = camera_forward->i * atmospheric_scale;
    constants[1] = camera_forward->j * atmospheric_scale;
    constants[2] = camera_forward->k * atmospheric_scale;
    constants[3] = -((window_fog->atmospheric_minimum_distance + camera_depth) * atmospheric_scale);
    // c7: planar fog plane scaled by 1 / maximum depth
    constants[4] = -(window_fog->plane.normal.i * planar_depth_scale);
    constants[5] = -(window_fog->plane.normal.j * planar_depth_scale);
    constants[6] = -(window_fog->plane.normal.k * planar_depth_scale);
    constants[7] = window_fog->plane.d * planar_depth_scale;
    // c8: planar fog distance ramp along the view axis
    constants[8] = camera_forward->i * planar_distance_scale;
    constants[9] = camera_forward->j * planar_distance_scale;
    constants[10] = camera_forward->k * planar_distance_scale;
    constants[11] = -(planar_distance_scale * camera_depth);
    // c9: atmospheric density, camera depth below the fog plane, planar density, 3.0
    constants[12] = real_pin_unit(window_fog->atmospheric_maximum_density);
    constants[13] = real_pin_unit(-(planar_depth_scale *
                                    ((camera_position->x * window_fog->plane.normal.i +
                                      camera_position->y * window_fog->plane.normal.j +
                                      camera_position->z * window_fog->plane.normal.k) - window_fog->plane.d)));
    constants[14] = real_pin_unit(window_fog->planar_maximum_density);
    constants[15] = 3.0f;

    vtable = *(void ***)rasterizer_device;
    ((d3d_set_vertex_shader_constant_f_fn)vtable[0x178 / 4])(rasterizer_device, 6, constants, 4);

    // fixed-function fog
    rasterizer_fog_enabled = console_debug_toggle_6893fc;
    rasterizer_set_render_state(0x1c, rasterizer_fog_enabled);                              // D3DRS_FOGENABLE
    rasterizer_set_render_state(0x22, color_rgb_float_to_int(&window_fog->atmospheric_color)); // D3DRS_FOGCOLOR
    rasterizer_set_render_state(0x23, 0);                                                   // D3DRS_FOGTABLEMODE none
    rasterizer_set_render_state(0x8c, 3);                                                   // D3DRS_FOGVERTEXMODE linear
    rasterizer_set_render_state(0x24, real_bits(window_fog->atmospheric_minimum_distance)); // D3DRS_FOGSTART
    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        // pre ps_1_1: stretch the end so that the far plane gets exactly the maximum density
        float z_far = rasterizer_window.frustum.z_far;
        rasterizer_set_render_state(0x25, real_bits((z_far - window_fog->atmospheric_maximum_density * z_far) +
                                                    window_fog->atmospheric_maximum_distance)); // D3DRS_FOGEND
    } else {
        rasterizer_set_render_state(0x25, real_bits(window_fog->atmospheric_maximum_distance)); // D3DRS_FOGEND
    }
}

#if 0
Original Ghidra decompilation (0x5176d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_005176d0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  uint uVar4;
  int iVar5;
  byte *in_EDX;
  byte *pbVar6;
  undefined4 *puVar7;
  float fVar8;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  undefined4 local_4;
  
  pbVar6 = in_EDX;
  puVar7 = (undefined4 *)&DAT_007c1408;
  for (iVar5 = 0x14; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar7 = *(undefined4 *)pbVar6;
    pbVar6 = pbVar6 + 4;
    puVar7 = puVar7 + 1;
  }
  if (DAT_007c1418 < 0.0 != (DAT_007c1418 == 0.0)) {
    DAT_007c1418 = 1.0;
  }
  if ((DAT_007c1420 == 0.0) || (DAT_00689407 == '\0')) {
    DAT_007c1420 = DAT_007c1268 + DAT_007c1268;
    DAT_007c1418 = 0.0;
    DAT_007c141c = DAT_007c1268;
  }
  if (DAT_007c1444 < 0.0 != (DAT_007c1444 == 0.0)) {
    DAT_007c1444 = 1.0;
  }
  if (((DAT_007c1424 == 0) || ((*in_EDX & 4) != 0)) || (DAT_00689408 == '\0')) {
    DAT_007c1424 = 0;
    DAT_007c1444 = 0.0;
    _DAT_007c1448 = 1.0;
    _DAT_007c144c = 1.0;
    DAT_007c1438 = *(undefined4 *)PTR_DAT_00686b04;
    DAT_007c143c = *(undefined4 *)(PTR_DAT_00686b04 + 4);
    DAT_007c1440 = *(undefined4 *)(PTR_DAT_00686b04 + 8);
    _DAT_007c1430 = DAT_007c123c;
    _DAT_007c142c = DAT_007c1238;
    _DAT_007c1428 = DAT_007c1234;
    _DAT_007c1434 =
         DAT_007c1228 * DAT_007c1234 + DAT_007c122c * DAT_007c1238 + DAT_007c1230 * DAT_007c123c;
  }
  else if (DAT_007c1424 == 2) {
    _DAT_007c144c = 1.0;
    FUN_0044d9e0(&DAT_007c1428);
    _DAT_007c1434 = _DAT_007c1434 + DAT_007c1268;
  }
  fVar8 = 1.0 / (DAT_007c1420 - DAT_007c141c);
  fVar1 = DAT_007c1228 * DAT_007c1234 + DAT_007c122c * DAT_007c1238 + DAT_007c1230 * DAT_007c123c;
  fVar2 = 1.0 / _DAT_007c1448;
  fVar3 = 1.0 / _DAT_007c144c;
  local_40 = DAT_007c1234 * fVar8;
  local_3c = DAT_007c1238 * fVar8;
  local_38 = DAT_007c123c * fVar8;
  local_34 = -((DAT_007c141c + fVar1) * fVar8);
  local_30 = -(_DAT_007c1428 * fVar3);
  local_2c = -(_DAT_007c142c * fVar3);
  local_28 = -(_DAT_007c1430 * fVar3);
  local_24 = _DAT_007c1434 * fVar3;
  local_20 = DAT_007c1234 * fVar2;
  local_1c = DAT_007c1238 * fVar2;
  local_18 = DAT_007c123c * fVar2;
  local_14 = -(fVar2 * fVar1);
  if (0.0 <= DAT_007c1418) {
    if (DAT_007c1418 <= 1.0) {
      local_10 = DAT_007c1418;
    }
    else {
      local_10 = 1.0;
    }
  }
  else {
    local_10 = 0.0;
  }
  local_c = -(fVar3 * ((DAT_007c1228 * _DAT_007c1428 +
                       DAT_007c122c * _DAT_007c142c + DAT_007c1230 * _DAT_007c1430) - _DAT_007c1434)
             );
  if (0.0 <= local_c) {
    if (1.0 < local_c) {
      local_c = 1.0;
    }
  }
  else {
    local_c = 0.0;
  }
  if (0.0 <= DAT_007c1444) {
    if (DAT_007c1444 <= 1.0) {
      local_8 = DAT_007c1444;
    }
    else {
      local_8 = 1.0;
    }
  }
  else {
    local_8 = 0.0;
  }
  local_4 = 0x40400000;
  (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,6,&local_40,4);
  DAT_0069c6a8 = DAT_006893fc;
  if (DAT_007c118c < 0xffff0101) {
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uVar4 = color_rgb_float_to_int((float *)&DAT_007c140c);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x22,uVar4);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x23,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x8c,3);
    fVar8 = 5.04467e-44;
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x24,DAT_007c141c);
  }
  else {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,DAT_006893fc);
    uVar4 = color_rgb_float_to_int((float *)&DAT_007c140c);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x22,uVar4);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x23,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x8c,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x24,DAT_007c141c);
    fVar8 = DAT_007c1420;
  }
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x25,fVar8);
  return;
}
#endif
