// sound_eax1_effect_shutdown  (Ghidra: FUN_0054ec40; earlier draft name
//   sound_eax1_effect_shutdown)
// address 0x54ec40, size 25 bytes
// name confidence: 0.7   rewrite confidence: 0.95
// evidence: slot 0 (shutdown) of the EAX1 sound_effect_object_vtable at 0x00671d4c, whose nine
//   slots are { 0x54ec40, 0x54ec60, 0x551240, 0x54ef10, 0x551250, 0x551260, 0x54edf0, 0x54ed50,
//   0x54ee30 } (read from .rdata in the phase-4 review); releases the IKsPropertySet held at
//   sound_effect_object.property_set (0x18) and clears it.
// register convention: ECX -> this_object (__thiscall, no stack arguments).

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: ECX -> this_object
void sound_eax1_effect_shutdown(sound_effect_object *this_object)
{
    void *property_set = this_object->property_set;

    if (property_set != 0) {
        ((uint32_t (__stdcall *)(void *))(*(void ***)property_set)[2])(property_set); // IUnknown::Release
        this_object->property_set = 0;
    }
}

#if 0
Original Ghidra decompilation (0x54ec40):

void FUN_0054ec40(void)

{
  int *piVar1;
  int in_ECX;

  piVar1 = *(int **)(in_ECX + 0x18);
  if (piVar1 != (int *)0x0) {
    (**(code **)(*piVar1 + 8))(piVar1);
    *(undefined4 *)(in_ECX + 0x18) = 0;
  }
  return;
}

Disassembly (0x54ec40..0x54ec59, capstone; phase-4 review):

0x54ec40: push esi
0x54ec41: mov esi, ecx
0x54ec43: mov eax, dword ptr [esi + 0x18]
0x54ec46: test eax, eax
0x54ec48: je 0x54ec57
0x54ec4a: mov ecx, dword ptr [eax]
0x54ec4c: push eax
0x54ec4d: call dword ptr [ecx + 8]
0x54ec50: mov dword ptr [esi + 0x18], 0
0x54ec57: pop esi
0x54ec58: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
