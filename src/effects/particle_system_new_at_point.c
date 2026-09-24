// particle_system_new_at_point  (Ghidra: FUN_00453600, still unnamed there)
// address 0x453600, size 236 bytes
// name confidence: 0.5 (out/phase4/effects_types_notes.md names it directly: "particle_system_new_at_point
//   0x453600")   rewrite confidence: 0.55
// evidence: types/effects.h particle_system fields definition_index (+0x08), object_index
//   (+0x0c), position (+0x20), velocity (+0x2c), color (+0x38), scale (+0x14), ambient_color
//   (+0x48) and flags (+0x04, _particle_system_emitting_bit) all match this function's writes
//   offset for offset; global 0x0069c566 particle_systems_enabled is already named in
//   types/effects.h's globals list; object_sample_ambient_lightmap_point (0x4f1e60) is plain cdecl
//   (point, lightmap_color, base_map_color, wait_for_textures): 0x4536a8..0x4536ba pushes 0,
//   &scratch, system + 0x48 and ECX = the system's own position copy (orphan pass 4).
// register convention: definition_index and scale are Ghidra-recognized stack parameters;
//   position pointer is also a recognized stack parameter; velocity pointer in ECX (in_ECX);
//   color pointer in EAX (in_EAX).
//   // blam-cc: stack -> definition_index, stack -> position, in_ECX -> velocity, in_EAX -> color,
//   //   stack -> scale
// The base-map color output of object_sample_ambient_lightmap_point (local_c, 12 bytes) is
//   never read afterward here, kept as an ignored scratch buffer.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include <stdint.h>  // uintptr_t only; this is a .c file, not a Ghidra-ingested header

extern data_array *particle_system_data; // 0x0087abd4
extern uint8_t particle_systems_enabled; // 0x0069c566

extern datum_index datum_new(data_array *array); // 0x4d0480
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510
extern void object_sample_ambient_lightmap_point(real_point3d *point, real_vector3d *lightmap_color,
    real_vector3d *base_map_color, uint8_t wait_for_textures); // 0x4f1e60, objects module, cdecl
extern uint8_t particle_system_new_type_states(datum_index handle); // 0x4538b0, this module

datum_index particle_system_new_at_point(uint32_t definition_index, real_point3d *position,
    real_vector3d *velocity, ColorARGB *color, float scale)
    // blam-cc: stack, stack, in_ECX, in_EAX, stack
{
    datum_index handle = (datum_index)0xffffffff;

    if (particle_systems_enabled != 0) {
        handle = datum_new(particle_system_data);
        if (handle != (datum_index)0xffffffff) {
            particle_system *system =
                &((particle_system *)particle_system_data->data)[handle & 0xffff];
            real_vector3d incident_scratch;

            system->definition_index = definition_index;
            system->object_index = (datum_index)0xffffffff;
            system->position = *position;
            system->velocity = *velocity;
            system->color = *color;
            system->scale = scale;
            system->flags |= _particle_system_emitting_bit;

            object_sample_ambient_lightmap_point(&system->position,
                (real_vector3d *)&system->ambient_color, &incident_scratch, 0);

            if (!particle_system_new_type_states(handle)) {
                datum_delete(particle_system_data, handle);
                return (datum_index)0xffffffff;
            }
        }
    }
    return handle;
}

#if 0
Original Ghidra decompilation (0x453600):

uint FUN_00453600(undefined4 param_1,undefined4 *param_2,undefined4 param_3)

{
  char cVar1;
  undefined4 *in_EAX;
  uint uVar2;
  int iVar3;
  undefined4 *in_ECX;
  undefined8 uVar4;
  undefined1 local_c [12];

  uVar2 = 0xffffffff;
  if (DAT_0069c566 != '\0') {
    uVar4 = datum_new();
    uVar2 = (uint)uVar4;
    if (uVar2 != 0xffffffff) {
      iVar3 = (uVar2 & 0xffff) * 0x158 + *(int *)((int)((ulonglong)uVar4 >> 0x20) + 0x34);
      *(undefined4 *)(iVar3 + 8) = param_1;
      *(undefined4 *)(iVar3 + 0xc) = 0xffffffff;
      *(undefined4 *)(iVar3 + 0x20) = *param_2;
      *(undefined4 *)(iVar3 + 0x24) = param_2[1];
      *(undefined4 *)(iVar3 + 0x28) = param_2[2];
      *(undefined4 *)(iVar3 + 0x2c) = *in_ECX;
      *(undefined4 *)(iVar3 + 0x30) = in_ECX[1];
      *(undefined4 *)(iVar3 + 0x34) = in_ECX[2];
      *(undefined4 *)(iVar3 + 0x38) = *in_EAX;
      *(undefined4 *)(iVar3 + 0x3c) = in_EAX[1];
      *(undefined4 *)(iVar3 + 0x40) = in_EAX[2];
      *(undefined4 *)(iVar3 + 0x44) = in_EAX[3];
      *(undefined4 *)(iVar3 + 0x14) = param_3;
      *(uint *)(iVar3 + 4) = *(uint *)(iVar3 + 4) | 1;
      object_sample_ambient_lightmap_point((undefined4 *)(iVar3 + 0x20),iVar3 + 0x48,local_c,0);
      cVar1 = FUN_004538b0(uVar2);
      if (cVar1 == '\0') {
        datum_delete();
        return 0xffffffff;
      }
    }
  }
  return uVar2;
}
#endif
