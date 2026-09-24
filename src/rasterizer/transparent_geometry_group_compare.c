// transparent_geometry_group_compare  (Ghidra: transparent_geometry_group_compare, already
// named)
// address 0x5155b0, size 287 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: a qsort-style comparator over transparent_geometry_group_sorted_indices; every
//   offset used (shader +0xc, shader_type +0x24, flags bit7 +0x00, depth +0x78, sort_key +0x08,
//   first_person +0xa5) matches transparent_geometry_group/Shader exactly (types/rasterizer.h,
//   types/tags.h). Orders: shader types 5/6/7 (with the derived shader's +0x29 bit4 set) or 8
//   sort first; first_person groups sort last; otherwise back-to-front by depth then by
//   sort_key, honoring _group_sort_first_bit.
// register convention: none -- __cdecl(short *a, short *b), the two recognized parameters are
//   pointers to sorted-index slots (int16_t*), matching qsort's element-pointer convention.
// UNSURE: the Shader+0x29 byte, same caveat as transparent_geometry_group_draw_all.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern transparent_geometry_group *transparent_geometry_groups; // 0x0071d14c

static int32_t is_batched_shader(Shader *shader)
{
    if (shader == (Shader *)0) {
        return 0;
    }
    return (shader->shader_type == 5 || shader->shader_type == 6 || shader->shader_type == 7) &&
           ((((uint8_t *)shader)[0x29] >> 4 & 1) != 0);
}

// __cdecl(short *a, short *b)
// qsort comparator: batched-shader groups (5/6/7 with the derived flag) always sort before b /
// after a; a type-8 shader on either side also sorts first; otherwise back-to-front by depth,
// then ascending sort_key, honoring _group_sort_first_bit; first_person groups always sort last
// (checked once at the very end, matching the original's single shared exit).
int __cdecl transparent_geometry_group_compare(int16_t *a, int16_t *b)
{
    transparent_geometry_group *ga = &transparent_geometry_groups[*a];
    transparent_geometry_group *gb = &transparent_geometry_groups[*b];
    Shader *shader_a = (Shader *)ga->shader;
    Shader *shader_b = (Shader *)gb->shader;
    int result;

    if (is_batched_shader(shader_a)) {
        result = -1;
        goto first_person_check;
    }
    if (is_batched_shader(shader_b)) {
        result = 1;
        goto first_person_check;
    }

    if (shader_a == (Shader *)0 || shader_a->shader_type != 8) {
        if (shader_b != (Shader *)0 && shader_b->shader_type == 8) {
            result = 1;
            goto first_person_check;
        }
        if ((ga->flags & 0x80) == 0) {
            if ((gb->flags & 0x80) != 0) {
                result = -1;
                goto first_person_check;
            }
        } else {
            if ((gb->flags & 0x80) == 0) {
                result = 1;
                goto first_person_check;
            }
        }
        if (gb->depth < ga->depth) {
            result = 1;
            goto first_person_check;
        }
        if (gb->depth <= ga->depth) {
            if (gb->sort_key < ga->sort_key) {
                result = 1;
                goto first_person_check;
            }
            if (gb->sort_key <= ga->sort_key) {
                result = 0;
                goto first_person_check;
            }
        }
    }
    result = -1;

first_person_check:
    if (ga->first_person == 0) {
        if (gb->first_person == 0) {
            return result;
        }
    } else if (gb->first_person == 0) {
        return 1;
    }
    if (ga->first_person != 0) {
        return result;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x5155b0):

int __cdecl transparent_geometry_group_compare(short *param_1,short *param_2)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  uint *puVar4;
  byte *pbVar5;
  int iVar6;

  puVar4 = (uint *)(*param_1 * 0xa8 + DAT_0071d14c);
  uVar2 = puVar4[3];
  iVar6 = 0;
  pbVar5 = (byte *)(*param_2 * 0xa8 + DAT_0071d14c);
  if ((uVar2 != 0) &&
     ((((sVar1 = *(short *)(uVar2 + 0x24), sVar1 == 5 || (sVar1 == 6)) || (sVar1 == 7)) &&
      ((*(byte *)(uVar2 + 0x29) >> 4 & 1) != 0)))) {
    iVar6 = -1;
    goto LAB_005156a1;
  }
  iVar3 = *(int *)(pbVar5 + 0xc);
  if ((iVar3 != 0) &&
     ((((sVar1 = *(short *)(iVar3 + 0x24), sVar1 == 5 || (sVar1 == 6)) || (sVar1 == 7)) &&
      ((*(byte *)(iVar3 + 0x29) >> 4 & 1) != 0)))) {
    iVar6 = 1;
    goto LAB_005156a1;
  }
  if ((uVar2 == 0) || (*(short *)(uVar2 + 0x24) != 8)) {
    if ((iVar3 != 0) && (*(short *)(iVar3 + 0x24) == 8)) {
      iVar6 = 1;
      goto LAB_005156a1;
    }
    if ((*puVar4 & 0x80) == 0) {
      if ((*pbVar5 & 0x80) != 0) goto LAB_00515665;
    }
    else {
      if ((*pbVar5 & 0x80) == 0) {
        iVar6 = 1;
        goto LAB_005156a1;
      }
LAB_00515665:
      if ((*puVar4 & 0x80) == 0) goto LAB_0051569d;
    }
    if (*(float *)(pbVar5 + 0x78) < (float)puVar4[0x1e]) {
      iVar6 = 1;
      goto LAB_005156a1;
    }
    if (*(float *)(pbVar5 + 0x78) <= (float)puVar4[0x1e]) {
      if (*(int *)(pbVar5 + 8) < (int)puVar4[2]) {
        iVar6 = 1;
        goto LAB_005156a1;
      }
      if (*(int *)(pbVar5 + 8) <= (int)puVar4[2]) goto LAB_005156a1;
    }
  }
LAB_0051569d:
  iVar6 = -1;
LAB_005156a1:
  if (*(char *)((int)puVar4 + 0xa5) == '\0') {
    if (pbVar5[0xa5] == 0) {
      return iVar6;
    }
  }
  else if (pbVar5[0xa5] == 0) {
    return 1;
  }
  if (*(char *)((int)puVar4 + 0xa5) != '\0') {
    return iVar6;
  }
  return -1;
}
#endif
