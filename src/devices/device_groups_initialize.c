// device_groups_initialize  (Ghidra: device_groups_initialize, already named; functions.md:
// "Allocates and initializes the runtime device-group value table from the scenario's device
// group definitions at level load")
// address 0x44c220, size 111 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/tags.h Scenario (device_groups: count 0x288, pointer 0x28c, stride 0x34),
// ScenarioDeviceGroup (initial_value 0x20, flags 0x24); types/devices.h device_group
// (flags/value), device_group_flags (_device_group_can_change_only_once_bit).
// register convention: none; takes no parameters (Ghidra's own `device_groups_initialize(void)`).
// Re-confirmed by a second disassembly pass in the phase-4 review: 0x44c249
// `mov cl,BYTE PTR [eax+esi*1+0x24]` then `test cl,0x1` / `mov esi,0x1` is the flags bit, and
// 0x44c25b `fld DWORD PTR [eax+0x20]` loads initial_value BEFORE `call 0x4d0480`, with
// `fstp [eax+0x4]` on success and a bare `fstp st(0)` discard at 0x44c27b on failure. The group
// index is scaled unsigned (`movzx eax,ax`). The loop counter is EBP re-narrowed through
// `movsx eax,bp` each iteration, so it is really an int16 against the int32 count -- reproduced
// as int32_t here, which differs only for a scenario with 32768 or more device groups.
// Resolved against disassembly (objdump -d -M intel bin/halo.exe, 0x44c220-0x44c28e): Ghidra's
// `extraout_ST0`/`extraout_EDX` are the scenario group's initial_value (loaded with `fld` right
// before the datum_new call, which does not touch the FPU stack) and the device_groups pointer
// itself (loaded into EDX once before the loop and never reassigned, so it survives the call
// untouched) -- not a real 64-bit datum_new return, matching the same misreading already
// documented in src/devices/device_new.c's header.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "devices.h"
#include "fn_memory.h"
#include "fn_devices.h"

extern Scenario *global_scenario; // 0x00746f8c, the Scenario tag data; same spelling as the
    // six declarations in src/hs
extern data_array *device_groups; // 0x0087abf0


void device_groups_initialize(void)
{
    Scenario *scenario = global_scenario;
    ScenarioDeviceGroup *scenario_groups = (ScenarioDeviceGroup *)scenario->device_groups.pointer;
    int32_t i;

    for (i = 0; i < (int32_t)scenario->device_groups.count; i++) {
        ScenarioDeviceGroup *scenario_group = &scenario_groups[i];
        datum_index new_group = datum_new(device_groups);

        if ((uint16_t)new_group != 0xffff) {
            device_group *group = &((device_group *)device_groups->data)[(uint16_t)new_group];
            group->flags = (scenario_group->flags & 0x1) != 0
                ? (1u << _device_group_can_change_only_once_bit) : 0;
            group->value = scenario_group->initial_value;
        }
    }
}

#if 0
Original Ghidra decompilation (0x44c220), from tools/pack.py 0x44c220:

void device_groups_initialize(void)

{
  byte bVar1;
  int iVar2;
  ushort uVar3;
  int iVar4;
  int extraout_EDX;
  short sVar5;
  float10 extraout_ST0;

  iVar2 = global_scenario;
  sVar5 = 0;
  if (0 < *(int *)(global_scenario + 0x288)) {
    iVar4 = 0;
    do {
      bVar1 = *(byte *)(iVar4 * 0x34 + 0x24 + *(int *)(iVar2 + 0x28c));
      uVar3 = datum_new();
      if (uVar3 != 0xffff) {
        iVar4 = *(int *)(extraout_EDX + 0x34) + (uint)uVar3 * 8;
        *(ushort *)(iVar4 + 2) = (ushort)((bVar1 & 1) != 0);
        *(float *)(iVar4 + 4) = (float)extraout_ST0;
      }
      sVar5 = sVar5 + 1;
      iVar4 = (int)sVar5;
    } while (iVar4 < *(int *)(iVar2 + 0x288));
  }
  return;
}
#endif
