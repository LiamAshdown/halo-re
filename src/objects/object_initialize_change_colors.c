// object_initialize_change_colors  (Ghidra: FUN_004f8b70, split by Ghidra into three pieces: 0x4f8bd0 (the loop body,
//   entered by 0x4f8bc4 jmp) and 0x4f8cb0 (inside the clamps) were separate "functions" object_set_position_network
//   and object_set_position_network_clone_4f8cb0, both removed; formerly object_apply_network_placement)
// address 0x4f8b70, size 521 bytes (0x4f8b70..0x4f8d78)
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: only caller object_new_with_datum_role_control (0x4f5801) passes EAX = the new object and the
//   placement's four colors (+0x58). objdump 0x4f8b70..0x4f8d78: for each of the four change colors i, the
//   object's working color (+0x188 + 0xc*i) starts as the placement color. When i is below the Object tag's
//   change_colors count (+0x164, blocks of 0x2c at +0x168), a weight fmod(|z*744.12415 + x*315.89313 + y*587.12946 +
//   i*431.12894|, 1) from the object's position (+0x5c) picks the first permutation (+0x20 count, 0x1c each at
//   +0x24) whose weight is at least it, and color_interpolate(EAX = its +0x10, ECX = its +0x4, stack: the working
//   color, 1, fmod(|y| + i*0.71211, 1)) fills the working color. The working color clamped to 0..1 (a NaN kept)
//   becomes change_colors[i] (+0x1b8 + 0xc*i). fmod is the CRT _CIfmod (x in ST(1), y in ST(0)).
//   The earlier split drafts called fmod with no arguments (a trap on the first campaign object).
// blam-cc: EAX -> object_index, stack -> colors (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t);
    // 0x43f6a0, blam-cc: EAX -> color1, ECX -> color0, stack -> dest, flags, t
extern double fmod(double x, double y); // 0x628cca, MSVC CRT _CIfmod
extern double fabs(double x);           // x87 fabs

static float clamp_unit(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

void object_initialize_change_colors(uint32_t object_index, ColorRGB *colors) // blam-cc: EAX -> object_index
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    float *position = (float *)(obj + 0x5c);
    int32_t i;

    for (i = 0; i < 4; i++) {
        ColorRGB *working = (ColorRGB *)(obj + 0x188 + i * 0xc);
        ColorRGB *final_color = (ColorRGB *)(obj + 0x1b8 + i * 0xc);

        *working = colors[i];
        if (i < *(int32_t *)&((Object *)tag)->change_colors.count) {
            uint8_t *change_color = *(uint8_t **)&((Object *)tag)->change_colors.pointer + i * 0x2c;
            // x87: each product is a float operand times a float constant (0x672e80, 0x672e7c, 0x672e78, 0x672e74)
            double seed = (double)position[2] * (double)744.12415f + (double)position[0] * (double)315.89313f +
                (double)position[1] * (double)587.12946f + (double)i * (double)431.12894f;
            float weight = (float)fmod(fabs(seed), 1.0);
            int32_t count = *(int32_t *)(change_color + 0x20);
            int16_t p;

            for (p = 0; p < count; p++) {
                uint8_t *permutation = *(uint8_t **)(change_color + 0x24) + p * 0x1c;

                if (weight <= *(float *)permutation) {
                    float t = (float)fmod(fabs(position[1]) + (double)i * (double)0.71210998f, 1.0); // 0x672e70

                    color_interpolate((ColorRGB *)(permutation + 0x10), (ColorRGB *)(permutation + 4), working, 1, t);
                    break;
                }
            }
        }
        final_color->red = clamp_unit(working->red);
        final_color->green = clamp_unit(working->green);
        final_color->blue = clamp_unit(working->blue);
    }
}

#if 0
Original Ghidra decompilation (0x4f8b70):

void FUN_004f8b70(void)

{
  object_set_position_network();
  return;
}
#endif
