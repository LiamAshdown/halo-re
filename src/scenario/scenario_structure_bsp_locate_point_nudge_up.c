// scenario_structure_bsp_locate_point_nudge_up  (Ghidra: FUN_0053e870, still unnamed there;
// renamed here -- out/phase2/results/scenario_00.json's guess "scenario_slot_allocate_with_aging"
// does not fit the disassembly, which nudges a *point* upward and re-probes the collision bsp,
// not any slot table)
// address 0x53e870, size 71 bytes
// name confidence: 0.3   rewrite confidence: 0.75
// evidence: out/phase4/scenario_types_notes.md: "EDX = real_point3d *point. It moves point.z up
// 0.05 on each failed probe, 150 times at most. It returns true when the first probe already hit
// a leaf." Raw disassembly confirms ECX = global_collision_bsp and EAX = 0 (root node) are set up
// immediately before each bsp3d_node_find_leaf call, with EDX left untouched throughout (the
// point pointer this function itself received); k_scenario_location_nudge_attempts (150) and the
// 0.05f step (0x00672be8) are the module's own named constants for this exact loop.
//   UNSURE: its only caller is unit_detach_reposition_and_nudge (0x56cb23), so the point is a
//   unit being repositioned after a detach; the name stays generic after what the code
//   demonstrably does: probing a point against the resident collision bsp and nudging it
//   upward until it lands inside geometry.
// register convention: EDX -> point (real_point3d *), no stack parameters. Returns a bool: true
// iff the very first probe (before any nudging) already found a leaf.
//   // blam-cc: EDX -> point; return in AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "scenario.h"

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, physics module

extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90

// blam-cc: EDX -> point
// Probes `point` against the resident collision bsp. While the probe misses (finds no leaf),
// nudges point->z upward by 0.05 and retries, up to k_scenario_location_nudge_attempts (150)
// times, mutating *point in place with every nudge that is applied. Returns true only when the
// very first probe, before any nudging, already found a leaf.
uint8_t scenario_structure_bsp_locate_point_nudge_up(real_point3d *point)
{
    int16_t attempts;
    uint32_t leaf;

    leaf = bsp3d_node_find_leaf(0, global_collision_bsp, point);
    attempts = 0;
    while (leaf == 0xffffffff && attempts < k_scenario_location_nudge_attempts) {
        attempts++;
        point->z = point->z + 0.05f;
        leaf = bsp3d_node_find_leaf(0, global_collision_bsp, point);
    }
    return attempts == 0;
}

#if 0
Original Ghidra decompilation (0x53e870):

bool FUN_0053e870(void)

{
  int iVar1;
  short sVar2;
  short sVar3;
  undefined8 uVar4;

  uVar4 = FUN_005013a0();
  sVar2 = 0;
  while ((iVar1 = (int)((ulonglong)uVar4 >> 0x20), sVar3 = sVar2, (int)uVar4 == -1 &&
         (sVar3 = sVar2 + 1, sVar2 < 0x96))) {
    *(float *)(iVar1 + 8) = *(float *)(iVar1 + 8) + 0.05;
    uVar4 = FUN_005013a0();
    sVar2 = sVar3;
  }
  return sVar3 == 0;
}

Raw disassembly (0x53e870-0x53e8b6):

  53e870: push   esi
  53e871: push   edi
  53e872: mov    edi,DWORD PTR ds:0x746f90     ; edi = global_collision_bsp
  53e878: xor    eax,eax                       ; eax = 0 (root node)
  53e87a: mov    ecx,edi
  53e87c: xor    esi,esi                       ; esi = attempts = 0
  53e87e: call   0x5013a0                      ; bsp3d_node_find_leaf(0, bsp, point [edx, unaff])
  53e883: cmp    eax,0xffffffff
  53e886: jne    0x53e8ac                      ; leaf found: done, esi still 0
  53e888: mov    ax,si                         ; ax = attempts (before increment)
  53e88b: inc    esi
  53e88c: cmp    ax,0x96                       ; 150
  53e890: jge    0x53e8ac                      ; give up
  53e892: fld    DWORD PTR [edx+0x8]           ; point->z
  53e895: xor    eax,eax
  53e897: fadd   DWORD PTR ds:0x672be8         ; + 0.05
  53e89d: mov    ecx,edi
  53e89f: fstp   DWORD PTR [edx+0x8]           ; point->z = ...
  53e8a2: call   0x5013a0                      ; retry
  53e8a7: cmp    eax,0xffffffff
  53e8aa: je     0x53e888
  53e8ac: xor    eax,eax
  53e8ae: test   si,si
  53e8b1: pop    edi
  53e8b2: sete   al                            ; al = (attempts == 0)
  53e8b5: pop    esi
  53e8b6: ret
#endif
