// first_person_camera_for_unit_and_vector  (Ghidra: first_person_camera_for_unit_and_vector, already named)
// address 0x446b70, size 439 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: cea-pdb symbol match via the "primary trigger" string; out/phase4/camera_functions.md
//   "Computes the first-person camera transform for a unit and an accompanying direction vector,
//   using the weapon's primary-trigger marker when present." Ghidra's decompiler loses every
//   register argument in this function (the incoming `command`/`vector` registers and every
//   callee's register-passed arguments); this rewrite is built from objdump instead, see the
//   #if 0 block for the full instruction trace.
// review (phase 4 gate, line by line against objdump 0x446b70..0x446d26): matches; the only
//   fix was the seat index read, which now goes through unit_data at object +0x1f4 (+0x2f0).
// register convention: command pointer in EBX (unaff_EBX), direction vector in EAX (in_EAX);
//   the unit handle is the single cdecl stack parameter.
//   // blam-cc: EBX -> command, EAX -> vector, stack -> unit
// UNSURE: unit_get_camera_position (0x568f80) may also read EDX (the unit object pointer is
//   live in EDX at 0x446bee). The marker lookup fills a types/objects.h object_marker (0x6c);
//   the camera reads its node_transform at +0x38 (forward +0x3c, up +0x54, position +0x60).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "camera.h"
#include "fn_math.h"

extern const real_point3d *global_origin3d_pointer; // 0x00696714
extern data_array *object_data;                     // 0x008603b0
extern tag_instance *tag_instances;                 // 0x0087bc14

extern void unit_get_camera_position(datum_index unit, real_point3d *out_position); // 0x568f80, units module,
                                                     // not yet rewritten; UNSURE: EDX (unit object
                                                     // pointer) may also be a live input, see #if 0
extern void object_get_root_object_velocities(datum_index object_index, real_vector3d *out_velocity,
                                               real_vector3d *out_angular_velocity); // 0x4f6aa0, objects module;
                                                     // blam-cc: EAX -> object_index, ESI -> out_velocity, EDI -> out_angular_velocity
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, objects module
extern void vector3d_compute_up_from_forward(Vector3D *forward, Vector3D *out_up); // 0x4479c0, this module
                                                     // (rewritten separately); blam-cc: ESI -> forward, EDI -> out_up


// cdecl, four stack arguments (0x446b1e..0x446b30 / 0x446c56..0x446c68: four pushes, add
// esp,0x10); the result is tested as AX. The objects module rewrite declares EAX/ECX/EDX
// register arguments and an int32 result for this function, which these call sites and the
// callee prologue (0x4f6080 mov eax,[esp+0x4]) contradict.
extern int16_t object_get_node_local_transform(datum_index object_index, const char *marker_name,
    object_marker *markers, int32_t maximum_count); // 0x4f6080, objects module
void first_person_camera_for_unit_and_vector(observer_command *command, Vector3D *vector, datum_index unit)
{
    command->timer = 0.0f;
    command->flags = 0;
    command->parameters.focus_offset = *(Vector3D *)global_origin3d_pointer;
    command->parameters.distance = 0.0f;
    command->parameters.forward = *vector;
    command->parameters.field_of_view = 1.2217305f; // 70 degrees
    vector3d_compute_up_from_forward(&command->parameters.forward, &command->parameters.up);

    if (unit == k_datum_index_none) {
        return;
    }

    {
        object_header *headers = (object_header *)object_data->data;
        object *unit_object = headers[unit & 0xffff].data;
        datum_index parent;
        object *parent_object;

        unit_get_camera_position(unit, (real_point3d *)&command->parameters.position);
        object_get_root_object_velocities(unit, (real_vector3d *)&command->velocity, (real_vector3d *)0);

        parent = unit_object->parent_object;
        if (parent == k_datum_index_none) {
            command->flags = 1;
            return;
        }

        parent_object = object_try_and_get(parent, 2);
        if (parent_object == (object *)0) {
            command->flags = 1;
            return;
        }

        {
            Unit *parent_unit_tag = (Unit *)tag_instances[parent_object->definition_tag & 0xffff].data;
            uint8_t *seats = (uint8_t *)parent_unit_tag->seats.pointer;
            int16_t seat_index = ((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->vehicle_seat_index;
            uint8_t seat_flags = *(uint8_t *)(seats + (int32_t)seat_index * sizeof(UnitSeat));

            if ((seat_flags & 0x80) != 0) { // UnitSeatFlags first_person_camera_slaved_to_gun
                object_marker marker; // 0x6c bytes on the stack
                int16_t ok = object_get_node_local_transform(parent, "primary trigger", &marker, 1);
                if (ok != 0) {
                    real_matrix4x3 *m = &marker.node_transform; // marker +0x38
                    command->parameters.position = *(Point3D *)&m->position;
                    command->parameters.forward = *(Vector3D *)&m->forward;
                    command->parameters.up = *(Vector3D *)&m->up;
                    command->flags = 1;
                    return;
                }
            } else {
                real_matrix4x3 seat_matrix;
                matrix4x3_from_forward_up_position((real_vector3d *)&parent_object->up,
                                                    (real_vector3d *)&parent_object->forward,
                                                    (real_point3d *)&parent_object->position, &seat_matrix);
                matrix4x3_inverse_transform_normal((real_vector3d *)&command->parameters.forward,
                                                    (real_vector3d *)&command->parameters.forward, &seat_matrix);
                vector3d_compute_up_from_forward(&command->parameters.forward, &command->parameters.up);
                matrix4x3_transform_normal((real_vector3d *)&command->parameters.forward,
                                            (real_vector3d *)&command->parameters.forward, &seat_matrix);
                matrix4x3_transform_normal((real_vector3d *)&command->parameters.up,
                                            (real_vector3d *)&command->parameters.up, &seat_matrix);
            }
        }

        command->flags = 1;
    }
}

#if 0
Original Ghidra decompilation (0x446b70) -- UNRELIABLE, every register argument is lost; kept
for reference only:

void first_person_camera_for_unit_and_vector(uint param_1)

{
  int iVar1;
  int iVar2;
  undefined *puVar3;
  short sVar4;
  undefined4 *in_EAX;
  uint *puVar5;
  undefined4 *unaff_EBX;
  undefined1 local_b0 [60];
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined1 local_40 [60];

  unaff_EBX[0x12] = 0;
  *unaff_EBX = 0;
  puVar3 = PTR_DAT_00696714;
  unaff_EBX[4] = *(undefined4 *)PTR_DAT_00696714;
  unaff_EBX[5] = *(undefined4 *)(puVar3 + 4);
  unaff_EBX[6] = *(undefined4 *)(puVar3 + 8);
  unaff_EBX[7] = 0;
  unaff_EBX[9] = *in_EAX;
  unaff_EBX[10] = in_EAX[1];
  unaff_EBX[0xb] = in_EAX[2];
  unaff_EBX[8] = 0x3f9c61aa;
  FUN_004479c0();
  if (param_1 != 0xffffffff) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    unit_get_camera_position();
    FUN_004f6aa0();
    iVar2 = *(int *)(iVar1 + 0x11c);
    if (iVar2 != -1) {
      puVar5 = (uint *)object_try_and_get(2);
      if (puVar5 != (uint *)0x0) {
        if ((*(byte *)(*(short *)(iVar1 + 0x2f0) * 0x11c +
                      *(int *)(*(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)) &
            0x80) == 0) {
          matrix4x3_from_forward_up_position(local_40);
          matrix4x3_inverse_transform_normal(local_40);
          FUN_004479c0();
          matrix4x3_transform_normal(local_40);
          matrix4x3_transform_normal(local_40);
        }
        else {
          sVar4 = object_get_node_local_transform(iVar2,"primary trigger",local_b0,1);
          if (sVar4 != 0) {
            unaff_EBX[1] = local_50;
            unaff_EBX[2] = local_4c;
            unaff_EBX[3] = local_48;
            unaff_EBX[9] = local_74;
            unaff_EBX[10] = local_70;
            unaff_EBX[0xb] = local_6c;
            unaff_EBX[0xc] = local_5c;
            unaff_EBX[0xd] = local_58;
            unaff_EBX[0xe] = local_54;
            *unaff_EBX = 1;
            return;
          }
        }
      }
    }
    *unaff_EBX = 1;
  }
  return;
}

Disassembly (objdump -d -M intel, 0x446b70..0x446d26) that this rewrite is actually built from:

0x446b70: push ebp
0x446b71: mov ebp, esp
0x446b73: and esp, 0xfffffff8
0x446b76: sub esp, 0xb0
0x446b7e: mov dword ptr [ebx + 0x48], 0        ; command.timer = 0
0x446b81: mov dword ptr [ebx], 0                ; command.flags = 0
0x446b84: mov esi, dword ptr [0x696714]         ; global_origin3d_pointer
0x446b8d: lea edx, [ebx + 0x10]                  ; &command.focus_offset = zero vector
0x446b9e: mov dword ptr [ebx + 0x1c], 0          ; command.distance = 0
0x446ba1: mov edx, dword ptr [eax]               ; eax = vector (register arg)
0x446ba3: lea esi, [ebx + 0x24]                   ; &command.forward
0x446ba8..0x446bb6: command.forward = *vector
0x446bb9: mov dword ptr [ebx + 0x20], 0x3f9c61aa  ; command.fov = 70 degrees
0x446bc0: call 0x4479c0                            ; up = f(forward)  (ESI/EDI)
0x446bc5: mov ecx, dword ptr [ebp + 8]             ; unit (single stack arg)
0x446bc8: cmp ecx, -1
0x446bcb: je 0x446d21                              ; unit == -1 -> return with flags still 0
0x446bd1..0x446be3: edx = object_header[unit].data  (unit object pointer)
0x446be7: lea edi, [ebx + 4]                        ; &command.position
0x446bee: call 0x568f80                              ; unit_get_camera_position(ECX=unit, EDI=out)
0x446bf3: mov eax, dword ptr [ebp + 8]
0x446bf6: lea esi, [ebx + 0x3c]                      ; &command.velocity
0x446bf9: xor edi, edi
0x446bfb: call 0x4f6aa0                               ; FUN_004f6aa0(EAX=unit, ESI=out_velocity, EDI=0)
0x446c00: mov edi, dword ptr [esp + 0xc]              ; unit object pointer, restored
0x446c04: mov esi, dword ptr [edi + 0x11c]            ; parent handle
0x446c0a: cmp esi, -1
0x446c0d: je 0x446d1b                                  ; no parent -> flags=1, return
0x446c13: push 2
0x446c15: mov ecx, esi
0x446c17: call 0x4f6ec0                                 ; object_try_and_get(ECX=parent, stack=2)
0x446c1c: mov edx, eax
0x446c21: test edx, edx
0x446c23: je 0x446d1b                                    ; parent invalid -> flags=1, return
0x446c29..0x446c4a: eax = Vehicle_tag->seats.pointer; ecx = seat_index * sizeof(UnitSeat)
0x446c50: test byte ptr [ecx + eax], 0x80                ; UnitSeatFlags::first_person_camera_slaved_to_gun
0x446c54: je 0x446cbf                                     ; not slaved -> matrix path
0x446c56..0x446c63: object_get_node_local_transform(parent, "primary trigger", out, 1)  ; esi=parent (handle, not a resolved pointer)
0x446c6b: test ax, ax
0x446c6e: je 0x446d1b                                     ; failed -> flags=1, return (keep vector defaults)
0x446c74..0x446cb3: command.position/forward/up <- out+0x60/+0x3c/+0x54 (i.e. a real_matrix4x3 at out+0x38)
0x446cb3: mov dword ptr [ebx], 1
0x446cb9..0x446cbe: pop edi/esi, epilogue, ret
0x446cbf..0x446cd3: seat_matrix = matrix4x3_from_forward_up_position(parent+0x80, parent+0x74, parent+0x5c)
0x446ce0..0x446ce5: command.forward = matrix4x3_inverse_transform_normal(command.forward, seat_matrix)
0x446cef: call 0x4479c0                                    ; up = f(forward), in seat-local space
0x446cfb..0x446d01: command.forward = matrix4x3_transform_normal(command.forward, seat_matrix)
0x446d0d..0x446d13: command.up = matrix4x3_transform_normal(command.up, seat_matrix)
0x446d18: add esp, 0x10                                     ; pops all four deferred stack args at once
0x446d1b: mov dword ptr [ebx], 1
0x446d21..0x446d26: pop edi/esi, epilogue, ret
#endif
