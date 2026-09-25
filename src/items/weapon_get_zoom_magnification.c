// weapon_get_zoom_magnification  (Ghidra: FUN_004c2d70; renamed per items_types_notes.md:
// "Weapon.zoom_levels 0x3da and zoom_magnification_range 0x3dc/0x3e0")
// address 0x4c2d70, size 219 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: types/tags.h Weapon.zoom_levels (0x3da), zoom_magnification_range (0x3dc/0x3e0).
// register convention: item index in EAX; zoom level in DX.
// blam-cc: EAX -> item_index, DX -> zoom_level
// UNSURE: FUN_006283c0 is the MSVC7.1 CRT `_CIpow` FPU intrinsic
// (src/objects/curve_apply_exponent.c: base in ST(1), exponent in ST(0), no real stack
// parameters). Ghidra shows three "arguments" at this call site, which is very likely an
// artifact of that FPU-register calling convention rather than a genuine 3-argument call.
// Rendered literally as a 3-argument opaque call rather than guessed apart into a 2-argument
// pow(); do not treat the resulting magnification value as verified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern double pow(double x, double y); // the original calls _CIpow (0x6283c0): st(1) ** st(0) on the x87 stack

// Interpolates a weapon's zoom magnification for one zoom level across its tag-defined range.
real weapon_get_zoom_magnification(datum_index item_index, int16_t zoom_level)
{
    object *item_obj;
    Weapon *weapon_tag;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (zoom_level >= 0 && zoom_level < weapon_tag->zoom_levels) {
        real fraction;
        real range0;
        real range1;

        if (weapon_tag->zoom_levels < 2) {
            fraction = 0.0f;
        } else {
            fraction = (real)zoom_level / (real)(weapon_tag->zoom_levels - 1);
        }
        range0 = (weapon_tag->zoom_magnification_range[0] <= 0.0f) ? 1.0f : weapon_tag->zoom_magnification_range[0];
        range1 = (weapon_tag->zoom_magnification_range[1] <= 0.0f) ? 1.0f : weapon_tag->zoom_magnification_range[1];

        // 0x4c2e34: fld range1 / fdiv range0 / fld fraction / call _CIpow / fmul range0
        return (real)(pow((double)range1 / range0, fraction) * range0);
    }
    return 1.0f;
}

#if 0
Original Ghidra decompilation (0x4c2d70):

float10 FUN_004c2d70(void)

{
  short sVar1;
  int iVar2;
  uint in_EAX;
  short in_DX;
  float10 fVar3;
  float fVar4;
  undefined4 uVar5;
  float fVar6;

  fVar3 = (float10)1.0;
  iVar2 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((-1 < in_DX) && (sVar1 = *(short *)(iVar2 + 0x3da), in_DX < sVar1)) {
    if (sVar1 < 2) {
      fVar6 = 0.0;
    }
    else {
      fVar6 = (float)(int)in_DX / (float)(sVar1 + -1);
    }
    if (*(float *)(iVar2 + 0x3dc) <= 0.0) {
      fVar4 = 1.0;
    }
    else {
      fVar4 = *(float *)(iVar2 + 0x3dc);
    }
    if (*(float *)(iVar2 + 0x3e0) <= 0.0) {
      uVar5 = 0x3f800000;
    }
    else {
      uVar5 = *(undefined4 *)(iVar2 + 0x3e0);
    }
    fVar3 = (float10)FUN_006283c0(fVar4,uVar5,fVar6);
    fVar3 = fVar3 * (float10)fVar4;
  }
  return fVar3;
}
#endif
