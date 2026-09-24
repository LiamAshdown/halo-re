// object_nudge_position_by_velocity  (Ghidra: FUN_004f7c40; renamed, Blam-style, not
// previously named)
// address 0x4f7c40, size 270 bytes
// name confidence: 0.25 (matches functions.md's summary: "Nudges a position by a
//   velocity-scaled amount over time, gated on several object state flags")
// rewrite confidence: 0.25 (three gating byte flags and the three-float position this reads
//   fall inside object's unresolved 0x022..0x05b range per types/objects.h, and time_query_performance_counter_ms /
//   FUN_006391b4 are foreign, unexamined callees; preserved as raw offsets, not renamed fields)
// evidence: types/objects.h object (unknown_018 0x018, unknown_022[0x3a] 0x022, velocity
//   0x068); global 0x008603b0 object_data; callee point3d_add_scaled (0x401930, established
//   math helper).
// register convention: object index in EAX. Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f7c4c and eax,0xffff at entry, no stack access before that.
//   // blam-cc: EAX -> object_index
// UNSURE: time_query_performance_counter_ms (foreign, called with obj+0x58 in EDI) and FUN_006391b4 (foreign,
//   returns what this call site treats as an unsigned 32-bit tick count converted to float) are
//   both declared with the plain argument lists Ghidra shows, which may be incomplete.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

extern void time_query_performance_counter_ms(uint8_t *field_0x58); // 0x449210, foreign module, UNSURE
extern int32_t __ftol(); // 0x006391b4, MSVC 7.1 CRT x87 float-to-int truncation
    // (verified by disassembling 0x006391b4: fld st(0) / fst [esp+0x18] / fistp qword /
    // fild qword ... , the classic _ftol2 body). The value arrives on the x87 stack, so
    // some call sites show a visible float argument and others show none; the empty
    // parameter list asserts no prototype, the same convention this module already uses
    // for FUN_00450870.
extern double sqrt(double x); // x87 FSQRT, as in src/objects/antenna_update_physics.c
extern void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, float scale); // 0x401930, established elsewhere as EAX -> out, ECX -> direction, stack -> (base, scale)

uint8_t object_nudge_position_by_velocity(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *raw = (uint8_t *)obj;

    if ((raw[0x18] == 1) && (raw[0x44] == 1) && (raw[0x54] == 1)) {
        uint32_t ticks;
        float speed;

        time_query_performance_counter_ms(raw + 0x58);
        ticks = __ftol();

        speed = (float)sqrt(obj->velocity.i * obj->velocity.i + obj->velocity.j * obj->velocity.j +
                             obj->velocity.k * obj->velocity.k);
        if ((ticks != 0) && (speed > 0.05f)) {
            // UNSURE: base/out is a snapshot at raw+0x1c (an undocumented, non-position field
            // per types/objects.h's unresolved 0x022..0x05b range), not obj->position itself;
            // preserved exactly rather than guessed into a named field. out and direction were
            // not fully hand-traced (see file header); out==base and direction==velocity are
            // this rewrite's best-effort guess at the two register arguments.
            real_point3d point = *(real_point3d *)(raw + 0x1c);
            float ticks_f = (float)ticks; // Ghidra's manual unsigned-to-float widen is implicit here
            point3d_add_scaled(&point, &obj->velocity, &point, ticks_f * 0.001f * 30.0f * speed);
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4f7c40):

undefined4 FUN_004f7c40(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  uint in_EAX;
  int iVar4;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (((*(char *)(iVar1 + 0x18) == '\x01') && (*(char *)(iVar1 + 0x44) == '\x01')) &&
     (*(char *)(iVar1 + 0x54) == '\x01')) {
    FUN_00449210();
    iVar4 = FUN_006391b4();
    if ((iVar4 != 0) &&
       (fVar2 = SQRT(*(float *)(iVar1 + 0x68) * *(float *)(iVar1 + 0x68) +
                     *(float *)(iVar1 + 0x6c) * *(float *)(iVar1 + 0x6c) +
                     *(float *)(iVar1 + 0x70) * *(float *)(iVar1 + 0x70)), 0.05 < fVar2)) {
      local_c = *(undefined4 *)(iVar1 + 0x1c);
      local_8 = *(undefined4 *)(iVar1 + 0x20);
      local_4 = *(undefined4 *)(iVar1 + 0x24);
      fVar3 = (float)iVar4;
      if (iVar4 < 0) {
        fVar3 = fVar3 + 4.2949673e+09;
      }
      point3d_add_scaled(&local_c,fVar3 * 0.001 * 30.0 * fVar2);
      return 1;
    }
  }
  return 0;
}
#endif
