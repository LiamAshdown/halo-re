// unit_throw_grenade_release  (Ghidra: already named unit_throw_grenade_release -- per
//   out/phase4/units_types_notes.md this name is WRONG: the real grenade-release function is
// name confidence: 0.1   rewrite confidence: 1.0 (FRAGMENT: 0x571b40 is inside vehicle_blend_animations 0x5718e0..0x571c6f, whose C covers this code; nothing calls or jumps here)
// address 0x571b40, size 309 bytes
// name confidence: 0.4 (pre-existing name kept per the task's renaming rule -- it already
//   carries a name, even though the notes file says it is misleading)
// rewrite confidence: 0.1 -- zero recorded callers and total register loss (unaff_EBX/EBP/ESI/
//   EDI, in_ECX, and even a stack-passed in_stack argument all arrive with no traceable origin)
//   mark this as a shared tail Ghidra split out of a larger vehicle physics function, most
//   likely one of the wheel/hover/thruster calculators in this same batch. The leading
//   byte-rotate loop over unaff_EBX is very likely decompiler noise from a register the real
//   function never treated as a byte counter at all, and is not reproduced.
// evidence: types/units.h vehicle_data.contact_point_traction (0x4f4, "0x575170 reads and
//   rewrites entry i, 0xff meaning full traction" -- the same *(byte*)(i+0x4f4+base) idiom
//   appears here); types/tags.h Vehicle.wheel_circumference (0x310, matches the
//   out/phase4/units_types_notes.md note that this function "reads Vehicle.wheel_circumference
//   (tag 0x310) and vehicle_data 0x4f4").
// register convention: UNRESOLVED.
//   // blam-cc: UNSURE -- see header
// UNSURE: everything below. This file accumulates weighted per-marker force contributions via
//   animation_overlay_interpolated_frame_orientations, but which markers, which vehicle, and what the
//   accumulated result feeds could not be determined from this decompile.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern void animation_overlay_interpolated_frame_orientations(float frame, void *out); // 0x4d53f0, UNSURE signature

// Accumulates weighted per-marker force contributions used when releasing a thrown object (e.g.
// a grenade) from the unit.
// UNSURE: reproduced only partially; see the file header. Ghidra's own register-loss makes a
// faithful full rewrite impossible without first identifying this function's real caller.
void unit_throw_grenade_release(vehicle_data *vehicle, void *marker_out, uint8_t *object_base,
                                 Vehicle *tag, void *physics_contact_points, int32_t contact_count)
{
    int32_t i;

    (void)vehicle;
    (void)marker_out;
    (void)tag;

    for (i = 0; i < contact_count; i++) {
        int16_t marker_index = *(int16_t *)((uint8_t *)physics_contact_points + i * 0x14 + 2);
        if (marker_index != -1) {
            uint8_t traction = object_base[0x4f4 + i]; // vehicle_data.contact_point_traction[i]
            float weight = (traction == 0xff) ? 1.0f : (float)traction * 0.003921569f;
            (void)weight; // UNSURE: the frame computed from it needs the animation graph base,
                          //   which is not recoverable here; see file header
        }
    }
}

#if 0
Original Ghidra decompilation (0x571b40):

void unit_throw_grenade_release(void)

{
  byte bVar1;
  short sVar2;
  float fVar3;
  short sVar4;
  int iVar5;
  int in_ECX;
  char cVar6;
  int unaff_EBX;
  int unaff_EBP;
  int unaff_ESI;
  int unaff_EDI;
  float10 in_ST1;
  int iStack00000010;
  int in_stack_00000018;
  undefined4 in_stack_0000001c;

  do {
    cVar6 = (char)((uint)unaff_EBX >> 8) + (char)unaff_EBX;
    unaff_EBX = CONCAT22((short)((uint)unaff_EBX >> 0x10),CONCAT11(cVar6,(char)unaff_EBX));
    in_ECX = in_ECX + -1;
  } while (in_ECX != 0 && cVar6 != '\0');
  iStack00000010 = *(short *)(unaff_EDI + 0x22) + -1;
  model_vertices_get_interpolated_frame((float)((float10)iStack00000010 * in_ST1),in_stack_0000001c)
  ;
  if ((5 < *(int *)(unaff_EBX + 0x5c)) &&
     (sVar4 = *(short *)(*(int *)(unaff_EBX + 0x60) + 10), sVar4 != -1)) {
    if (*(float *)(in_stack_00000018 + 0x310) <= 0.0) {
      fVar3 = 0.0;
    }
    else {
      fVar3 = *(float *)(unaff_ESI + 0x4e0) / *(float *)(in_stack_00000018 + 0x310);
    }
    model_vertices_get_interpolated_frame
              ((float)(int)*(short *)(sVar4 * 0xb4 + *(int *)(unaff_EBP + 0x78) + 0x22) * fVar3,
               in_stack_0000001c);
  }
  iVar5 = 0;
  sVar4 = 0;
  if (0 < *(int *)(unaff_EBX + 0x68)) {
    do {
      sVar2 = *(short *)(*(int *)(unaff_EBX + 0x6c) + iVar5 * 0x14 + 2);
      if (sVar2 != -1) {
        bVar1 = *(byte *)(iVar5 + 0x4f4 + unaff_ESI);
        if (bVar1 == 0xff) {
          fVar3 = 1.0;
        }
        else {
          fVar3 = (float)bVar1 * 0.003921569;
        }
        iStack00000010 = *(short *)(sVar2 * 0xb4 + *(int *)(unaff_EBP + 0x78) + 0x22) + -1;
        model_vertices_get_interpolated_frame((float)iStack00000010 * fVar3,in_stack_0000001c);
      }
      sVar4 = sVar4 + 1;
      iVar5 = (int)sVar4;
    } while (iVar5 < *(int *)(unaff_EBX + 0x68));
  }
  return;
}
#endif
