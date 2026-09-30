// transparent_geometry_group_draw_all  (Ghidra: transparent_geometry_group_draw_all, already
// named)
// address 0x5154a0, size 251 bytes
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: walks transparent_geometry_group_sorted_indices from
//   transparent_geometry_group_draw_cursor (0x006d9838), stopping early (when resorting) at the
//   first group whose shader is not one of the batched types (5, 6 or 7 with its derived shader
//   flag bit 4 set, or 8 -- matches transparent_geometry_group_compare's same test) and drawing
//   the rest through rasterizer_transparent_geometry_group_draw; applies the frustum z bias once
//   for the first group with _group_sort_first_bit set (types/rasterizer.h).
// register convention: resort flag in the recognized stack parameter.
// UNSURE: the Shader+0x29 byte is one past the base Shader struct (types/tags.h, size 0x28) --
//   the high byte of "the first flags word of the derived shader" per the type header's own
//   note; kept as a raw offset rather than a named field since the derived shader isn't typed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern int32_t transparent_geometry_group_count;           // 0x0071d154
extern int16_t transparent_geometry_group_draw_cursor;     // 0x006d9838
extern int32_t transparent_geometry_group_last_drawn_key;  // 0x006e1d58
extern uint8_t rasterizer_secondary_groups_drawn; // 0x0071d274
extern int16_t *transparent_geometry_group_sorted_indices; // 0x0071d15c
extern transparent_geometry_group *transparent_geometry_groups; // 0x0071d14c
extern uint32_t rasterizer_frustum_z_values[2]; // 0x0069c664

extern void transparent_geometry_group_sort(void); // 0x5156d0
// blam-cc: stack -> (z_near, z_far) as raw float bits

// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode);       // 0x519200


// blam-cc: stack -> resort
// Draws every active transparent-geometry group in depth-sorted order starting from the shared
// draw cursor, re-sorting first (and resetting the cursor) when `resort` is set.
void transparent_geometry_group_draw_all(uint8_t resort)
{
    uint8_t applied_frustum_z = 0;
    int32_t cursor;

    if (transparent_geometry_group_count <= 0) {
        return;
    }

    if (resort != 0) {
        transparent_geometry_group_sort();
        transparent_geometry_group_draw_cursor = 0;
    }

    cursor = transparent_geometry_group_draw_cursor;
    transparent_geometry_group_last_drawn_key = 0;
    rasterizer_secondary_groups_drawn = 0;

    if (cursor >= transparent_geometry_group_count) {
        return;
    }

    do {
        transparent_geometry_group *group =
            &transparent_geometry_groups[transparent_geometry_group_sorted_indices[cursor]];

        if (resort != 0) {
            Shader *shader = (Shader *)group->shader;
            if (shader == (Shader *)0 ||
                (shader->shader_type != 8 &&
                 ((shader->shader_type != 5 && shader->shader_type != 6 && shader->shader_type != 7) ||
                  ((((uint8_t *)shader)[0x29] >> 4 & 1) == 0)))) {
                break;
            }
        }

        if ((int8_t)group->flags < 0 && applied_frustum_z == 0) {
            rasterizer_set_shader_stage_config(0);
            chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
            applied_frustum_z = 1;
        }

        rasterizer_transparent_geometry_group_draw(group, 0);
        transparent_geometry_group_draw_cursor = transparent_geometry_group_draw_cursor + 1;
        cursor = transparent_geometry_group_draw_cursor;
    } while (cursor < transparent_geometry_group_count);

    if (applied_frustum_z != 0) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }
}

#if 0
Original Ghidra decompilation (0x5154a0):

void transparent_geometry_group_draw_all(char param_1)

{
  short sVar1;
  bool bVar2;
  int iVar3;
  char *pcVar4;

  if (0 < DAT_0071d154) {
    bVar2 = false;
    if (param_1 != '\0') {
      transparent_geometry_group_sort();
      DAT_006d9838 = 0;
    }
    iVar3 = (int)DAT_006d9838;
    DAT_006e1d58 = 0;
    DAT_0071d274 = 0;
    if (iVar3 < DAT_0071d154) {
      do {
        pcVar4 = (char *)(*(short *)(DAT_0071d15c + iVar3 * 2) * 0xa8 + DAT_0071d14c);
        if ((param_1 != '\0') &&
           ((iVar3 = *(int *)(pcVar4 + 0xc), iVar3 == 0 ||
            ((*(short *)(iVar3 + 0x24) != 8 &&
             ((((sVar1 = *(short *)(iVar3 + 0x24), sVar1 != 5 && (sVar1 != 6)) && (sVar1 != 7)) ||
              ((*(byte *)(iVar3 + 0x29) >> 4 & 1) == 0)))))))) break;
        if ((*pcVar4 < '\0') && (!bVar2)) {
          rasterizer_set_shader_stage_config();
          chimera__rasterizer_set_frustum_z_func(DAT_0069c664,DAT_0069c668);
          bVar2 = true;
        }
        rasterizer_transparent_geometry_group_draw(pcVar4,0);
        DAT_006d9838 = DAT_006d9838 + 1;
        iVar3 = (int)DAT_006d9838;
      } while (iVar3 < DAT_0071d154);
      if (bVar2) {
        chimera__rasterizer_set_frustum_z_func(0,0);
      }
    }
  }
  return;
}
#endif
