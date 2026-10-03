// model_nodes_get_default_transforms  (Ghidra: model_nodes_get_default_transforms, already
// named)
// address 0x4d7610, size 114 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/models_functions.md summary ("copies each node's default (unanimated)
//   rotation and translation into an SQT transform array with scale fixed at 1.0"); matches
//   types/models.h real_orientation section field-for-field (rotation from ModelNode +0x34,
//   translation from +0x28, scale 1.0f).
// FIXED (difftest): TagReflexive.count is uint32_t, so the loop compared unsigned; the original compares signed and
//   skips a negative count.
// register convention: model in ESI (unaff_ESI); output real_orientation array as the
//   recognized stack parameter.
//   // blam-cc: ESI -> model, stack -> out

#include "tags.h"
#include "math.h"
#include "models.h"

// Fills one real_orientation per node with that node's unanimated default pose (scale always
// 1.0), i.e. the base pose before any animation is applied.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void model_nodes_get_default_transforms(GBXModel *model, real_orientation *out)
{
    ModelNode *nodes;
    int16_t node;

    nodes = (ModelNode *)model->nodes.pointer;
    for (node = 0; (int32_t)node < (int32_t)model->nodes.count; node++) { // movsx dx vs the count, SIGNED (jle 0x4d761a, jl 0x4d767c)
        out[node].rotation = *(real_quaternion *)&nodes[node].default_rotation;
        out[node].translation = *(real_point3d *)&nodes[node].default_translation;
        out[node].scale = 1.0f;
    }
}

#if 0
Original Ghidra decompilation (0x4d7610):

void model_nodes_get_default_transforms(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  short sVar5;
  int unaff_ESI;

  sVar5 = 0;
  if (0 < *(int *)(unaff_ESI + 0xb8)) {
    iVar3 = 0;
    do {
      iVar2 = iVar3 * 0x9c + *(int *)(unaff_ESI + 0xbc);
      puVar4 = (undefined4 *)(iVar3 * 0x20 + param_1);
      *puVar4 = *(undefined4 *)(iVar2 + 0x34);
      puVar4[1] = *(undefined4 *)(iVar2 + 0x38);
      puVar4[2] = *(undefined4 *)(iVar2 + 0x3c);
      puVar4[3] = *(undefined4 *)(iVar2 + 0x40);
      puVar4[4] = *(undefined4 *)(iVar2 + 0x28);
      puVar4[5] = *(undefined4 *)(iVar2 + 0x2c);
      uVar1 = *(undefined4 *)(iVar2 + 0x30);
      sVar5 = sVar5 + 1;
      puVar4[7] = 0x3f800000;
      iVar3 = (int)sVar5;
      puVar4[6] = uVar1;
    } while (iVar3 < *(int *)(unaff_ESI + 0xb8));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
