// point3d_project_onto_line  (Ghidra: point3d_project_onto_line, already named)
// address 0x5066e0, size 121 bytes
// name confidence: 0.50   rewrite confidence: 0.85
// evidence: out/phase4/physics_types_notes.md "0x5066e0 point3d_project_onto_line - a pure math
//   helper (project a point onto an infinite line). Belongs in the math module; already named,
//   no types of its own."; the module's one caller, src/physics/physics_model_slide_along_contacts.c,
//   already carries a matching extern declaration and blam-cc line for this address, cross-checked
//   here against `objdump -d -M intel --start-address=0x5066e0 --stop-address=0x50675e bin/halo.exe`.
//   cleanup pass 4 orphan pass: physics judged this address out of place for physics (see
//   out/phase4/orphans_notes.md) and it was picked up here.
// register convention: EAX (direction), ECX (line_origin) and EDX (out_result) are Ghidra's
//   recognized hidden registers; the point to project is the sole stack argument. Confirmed by
//   disassembly: `mov esi,[esp+0x8]` reads the point through the incoming stack slot, `fsub [ecx]`
//   subtracts line_origin, the two `fmul [eax+n]` runs against the direction vector build the dot
//   products, and the three trailing `fstp [edx+n]` stores write the result through EDX.
//   // blam-cc: EAX -> direction, ECX -> line_origin, EDX -> out_result, stack -> point

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Projects *point onto the infinite line through *line_origin in direction *direction, and
// writes the projected point to *out_result. out_result may alias point.
void point3d_project_onto_line(real_point3d *point, real_vector3d *direction,
    real_point3d *line_origin, real_point3d *out_result)
{
    real_vector3d delta;
    real t;

    delta.i = point->x - line_origin->x;
    delta.j = point->y - line_origin->y;
    delta.k = point->z - line_origin->z;

    t = (delta.i * direction->i + delta.j * direction->j + delta.k * direction->k) /
        (direction->i * direction->i + direction->j * direction->j + direction->k * direction->k);

    out_result->x = t * direction->i + line_origin->x;
    out_result->y = t * direction->j + line_origin->y;
    out_result->z = t * direction->k + line_origin->z;
}

#if 0
Original Ghidra decompilation (0x5066e0):

void point3d_project_onto_line(float *param_1)

{
  float fVar1;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;

  fVar1 = ((*param_1 - *in_ECX) * *in_EAX +
          (param_1[1] - in_ECX[1]) * in_EAX[1] + (param_1[2] - in_ECX[2]) * in_EAX[2]) /
          (in_EAX[2] * in_EAX[2] + in_EAX[1] * in_EAX[1] + *in_EAX * *in_EAX);
  *in_EDX = fVar1 * *in_EAX + *in_ECX;
  in_EDX[1] = fVar1 * in_EAX[1] + in_ECX[1];
  in_EDX[2] = fVar1 * in_EAX[2] + in_ECX[2];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
