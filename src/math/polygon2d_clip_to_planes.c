// polygon2d_clip_to_planes  (Ghidra: polygon2d_clip_to_planes, already named)
// address 0x4caee0, size 261 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/math_types_notes.md item 8 ("double-buffers through 0x800 dwords of
//   __chkstk stack, two halves of 0x400 dwords = 512 real_point2d each") and
//   out/phase4/math_functions.md ("Iteratively clips a 2D polygon against a sequence of
//   planes/edges, double-buffering the vertex list between clips").
// register convention: vertex count in ECX, vertex array in EDX, and five stack parameters.
//   The result comes back in AX only (mov ax,bx at 0x4caf07 / 0x4cafda).
//   // blam-cc: ECX -> vertex_count, EDX -> vertices,
//   //          stack -> (clip_point_count, clip_points, maximum_count, out, epsilon)
//
// VERIFIED against the disassembly at 0x4caee0 (objdump -d -M intel). Two earlier readings were
// wrong, and the second one changes the function's whole interface:
//
//   1. The second stack parameter is NOT an array of real_plane2d. It is an array of
//      real_point2d -- the vertices of the CLIP POLYGON -- and each clip line is built on the
//      fly from a consecutive pair of them, wrapping at the start:
//        0x4caf25  test di,di / lea edx,[edi-1] / jne / lea edx,[eax-1]
//                    previous = (i == 0) ? clip_point_count - 1 : i - 1
//        0x4caf53  mov edx,[esp+0x2028] / lea eax,[edx+prev*8] / lea edx,[edx+i*8]
//        0x4caf60  lea ecx,[esp+0x14] / call 0x44d950
//      i.e. plane2d_from_points(&edge, &clip_points[previous], &clip_points[i]). The stride is
//      8, which is real_point2d, not the 12 a real_plane2d would need. That also explains the
//      12-byte local Ghidra shows as `local_200c` and cannot find an assignment for: it is the
//      edge plane the callee writes, sitting immediately below the 0x2000-byte ping-pong
//      scratch at [esp+0x20]. An earlier reading filled it with `planes[i]`, which would read
//      the wrong 12 of every 8 bytes.
//
//   2. FUN_0044d950 is not an opaque "should this plane clip" predicate. It is
//      plane2d_from_points (see polygon2d_points_classify's header for its full body); it
//      returns NULL exactly when the two adjacent clip points are closer together than 0.0001,
//      i.e. when the edge is degenerate. That is the case in which this function copies the
//      vertex list through unchanged (rep movs of vertex_count*8 bytes at 0x4cafb5).
//
// Buffer routing (0x4caf30): the last edge writes straight into the caller's `out`; every other
// edge writes into scratch half (i & 1), and the next iteration reads from whatever was just
// written (mov esi,ebp at 0x4cafcb). polygon2d_clip_to_plane takes its destination in EDX and
// everything else on the stack, so the call is
//   polygon2d_clip_to_plane(EDX = dest, vertex_count, vertices, &edge, maximum_count, 0, 0, epsilon)
// with the edge-bitmask and clipped-flag out-parameters both passed as NULL (push 0 / push 0 at
// 0x4caf7c).

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// 0x44d950, out of this module and still unnamed in symbols/functions.txt; ECX = out plane,
// EAX = first point, EDX = second point. Returns out_plane, or NULL if the points coincide.


// Clips a 2D polygon against every edge of a second (clip) polygon in turn, double-buffering
// the vertex list between clips. Returns the final vertex count, or -1 if any single clip
// overflowed `maximum_count`.
int16_t polygon2d_clip_to_planes(int16_t vertex_count, real_point2d *vertices,
                                  int16_t clip_point_count, real_point2d *clip_points,
                                  int16_t maximum_count, real_point2d *out, real epsilon)
{
    real_plane2d edge;             // [esp+0x14], written by plane2d_from_points
    real_point2d scratch[2][512];  // [esp+0x20] and [esp+0x1020], 0x1000 bytes each
    int16_t i;
    int16_t previous;
    real_point2d *dest;
    int16_t j;

    if (clip_point_count <= 0) {
        return vertex_count;
    }

    i = 0;
    while (0 < vertex_count) {
        previous = (i == 0) ? (int16_t)(clip_point_count - 1) : (int16_t)(i - 1);

        dest = out;
        if ((int32_t)i != (int32_t)clip_point_count - 1) {
            dest = scratch[i & 1];
        }

        if (plane2d_from_points(&edge, &clip_points[previous], &clip_points[i]) == 0) {
            // Degenerate edge (the two clip points coincide): pass the polygon straight through.
            for (j = 0; j < vertex_count; j++) {
                dest[j] = vertices[j];
            }
        } else {
            vertex_count = polygon2d_clip_to_plane(dest, vertex_count, vertices, &edge,
                                                    maximum_count, 0, 0, epsilon);
            if (vertex_count == -1) {
                return -1;
            }
        }

        i = i + 1;
        vertices = dest;
        if (clip_point_count <= i) {
            return vertex_count;
        }
    }
    return vertex_count;
}

#if 0
Original Ghidra decompilation (0x4caee0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint polygon2d_clip_to_planes
               (short param_1,undefined4 param_2,undefined4 param_3,undefined4 *param_4,
               undefined4 param_5)

{
  int iVar1;
  uint in_ECX;
  undefined4 *in_EDX;
  undefined4 *puVar2;
  short sVar3;
  undefined4 *puVar4;
  undefined1 local_200c [12];
  undefined4 auStack_2000 [2047];
  undefined4 uStack_4;

  uStack_4 = 0x4caeea;
  sVar3 = 0;
  if (0 < param_1) {
    while (0 < (short)in_ECX) {
      puVar2 = param_4;
      if ((int)sVar3 != (int)param_1 - 1U) {
        puVar2 = auStack_2000 + ((int)sVar3 & 1U) * 0x400;
      }
      iVar1 = FUN_0044d950();
      if (iVar1 == 0) {
        puVar4 = puVar2;
        for (iVar1 = ((int)(short)in_ECX & 0x1fffffffU) << 1; iVar1 != 0; iVar1 = iVar1 + -1) {
          *puVar4 = *in_EDX;
          in_EDX = in_EDX + 1;
          puVar4 = puVar4 + 1;
        }
        for (iVar1 = 0; iVar1 != 0; iVar1 = iVar1 + -1) {
          *(undefined1 *)puVar4 = *(undefined1 *)in_EDX;
          in_EDX = (undefined4 *)((int)in_EDX + 1);
          puVar4 = (undefined4 *)((int)puVar4 + 1);
        }
      }
      else {
        in_ECX = polygon2d_clip_to_plane(in_ECX,in_EDX,local_200c,param_3,0,0,param_5);
        if ((short)in_ECX == -1) {
          return in_ECX;
        }
      }
      sVar3 = sVar3 + 1;
      in_EDX = puVar2;
      if (param_1 <= sVar3) {
        return in_ECX & 0xffff;
      }
    }
  }
  return in_ECX & 0xffff;
}
#endif
