// object_update_change_colors  (Ghidra: object_update_change_colors, already named)
// address 0x4f9110, size 480 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Recomputes an object's blended 'change color' values for each
//   region from its model's change-color definitions")
// rewrite confidence: 0.5
// evidence: types/objects.h object (change_colors 0x1b8, function_in_values/out_values region
//   0x120+4*selector per the object struct comment); types/tags.h Object.scales_change_colors,
//   Object.change_colors (TagReflexive), ObjectChangeColors (darken_by 0x00, scale_by 0x02,
//   flags 0x04); global 0x008603b0 object_data, 0x0087bc14 tag_instances; callee
//   color_interpolate (0x43f6a0, established).
// register convention: object index in EAX. Confirmed against objdump-equivalent pattern used
//   throughout this module for single-register accessors; consistent with this function's own
//   Ghidra signature carrying only "in_EAX".
//   // blam-cc: EAX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t);
    // 0x43f6a0, blam-cc: EAX -> color1, ECX -> color0, stack -> dest, flags, t

void object_update_change_colors(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    if ((definition->scales_change_colors & 1) != 0) {
        int32_t count = definition->change_colors.count;
        int32_t i;

        for (i = 0; i < count; i++) {
            ObjectChangeColors *tag_color = (ObjectChangeColors *)((uint8_t *)definition->change_colors.pointer + i * 0x2c);
            ColorRGB *out = &obj->change_colors[i];
            int c;

            if (tag_color->scale_by != 0) {
                float t = *(float *)((uint8_t *)obj + 0x120 + tag_color->scale_by * 4);
                // 0x4f9193..0x4f91b1: EAX = the tag entry's +0x14 color, ECX = its +0x8 color, stack: the object's
                // change color, the entry's +0x4 flags, t
                color_interpolate((ColorRGB *)((uint8_t *)tag_color + 0x14), (ColorRGB *)((uint8_t *)tag_color + 8), out,
                    *(uint32_t *)((uint8_t *)tag_color + 4), t);
            }
            if (tag_color->darken_by != 0) {
                float scale = *(float *)((uint8_t *)obj + 0x120 + tag_color->darken_by * 4);
                out->red *= scale;
                out->green *= scale;
                out->blue *= scale;
            }

            for (c = 0; c < 3; c++) {
                float *component = &out->red + c;
                if (*component < 0.0f) {
                    *component = 0.0f;
                } else if (*component > 1.0f) {
                    *component = 1.0f;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f9110):

void object_update_change_colors(void)

{
  float *pfVar1;
  int iVar2;
  uint uVar3;
  float fVar4;
  short sVar5;
  uint *puVar6;
  int iVar7;
  short sVar8;
  uint in_EAX;
  int iVar9;
  short *psVar10;

  puVar6 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar7 = *(int *)((*puVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((*(byte *)(iVar7 + 0x24) & 1) != 0) {
    iVar9 = 0;
    sVar8 = 0;
    if (0 < *(int *)(iVar7 + 0x164)) {
      do {
        sVar5 = *(short *)(iVar9 * 0x2c + 2 + *(int *)(iVar7 + 0x168));
        psVar10 = (short *)(iVar9 * 0x2c + *(int *)(iVar7 + 0x168));
        if (sVar5 != 0) {
          if (sVar5 < 5) {
            uVar3 = puVar6[sVar5 + 0x48];
          }
          else {
            uVar3 = puVar6[sVar5 + 0x48];
          }
          color_interpolate(puVar6 + iVar9 * 3 + 0x6e,*(undefined4 *)(psVar10 + 2),uVar3);
        }
        sVar5 = *psVar10;
        if (sVar5 != 0) {
          if (sVar5 < 5) {
            fVar4 = (float)puVar6[sVar5 + 0x48];
          }
          else {
            fVar4 = (float)puVar6[sVar5 + 0x48];
          }
          puVar6[iVar9 * 3 + 0x6e] = (uint)(fVar4 * (float)puVar6[iVar9 * 3 + 0x6e]);
          puVar6[iVar9 * 3 + 0x6f] = (uint)(fVar4 * (float)puVar6[iVar9 * 3 + 0x6f]);
          puVar6[iVar9 * 3 + 0x70] = (uint)(fVar4 * (float)puVar6[iVar9 * 3 + 0x70]);
        }
        if (0.0 <= (float)puVar6[iVar9 * 3 + 0x6e]) {
          if ((float)puVar6[iVar9 * 3 + 0x6e] <= 1.0) {
            uVar3 = puVar6[iVar9 * 3 + 0x6e];
          }
          else {
            uVar3 = 0x3f800000;
          }
        }
        else {
          uVar3 = 0;
        }
        iVar2 = iVar9 * 3 + 0x6f;
        puVar6[iVar9 * 3 + 0x6e] = uVar3;
        pfVar1 = (float *)(puVar6 + iVar2);
        if (0.0 <= (float)puVar6[iVar2]) {
          if (*pfVar1 <= 1.0) {
            fVar4 = *pfVar1;
          }
          else {
            fVar4 = 1.0;
          }
        }
        else {
          fVar4 = 0.0;
        }
        *pfVar1 = fVar4;
        if (0.0 <= (float)puVar6[iVar9 * 3 + 0x70]) {
          if ((float)puVar6[iVar9 * 3 + 0x70] <= 1.0) {
            uVar3 = puVar6[iVar9 * 3 + 0x70];
          }
          else {
            uVar3 = 0x3f800000;
          }
        }
        else {
          uVar3 = 0;
        }
        puVar6[iVar9 * 3 + 0x70] = uVar3;
        sVar8 = sVar8 + 1;
        iVar9 = (int)sVar8;
      } while (iVar9 < *(int *)(iVar7 + 0x164));
    }
  }
  return;
}
#endif
