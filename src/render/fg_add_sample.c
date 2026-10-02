// fg_add_sample  (Ghidra: FUN_00512d90; named from out/phase4/render_types_notes.md: "fg_add_sample
// 0x512d90")
// address 0x512d90, size 238 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: types/render.h frame_graph.recent_samples[4] (+0x30a0), maximum (+0x3098), average
//   (+0x309c) and vertices[0x200] (+0x0020, "fg_add_sample scrolls the vertex heights and folds
//   the sample into the running average, and clamps against maximum, and sets the last one
//   (+0x300c)"). The unrolled 7-vertices-per-iteration shift is transcribed as a plain per-vertex
//   loop (same net data movement: vertices[i].y = old vertices[i+1].y for i in [0, 510]).
// register convention: ECX = frame graph index, stack = sample.
//   // blam-cc: ECX -> index, stack -> sample

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern frame_graph frame_graphs[1]; // 0x006b9260

// Pushes a new sample into frame_graphs[index]'s 4 entry recent-sample ring, recomputes the
// running average, scrolls the graph's 0x200 line-strip vertices left by one, and appends the
// new sample (clamped to the graph's maximum) as the rightmost vertex height.
void fg_add_sample(int32_t index, float sample) // blam-cc: ECX -> index, stack -> sample
{
    frame_graph *g = &frame_graphs[index];
    int i;
    float clamped;

    g->recent_samples[0] = g->recent_samples[1];
    g->recent_samples[1] = g->recent_samples[2];
    g->recent_samples[2] = g->recent_samples[3];
    g->recent_samples[3] = sample;

    g->average = (g->recent_samples[0] + g->recent_samples[1] + g->recent_samples[2] +
                  g->recent_samples[3]) * 0.25f;

    clamped = sample;
    if (g->maximum < sample) {
        clamped = g->maximum;
    }

    for (i = 0; i < 0x200 - 1; i++) {
        g->vertices[i].y = g->vertices[i + 1].y;
    }

    g->vertices[0x200 - 1].y = (float)g->bounds.bottom - (clamped / g->maximum) * 120.0f;
}

#if 0
Original Ghidra decompilation (0x512d90):

void FUN_00512d90(float param_1)

{
  float fVar1;
  undefined4 *puVar2;
  int in_ECX;
  int iVar3;
  int iVar4;

  iVar3 = in_ECX * 0x32b0;
  *(undefined4 *)(&DAT_006bc300 + in_ECX * 0x32b0) =
       *(undefined4 *)(&DAT_006bc304 + in_ECX * 0x32b0);
  fVar1 = *(float *)(&DAT_006bc30c + in_ECX * 0x32b0);
  *(undefined4 *)(&DAT_006bc304 + in_ECX * 0x32b0) =
       *(undefined4 *)(&DAT_006bc308 + in_ECX * 0x32b0);
  *(float *)(&DAT_006bc308 + in_ECX * 0x32b0) = fVar1;
  *(float *)(&DAT_006bc30c + in_ECX * 0x32b0) = param_1;
  *(float *)(&DAT_006bc2fc + iVar3) =
       (*(float *)(&DAT_006bc300 + in_ECX * 0x32b0) + *(float *)(&DAT_006bc304 + in_ECX * 0x32b0) +
        fVar1 + param_1) * 0.25;
  if (*(float *)(&DAT_006bc2f8 + iVar3) < param_1) {
    param_1 = *(float *)(&DAT_006bc2f8 + iVar3);
  }
  puVar2 = (undefined4 *)(&DAT_006b929c + iVar3);
  fVar1 = *(float *)(&DAT_006bc2f8 + iVar3);
  iVar4 = 0x49;
  do {
    puVar2[-6] = *puVar2;
    *puVar2 = puVar2[6];
    puVar2[6] = puVar2[0xc];
    puVar2[0xc] = puVar2[0x12];
    puVar2[0x12] = puVar2[0x18];
    puVar2[0x18] = puVar2[0x1e];
    puVar2[0x1e] = puVar2[0x24];
    puVar2 = puVar2 + 0x2a;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  *(float *)(&DAT_006bc26c + iVar3) =
       (float)(int)*(short *)(&DAT_006b9264 + iVar3) - (param_1 / fVar1) * 120.0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
