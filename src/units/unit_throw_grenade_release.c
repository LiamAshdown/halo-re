// unit_throw_grenade_release  (NOT A FUNCTION: an address inside vehicle_blend_animations)
// address 0x571b40, size 309 bytes
// VERIFIED against disassembly 0x571b40..0x571c6f (2026-09-30): the range starts mid-instruction stream (0x571b40 decodes
//   as junk, `add bh,bl / loopne`, because it is the tail of a longer instruction at 0x571b3x) and uses EBX/EBP/ESI/EDI, ECX
//   and stack slots [esp+0x18..0x20] set up by vehicle_blend_animations (0x5718e0..0x571c6f, `ret` at 0x571c6f). It samples
//   the animation frame for the seat blend and every wheel/contact marker (0x4d53f0), weighting each by
//   vehicle_data.contact_point_traction[i] (0xff = 1.0, else * 1/255). Nothing calls or jumps to 0x571b40. It is
//   implemented in vehicle_blend_animations.c; this file has no body of its own and must never be hooked.
//   The name unit_throw_grenade_release is wrong (the grenade release is unit_release_thrown_grenade, 0x56e440).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

// Not callable: see the header. Kept only so the address stays listed in the symbol tables.
void unit_throw_grenade_release(void)
{
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
