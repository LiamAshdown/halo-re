// motion_sensor_plot_blip  (Ghidra: motion_sensor_plot_blip, already named)
// address 0x4b37a0, size 370 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4b37a0..0x4b3911 in the phase-4 review (the first rewrite
// had the wrong arguments and rotation). EAX is the blip position relative to the sensor
// center in world units, BL the blip type. The position is rotated by -viewer_facing, dropped
// outside HUDGlobals motion_sensor_range (+0x2d0), pulled in radially to range * (d / range)
// ^ 0.7 (d at least 1/64), scaled to pixels and drawn by the rasterizer blip call 0x52bad0
// (EAX point, EDI the blip color of 0x0065c108 + type * 12, stack alpha, size) with size =
// pulse * size_factor + the subtype size of 0x00692fd4; type 5 pulses with (sin(t * 0.1047)
// + 1) / 3 + 1.
// register convention: EAX position, BL type; five stack arguments.
//   // blam-cc: position -> EAX, type -> BL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern game_time_globals *game_time;              // 0x006f1d6c
extern float motion_sensor_blip_subtype_size[3];  // 0x00692fd4: 0.0, -0.75, 1.0
extern ColorRGB motion_sensor_blip_colors[6];     // 0x0065c108, per blip type

extern float sinf(float x);
extern float cosf(float x);
extern float sqrtf(float x);
extern double sin(double x);
extern double pow(double base, double exponent); // 0x6283c0, MSVC 7.1 CRT _CIpow
extern void rasterizer_motion_sensor_blip_draw(const float *point, const ColorRGB *color, float alpha, float size); // 0x52bad0, rasterizer blip, blam-cc: EAX point, EDI color

// blam-cc: position -> EAX, type -> BL
void motion_sensor_plot_blip(const float *position, uint8_t type, const motion_sensor_frame *frame, int8_t subtype,
                             float pixels_per_unit, float alpha, float size_factor)
{
    float x = position[0];
    float y = position[1];
    float sine = sinf(-frame->viewer_facing);
    float cosine = cosf(-frame->viewer_facing);
    float u = y * cosine + x * sine;
    float v = x * cosine - y * sine;
    float range = *(float *)((uint8_t *)hud_globals_tag_data + 0x2d0); // motion sensor range
    float distance;
    float pulled;
    float point[2];
    float pulse;
    float size;

    if (!(v * v + u * u < range * range)) {
        return;
    }
    distance = sqrtf(v * v + u * u);
    if (distance < 0.015625f) {
        distance = 0.015625f;
    }
    pulled = (float)pow((double)(distance / range), 0.7) * range;
    point[0] = v * (1.0f / distance) * pulled * pixels_per_unit;
    point[1] = u * (1.0f / distance) * pulled * pixels_per_unit;
    size = motion_sensor_blip_subtype_size[subtype];
    pulse = 1.0f;
    if (type == 5) {
        pulse = (float)((sin((double)((float)game_time->game_time * 0.10471973568201065f)) + 1.0) * 0.3333333333333333 + 1.0);
    }
    rasterizer_motion_sensor_blip_draw(point, &motion_sensor_blip_colors[(int8_t)type], alpha, pulse * size_factor + size);
}

#if 0
Original Ghidra decompilation (0x4b37a0):

void FUN_004b37a0(int param_1,char param_2,undefined4 param_3,undefined4 param_4,float param_5)

{
  float fVar1;
  float *in_EAX;
  char unaff_BL;
  float10 fVar2;
  float10 fVar3;

  fVar2 = (float10)fsin(-(float10)*(float *)(param_1 + 0x7c));
  fVar3 = (float10)fcos(-(float10)*(float *)(param_1 + 0x7c));
  fVar1 = (float)((float10)*in_EAX * fVar2 + (float10)in_EAX[1] * fVar3);
  fVar2 = (float10)*in_EAX * fVar3 - (float10)in_EAX[1] * fVar2;
  if ((float10)fVar1 * (float10)fVar1 + fVar2 * fVar2 <
      (float10)*(float *)(DAT_0071941c + 0x2d0) * (float10)*(float *)(DAT_0071941c + 0x2d0)) {
    FUN_006283c0();
    fVar2 = (float10)1.0;
    if (unaff_BL == '\x05') {
      fVar2 = (float10)fsin((float10)*(int *)(DAT_006f1d6c + 0xc) * (float10)0.104719736);
      fVar2 = (fVar2 + (float10)1.0) * (float10)0.3333333333333333 + (float10)1.0;
    }
    FUN_0052bad0(param_4,(float)(fVar2 * (float10)param_5 +
                                (float10)*(float *)(&DAT_00692fd4 + param_2 * 4)));
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
