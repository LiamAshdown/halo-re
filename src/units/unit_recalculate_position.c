// unit_recalculate_position  (Ghidra: FUN_00558eb0)
// address 0x558eb0, size 494 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// REWRITTEN from objdump 0x558eb0..0x55909d (the draft called the nudge and the position setter without their
//   registers: 0x4f7c40 takes EAX object + stack out, 0x4f52c0 takes ESI position + EDI object). EAX: unit.
//   The unit is moved back towards its anchor (+0x1c) from its position (+0x5c): straight onto the anchor when
//   more than 5 away or when the smoothing toggle (0x689471) is off, else onto the midpoint of the position and
//   the nudged anchor (0x4f7c40; the anchor when that fails) provided the midpoint is a valid number within
//   +-5000 on every axis. A resulting jump of more than 2 snaps it onto the anchor after all.
// blam-cc: EAX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t DAT_00689471;    // 0x00689471, the smoothing toggle

extern uint8_t object_nudge_position_by_velocity(uint32_t object_index, real_point3d *out); // 0x4f7c40, EAX, stack
extern void object_set_position_and_recalculate(real_point3d *position, uint32_t object_index); // 0x4f52c0, ESI, EDI
extern double sqrt(double x);      // a single x87 FSQRT
extern int __isnan(double x);      // 0x624494, msvcrt
extern uint8_t real_is_valid(float value); // 0x4476c0

static uint8_t coordinate_in_range(float value)
{
    return (uint8_t)(!(value < -5000.0f) && value <= 5000.0f);
}

void unit_recalculate_position(uint32_t object_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    real_point3d *current = (real_point3d *)(obj + 0x5c);
    real_point3d anchor = *(real_point3d *)(obj + 0x1c);
    real_point3d previous = *current;
    real_point3d *target = &anchor;
    real_point3d midpoint;
    real_point3d nudged;

    // 0x558eeb..0x558f26: the differences stay on the x87 stack (z*z + y*y + x*x)
    if (!(sqrt((anchor.z - previous.z) * (anchor.z - previous.z) + (anchor.y - previous.y) * (anchor.y - previous.y) +
               (anchor.x - previous.x) * (anchor.x - previous.x)) > 5.0) && DAT_00689471) {
        if (!object_nudge_position_by_velocity(object_index, &nudged)) {
            nudged = anchor;
        }
        midpoint.x = (previous.x + nudged.x) * 0.5f;
        midpoint.y = (previous.y + nudged.y) * 0.5f;
        midpoint.z = (previous.z + nudged.z) * 0.5f;
        if (!__isnan((double)midpoint.x) && coordinate_in_range(midpoint.x) &&
            real_is_valid(midpoint.y) && coordinate_in_range(midpoint.y) &&
            real_is_valid(midpoint.z) && coordinate_in_range(midpoint.z)) {
            target = &midpoint;
        }
    }
    object_set_position_and_recalculate(target, object_index);
    if (sqrt((current->z - previous.z) * (current->z - previous.z) + (current->y - previous.y) * (current->y - previous.y) +
             (current->x - previous.x) * (current->x - previous.x)) > 2.0) {
        object_set_position_and_recalculate(&anchor, object_index);
    }
}

#if 0
Original Ghidra decompilation (0x558eb0):

void FUN_00558eb0(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  char cVar11;
  uint in_EAX;
  int iVar12;
  float local_c;
  float local_8;
  float local_4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  fVar2 = *(float *)(iVar1 + 0x1c);
  fVar3 = *(float *)(iVar1 + 0x20);
  fVar4 = *(float *)(iVar1 + 0x5c);
  fVar5 = *(float *)(iVar1 + 0x60);
  fVar6 = *(float *)(iVar1 + 100);
  fVar8 = fVar2 - fVar4;
  fVar7 = *(float *)(iVar1 + 0x24);
  fVar10 = fVar3 - fVar5;
  fVar9 = fVar7 - fVar6;
  if ((SQRT(fVar8 * fVar8 + fVar10 * fVar10 + fVar9 * fVar9) <= 5.0) && (DAT_00689471 != '\0')) {
    cVar11 = FUN_004f7c40();
    if (cVar11 == '\0') {
      local_c = fVar2;
      local_8 = fVar3;
      local_4 = fVar7;
    }
    fVar2 = (fVar4 + local_c) * 0.5;
    fVar3 = (local_8 + fVar5) * 0.5;
    iVar12 = __isnan((double)fVar2);
    if ((((iVar12 == 0) && (-5000.0 <= fVar2)) && (fVar2 < 5000.0 != (fVar2 == 5000.0))) &&
       (((iVar12 = real_is_valid(fVar3), (char)iVar12 != '\0' && (-5000.0 <= fVar3)) &&
        (fVar3 < 5000.0 != (fVar3 == 5000.0))))) {
      real_is_valid((local_4 + fVar6) * 0.5);
    }
  }
  object_set_position_and_recalculate();
  fVar4 = *(float *)(iVar1 + 0x5c) - fVar4;
  fVar5 = *(float *)(iVar1 + 0x60) - fVar5;
  fVar6 = *(float *)(iVar1 + 100) - fVar6;
  if (2.0 < SQRT(fVar4 * fVar4 + fVar5 * fVar5 + fVar6 * fVar6)) {
    object_set_position_and_recalculate();
  }
  return;
}
#endif
