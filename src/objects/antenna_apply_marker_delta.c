// antenna_apply_marker_delta
// address 0x4fb1c0, size 378 bytes
// name confidence: 0.55 (still FUN_004fb1c0 in Ghidra; functions.md: "Shifts a widget's stored
//   point cloud (antenna chain or flag rope) by the movement of its anchoring marker since the
//   last update"; this instance only ever indexes with the antenna_vertex stride (0x20), so it
//   is the antenna-specific half of that description, not a shared widget routine)
// rewrite confidence: 0.7 (raised from 0.6 by the phase-4 review pass: the leaf probe now passes the marker position the disassembly shows in EDX instead of an uninitialised local) (resolved from disassembly, see below)
// evidence: types/objects.h antenna (object_index 0x0c, previous_marker_position 0x10,
//   vertices 0x1c stride 0x20), antenna_vertex (position 0x00); types/tags.h Antenna
//   (attachment_marker_name 0x00 doubles as the marker-name string, vertices TagReflexive
//   0xc4); object_get_node_local_transform 0x4f6080 (cdecl, all four args on the stack: object
//   index, marker name, object_marker*, flags); math.h real_matrix4x3 (node_transform.forward
//   at marker+0x3c, .position at marker+0x60).
// register convention: resolved by disassembling 0x4fb1c0 directly (objdump -d -M intel
//   bin/halo.exe, 0x4fb1c0..0x4fb339), because Ghidra's decompilation showed three
//   caller-supplied values (in_EAX, unaff_EBX, unaff_ESI) with no visible definition:
//     - at entry, ECX is loaded from [ESI+0xc] (the caller's antenna->object_index) BEFORE the
//       prologue even runs, and EDI is set to the incoming EAX; EAX and EBX are never reloaded
//       afterward, so they are true caller-supplied register inputs.
//     - antenna_tag and the node-reference output are pushed on the stack (verified against
//       object_get_node_local_transform's own entry, which is `mov eax,[esp+4]` first -- a
//       pure-stack function; the earlier claim in object_get_node_local_transform.c that it
//       takes EAX/ECX/EDX register arguments is contradicted by this disassembly and is not
//       corrected here, out of this file's scope).
//   EAX -> out_forward, EBX -> out_position, ESI -> ant; stack -> antenna_tag, node_ref.
// UNSURE: the "position" and "forward" buffers this function writes (EBX and EAX) are the
//   SAME two output vectors antenna_update_physics reads back for its own vertex-0 seed; their
//   identity as node_transform.position / node_transform.forward is inferred from
//   object_marker's known layout at the matching stack offsets, not from a field name Ghidra
//   itself attached to them.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90, passed to FUN_005013a0 in ECX
extern uint8_t *global_structure_bsp; // 0x00746f9c; +0xe4 is the per-node lookup table
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080, all four on the stack
extern int32_t bsp3d_node_find_leaf(void *globals, real_point3d *point, int32_t index);
    // 0x5013a0; globals in ECX, point in EDX, index in EAX (established by
    // object_light_recompute_transform.c's identical call site)
extern int32_t __ftol(); // 0x006391b4, MSVC 7.1 CRT x87 float-to-int truncation
    // (verified by disassembling 0x006391b4: fld st(0) / fst [esp+0x18] / fistp qword /
    // fild qword ... , the classic _ftol2 body). The value arrives on the x87 stack, so
    // some call sites show a visible float argument and others show none; the empty
    // parameter list asserts no prototype, the same convention this module already uses
    // for FUN_00450870.

void antenna_apply_marker_delta(real_vector3d *out_forward /*EAX*/, real_point3d *out_position /*EBX*/,
                                 antenna *ant /*ESI*/, Antenna *antenna_tag, bsp_leaf_reference *node_ref)
    // blam-cc: EAX -> out_forward, EBX -> out_position, ESI -> ant, stack -> antenna_tag, node_ref
{
    object_marker marker;

    object_get_node_local_transform(ant->object_index, (char *)antenna_tag, &marker, 1);
    *out_position = marker.node_transform.position;
    *out_forward = marker.node_transform.forward;

    {
        // Resolved from the disassembly at 0x4fb21b: EDX is `lea edx,[esp+0x78]`, which is
        // marker+0x60 -- the node_transform position this function has just copied out. EAX is
        // zeroed and ECX is the 0x00746f90 globals pointer.
        int32_t node_index = bsp3d_node_find_leaf(global_collision_bsp, &marker.node_transform.position, 0);

        node_ref->leaf_index = node_index;
        if (node_index == -1) {
            node_ref->cluster_index = -1;
        } else {
            node_ref->cluster_index = *(int16_t *)(*(uint8_t **)(global_structure_bsp + 0xe4) +
                                                 (uint32_t)(node_index & 0x7fffffff) * 0x10 + 8);
        }
    }

    {
        real_vector3d delta;
        delta.i = out_position->x - ant->previous_marker_position.x;
        delta.j = out_position->y - ant->previous_marker_position.y;
        delta.k = out_position->z - ant->previous_marker_position.z;

        // Ghidra: three nested ifs, each testing abs(__ftol(axis)) <= 1.0 and only checking the
        // next axis when the current one passes; all three passing skips the vertex re-base
        // below. Preserved as the same short-circuiting nest rather than a single && expression.
        int32_t tx = __ftol((double)delta.i);
        int skip = (tx < 0 ? -tx : tx) <= 1;
        if (skip) {
            int32_t ty = __ftol((double)delta.j);
            skip = (ty < 0 ? -ty : ty) <= 1;
            if (skip) {
                int32_t tz = __ftol((double)delta.k);
                skip = (tz < 0 ? -tz : tz) <= 1;
            }
        }

        if (!skip) {
            int32_t vertex_count = antenna_tag->vertices.count;
            int32_t i;

            for (i = 0; i <= vertex_count; i++) {
                ant->vertices[i].position.x += delta.i;
                ant->vertices[i].position.y += delta.j;
                ant->vertices[i].position.z += delta.k;
            }
        }
    }

    ant->previous_marker_position = *out_position;
}

#if 0
Original Ghidra decompilation (0x4fb1c0):

void FUN_004fb1c0(int param_1,int *param_2)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  undefined2 uVar8;
  undefined4 *in_EAX;
  int iVar9;
  uint uVar10;
  short sVar11;
  float *unaff_EBX;
  int unaff_ESI;
  undefined1 local_6c [60];
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  float local_c;
  float local_8;
  float local_4;

  FUN_004f6080(*(undefined4 *)(unaff_ESI + 0xc),param_1,local_6c,1);
  *unaff_EBX = local_c;
  unaff_EBX[1] = local_8;
  unaff_EBX[2] = local_4;
  *in_EAX = local_30;
  in_EAX[1] = local_2c;
  in_EAX[2] = local_28;
  iVar9 = FUN_005013a0();
  *param_2 = iVar9;
  if (iVar9 == -1) {
    uVar8 = 0xffff;
  }
  else {
    uVar8 = *(undefined2 *)(iVar9 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  }
  *(undefined2 *)(param_2 + 1) = uVar8;
  fVar2 = *unaff_EBX;
  fVar3 = *(float *)(unaff_ESI + 0x10);
  fVar4 = unaff_EBX[1];
  fVar5 = *(float *)(unaff_ESI + 0x14);
  fVar6 = unaff_EBX[2];
  fVar7 = *(float *)(unaff_ESI + 0x18);
  uVar10 = FUN_006391b4();
  if ((float)(int)((uVar10 ^ (int)uVar10 >> 0x1f) - ((int)uVar10 >> 0x1f)) <= 1.0) {
    uVar10 = FUN_006391b4();
    if ((float)(int)((uVar10 ^ (int)uVar10 >> 0x1f) - ((int)uVar10 >> 0x1f)) <= 1.0) {
      uVar10 = FUN_006391b4();
      if ((float)(int)((uVar10 ^ (int)uVar10 >> 0x1f) - ((int)uVar10 >> 0x1f)) <= 1.0)
      goto LAB_004fb324;
    }
  }
  if (*(int *)(param_1 + 0xc4) != -1 && -1 < *(int *)(param_1 + 0xc4) + 1) {
    iVar9 = 0;
    sVar11 = 0;
    do {
      pfVar1 = (float *)(iVar9 * 0x20 + 0x1c + unaff_ESI);
      sVar11 = sVar11 + 1;
      *pfVar1 = (fVar2 - fVar3) + *(float *)(iVar9 * 0x20 + 0x1c + unaff_ESI);
      pfVar1[1] = (fVar4 - fVar5) + pfVar1[1];
      pfVar1[2] = (fVar6 - fVar7) + pfVar1[2];
      iVar9 = (int)sVar11;
    } while (iVar9 < *(int *)(param_1 + 0xc4) + 1);
  }
LAB_004fb324:
  *(float *)(unaff_ESI + 0x10) = *unaff_EBX;
  *(float *)(unaff_ESI + 0x14) = unaff_EBX[1];
  *(float *)(unaff_ESI + 0x18) = unaff_EBX[2];
  return;
}

Disassembly (0x4fb1c0..0x4fb339), resolved by objdump -d -M intel bin/halo.exe:

004fb1c0:  mov ecx,[esi+0xc]        ; ecx = ant->object_index (ESI already holds ant at entry)
004fb1c3:  sub esp,0x7c
004fb1c6:  push ebp
004fb1c7:  mov ebp,[esp+0x84]       ; ebp = antenna_tag (first stack arg)
004fb1ce:  push edi
004fb1cf:  mov edi,eax              ; edi = incoming EAX (out_forward)
004fb1d1:  push 0x1
004fb1d3:  lea eax,[esp+0x1c]       ; eax = &local object_marker buffer
004fb1d7:  push eax
004fb1d8:  push ebp                 ; antenna_tag doubles as the marker-name char* (offset 0)
004fb1d9:  push ecx                 ; object_index
004fb1da:  call 0x4f6080            ; object_get_node_local_transform(object_index, tag, &marker, 1)
004fb1df..004fb218:  copy marker.node_transform.position (marker+0x60) to *unaff_EBX,
                      marker.node_transform.forward (marker+0x3c) to *edi (the saved EAX)
004fb20c:  mov ecx,ds:0x746f90      ; global_globals
004fb21b:  lea edx,[esp+0x78]
004fb21f:  xor eax,eax
004fb221:  call 0x5013a0            ; FUN_005013a0(ecx=globals, edx=&scratch_point, eax=0)
004fb226..004fb251:  node_ref->node_index = result; node_ref->unknown_04 = -1 or the
                      structure_bsp_globals+0xe4 table lookup
004fb255..004fb271:  delta = *unaff_EBX - ant->previous_marker_position (three fsub)
004fb279,004fb29c,004fb2bf:  call 0x6391b4 (__ftol) on each delta component, abs it, compare <= 1.0;
                      all three true -> jmp 0x4fb324 (skip the vertex loop)
004fb2de..004fb322:  for (i = 0; i <= antenna_tag->vertices.count; i++)
                        ant->vertices[i].position += delta;   (stride 0x20, base ant+0x1c)
004fb324..004fb333:  ant->previous_marker_position = *unaff_EBX
#endif
