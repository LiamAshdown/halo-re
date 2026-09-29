// actor_grenade_avoidance_entry_init  (Ghidra: actor_grenade_avoidance_entry_init; named for this rewrite)
// address 0x42af50, size 98 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: its only caller, actor_gather_nearby_grenade_targets @0x42afc0, fills an
// out-array of these records ("started a grenade-avoidance timer for each" per its own
// phase-4 summary), 0x28 bytes apart. Field layout re-derived from the real disassembly
// (objdump -d -M intel --start-address=0x42af50 --stop-address=0x42afb2 bin/halo.exe)
// because Ghidra's own decompile lost two of this function's inputs as "unaff_ESI" /
// "unaff_EDI" and a callee output as "unaff_EBX", none of which it shows being set.
// register convention: ESI -> entry (the output record slot, set up by the caller as
// array_base + count*sizeof(*entry)), EDI -> object_index (also read at the call sites,
// e.g. actor.unit_index or prop.object_index -- the object the crouch-offset lookup runs
// against), stack -> prop_index (the datum the caller wants recorded once the timer fires).
// blam-cc: ESI -> entry, EDI -> object_index, stack -> prop_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height,
    float *pill_radius_out); // 0x55a2e0, EAX position out, ECX object, stack height, EBX radius
    // 0x0055a2e0, not yet rewritten (units module). blam-cc: out_offset on the stack,
    // out_time in EBX, object_index in ECX. Computes a unit's crouch-interpolated vertical
    // offset (out_offset) from its object position; out_time's exact meaning is not
    // established here -- only that this function adds a fixed 0.15s window to it.

// blam-cc: ESI -> entry, EDI -> object_index, stack -> prop_index
// Fills one slot of the caller's "nearby grenade target" array: runs the crouch-offset
// lookup against object_index, notes whether it came back exactly zero, and stamps the
// entry with a 0.15-second-from-now avoidance deadline plus the two identifying handles.
void actor_grenade_avoidance_entry_init(ai_grenade_avoidance_entry *entry,
                                         datum_index object_index, datum_index prop_index)
{
    float offset;
    float deadline;

    // 0x42af54: EAX = &entry->target_position (written by the callee), ECX = object, [esp] = &offset, EBX = &deadline
    unit_get_crouch_height_offset(&entry->target_position, object_index, &offset, &deadline);
    entry->already_clear = (offset == 0.0f);
    entry->unknown_10 = 0;
    entry->unknown_14 = 0;
    entry->crouch_offset = offset;
    entry->avoid_until = deadline + 0.15f;
    entry->prop_index = prop_index;
    entry->object_index = object_index;
}

#if 0
Original Ghidra decompilation (0x42af50):

void FUN_0042af50(undefined4 param_1)

{
  int unaff_ESI;
  undefined4 unaff_EDI;
  float local_8;
  float local_4;

  FUN_0055a2e0(&local_8);
  *(bool *)unaff_ESI = local_8 == 0.0;
  *(undefined4 *)(unaff_ESI + 0x10) = 0;
  *(undefined4 *)(unaff_ESI + 0x14) = 0;
  *(float *)(unaff_ESI + 0x18) = local_8;
  *(float *)(unaff_ESI + 0x24) = local_4 + 0.15;
  *(undefined4 *)(unaff_ESI + 0x1c) = param_1;
  *(undefined4 *)(unaff_ESI + 0x20) = unaff_EDI;
  return;
}

Real disassembly (0x42af50-0x42afb1), used in place of the above because it lost two
parameters and a callee-output register entirely:

0042af50: sub    esp,0x8
0042af53: push   ebx
0042af54: lea    eax,[esp+0x4]
0042af58: push   eax
0042af59: lea    eax,[esi+0x4]
0042af5c: lea    ebx,[esp+0xc]
0042af60: mov    ecx,edi
0042af62: call   0x55a2e0
0042af67: fld    dword ptr ds:0x672ac0        ; 0.0
0042af6d: fld    dword ptr [esp+0x8]
0042af71: add    esp,0x4
0042af74: fucompp
0042af76: xor    ecx,ecx
0042af78: pop    ebx
0042af79: fnstsw ax
0042af7b: test   ah,0x44
0042af7e: jp     0x42af87
0042af80: mov    eax,0x1
0042af85: jmp    0x42af89
0042af87: xor    eax,eax
0042af89: fld    dword ptr [esp+0x4]
0042af8d: mov    edx,dword ptr [esp+0xc]
0042af91: fadd   dword ptr ds:0x672dc0        ; 0.15
0042af97: mov    byte ptr [esi],al
0042af99: mov    dword ptr [esi+0x10],ecx
0042af9c: mov    dword ptr [esi+0x14],ecx
0042af9f: mov    ecx,dword ptr [esp]
0042afa2: mov    dword ptr [esi+0x18],ecx
0042afa5: fstp   dword ptr [esi+0x24]
0042afa8: mov    dword ptr [esi+0x1c],edx
0042afab: mov    dword ptr [esi+0x20],edi
0042afae: add    esp,0x8
0042afb1: ret
#endif
