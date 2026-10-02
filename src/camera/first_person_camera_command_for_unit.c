// first_person_camera_command_for_unit  (Ghidra: FUN_00446d30; renamed, Blam-style, not previously named)
// address 0x446d30, size 44 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// reviewed (phase 4 gate): objdump 0x446d30..0x446d5b; aiming vector is object +0x23c (unit_data base fix).
// evidence: out/phase4/camera_functions.md: "Forwards to first_person_camera_for_unit_and_vector
//   to compute the first-person camera pov." It resolves `unit`'s object_header directly out of
//   the global object data_array (no validity check of its own -- callers already validated the
//   handle with object_try_and_get) and passes its aiming_vector (types/units.h unit_data
//   +0x23c) as the "vector" argument.
// register convention: unit handle in ECX (in_ECX); command pointer as the single cdecl stack
//   parameter (confirmed with objdump: `mov ebx,[esp+8]` reads it right after `push ebx`, i.e.
//   the first stack slot past the return address).
//   // blam-cc: ECX -> unit, stack -> command

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void first_person_camera_for_unit_and_vector(observer_command *command, Vector3D *vector,
                                                      datum_index unit); // 0x446b70, this module;
                                                     // blam-cc: EBX -> command, EAX -> vector, stack -> unit

// blam-cc: ECX -> unit, stack -> command
void first_person_camera_command_for_unit(datum_index unit, observer_command *command)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[unit & 0xffff].data;
    Vector3D *aiming_vector = (Vector3D *)&((unit_data *)((uint8_t *)obj + k_unit_data_offset))->aiming_vector; // object +0x23c (0x446d4c)

    first_person_camera_for_unit_and_vector(command, aiming_vector, unit);
}

#if 0
Original Ghidra decompilation (0x446d30):

void FUN_00446d30(void)

{
  first_person_camera_for_unit_and_vector();
  return;
}

Disassembly (objdump -d -M intel, 0x446d30..0x446d5b), showing the register roles Ghidra dropped:

0x446d30: mov edx, dword ptr [0x8603b0]      ; object_data
0x446d36: mov edx, dword ptr [edx + 0x34]    ; object_data->data
0x446d39: mov eax, ecx                        ; unit (in_ECX)
0x446d3b: and eax, 0xffff
0x446d40: lea eax, [eax + eax*2]              ; index * 3
0x446d43: mov eax, dword ptr [edx + eax*4 + 8] ; object_header[index].data  (stride 0xc, field +8)
0x446d47: push ebx
0x446d48: mov ebx, dword ptr [esp + 8]        ; command (single stack arg)
0x446d4c: add eax, 0x23c                       ; &unit_data.aiming_vector
0x446d51: push ecx                             ; unit, forwarded on the stack
0x446d52: call 0x446b70                        ; first_person_camera_for_unit_and_vector(EBX=command, EAX=vector, stack=unit)
0x446d57: add esp, 4
0x446d5a: pop ebx
0x446d5b: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
