// vector3d_project_onto_direction  (no Ghidra function; only reached from the velocity switch of
// physics_model_slide_along_contacts at 0x506d1e)
// address 0x506760, size 73 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: objdump -d 0x506760..0x5067a8: out = axis * (v . axis) / (axis . axis).
// register convention: out in ECX, axis in EAX, v in EDX, no stack arguments.
//   // blam-cc: ECX -> out, EAX -> axis, EDX -> v

#include "tags.h"
#include "math.h"

// blam-cc: ECX -> out, EAX -> axis, EDX -> v
// Projects v onto the (not necessarily unit) axis.
void vector3d_project_onto_direction(real_vector3d *out, const real_vector3d *axis, const real_vector3d *v)
{
    float scale = (v->k * axis->k + v->j * axis->j + v->i * axis->i) /
                  (axis->i * axis->i + axis->j * axis->j + axis->k * axis->k);

    out->i = scale * axis->i;
    out->j = scale * axis->j;
    out->k = scale * axis->k;
}

#if 0
No Ghidra function at 0x506760 (it sits in the gap after point3d_project_onto_line).
  fld [eax+8]; fld [eax+4]; fld [eax]                     ; axis z y x
  v . axis via EDX; axis . axis; fdivp; out = axis * scale through ECX; ret
#endif
