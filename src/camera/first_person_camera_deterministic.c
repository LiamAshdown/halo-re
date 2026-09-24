// first_person_camera_deterministic  (Ghidra: first_person_camera_deterministic, already named)
// address 0x446a90, size 217 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: cea-pdb symbol match via the "primary trigger" string; out/phase4/camera_functions.md
//   "Returns the deterministic first-person camera position for a unit, optionally overridden
//   by its weapon's primary-trigger marker when present." Shares its seat/marker logic with
//   first_person_camera_for_unit_and_vector (0x446b70) almost verbatim; built from objdump since
//   Ghidra drops every register argument here too.
// register convention: output position in EAX (in_EAX), unit handle in ECX (in_ECX), output
//   direction as the single cdecl stack parameter (confirmed with objdump).
//   // blam-cc: EAX -> out_position, ECX -> unit, stack -> out_direction

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "camera.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void unit_get_camera_position(datum_index unit, real_point3d *out_position); // 0x568f80, units module, not yet rewritten
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, objects module
// cdecl, four stack arguments (0x446b1e..0x446b30 / 0x446c56..0x446c68: four pushes, add
// esp,0x10); the result is tested as AX. The objects module rewrite declares EAX/ECX/EDX
// register arguments and an int32 result for this function, which these call sites and the
// callee prologue (0x4f6080 mov eax,[esp+0x4]) contradict.
extern int16_t object_get_node_local_transform(datum_index object_index, const char *marker_name,
    object_marker *markers, int32_t maximum_count); // 0x4f6080, objects module
void first_person_camera_deterministic(Point3D *out_position, datum_index unit, Vector3D *out_direction)
{
    object_header *headers = (object_header *)object_data->data;
    object *unit_object = headers[unit & 0xffff].data;
    datum_index parent;

    unit_get_camera_position(unit, (real_point3d *)out_position);
    *out_direction = *(Vector3D *)((uint8_t *)unit_object + 0x23c); // unit_data.aiming_vector

    parent = unit_object->parent_object;
    if (parent == k_datum_index_none) {
        return;
    }

    {
        object *parent_object = object_try_and_get(parent, 2);
        if (parent_object == (object *)0) {
            return;
        }

        {
            Unit *parent_unit_tag = (Unit *)tag_instances[parent_object->definition_tag & 0xffff].data;
            uint8_t *seats = (uint8_t *)parent_unit_tag->seats.pointer;
            int16_t seat_index = ((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->vehicle_seat_index;
            int8_t seat_flags = *(int8_t *)(seats + (int32_t)seat_index * sizeof(UnitSeat));

            if (seat_flags < 0) { // UnitSeatFlags::first_person_camera_slaved_to_gun (bit 0x80)
                object_marker marker; // 0x6c bytes on the stack
                int16_t ok = object_get_node_local_transform(parent, "primary trigger", &marker, 1);
                if (ok != 0) {
                    real_matrix4x3 *m = &marker.node_transform; // marker +0x38
                    *out_position = *(Point3D *)&m->position;
                    *out_direction = *(Vector3D *)&m->forward;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x446a90) -- UNRELIABLE, every register argument is lost; kept
for reference only:

void first_person_camera_deterministic(undefined4 *param_1)

{
  int iVar1;
  int iVar2;
  short sVar3;
  undefined4 *in_EAX;
  uint *puVar4;
  uint in_ECX;
  undefined1 local_6c [60];
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  unit_get_camera_position();
  *param_1 = *(undefined4 *)(iVar1 + 0x23c);
  param_1[1] = *(undefined4 *)(iVar1 + 0x240);
  param_1[2] = *(undefined4 *)(iVar1 + 0x244);
  iVar2 = *(int *)(iVar1 + 0x11c);
  if (iVar2 != -1) {
    puVar4 = (uint *)object_try_and_get(2);
    if ((puVar4 != (uint *)0x0) &&
       (*(char *)(*(short *)(iVar1 + 0x2f0) * 0x11c +
                 *(int *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)) < '\0'
       )) {
      sVar3 = object_get_node_local_transform(iVar2,"primary trigger",local_6c,1);
      if (sVar3 != 0) {
        *in_EAX = local_c;
        in_EAX[1] = local_8;
        in_EAX[2] = local_4;
        *param_1 = local_30;
        param_1[1] = local_2c;
        param_1[2] = local_28;
      }
    }
  }
  return;
}

Disassembly (objdump -d -M intel, 0x446a90..0x446b68), confirming the register roles:

0x446a90: mov edx, dword ptr [0x8603b0]
0x446a99: sub esp, 0x6c
0x446a9e: mov ebp, dword ptr [esp + 0x78]     ; out_direction (single stack arg)
0x446aa4: mov edi, eax                          ; out_position (EAX)
0x446aa6: mov eax, ecx                          ; unit (ECX), copied for indexing only
0x446ab4: call 0x568f80                          ; unit_get_camera_position(ECX=unit, EDI=out_position)
0x446ab9..0x446ace: *out_direction = unit_object+0x23c (aiming_vector)   ; written through ebp
0x446ad1: mov ebx, dword ptr [esi + 0x11c]      ; parent_object
0x446ad7: cmp ebx, -1
0x446ada: je 0x446b61
0x446ae0: push 2
0x446ae2: mov ecx, ebx
0x446ae4: call 0x4f6ec0                          ; object_try_and_get(ECX=parent, stack=2)
0x446b18: cmp byte ptr [ecx + edx], 0
0x446b1c: jns 0x446b61                            ; sign bit clear -> not slaved, skip marker override
0x446b1e..0x446b2b: object_get_node_local_transform(parent, "primary trigger", out, 1)
0x446b33: test ax, ax
0x446b36: je 0x446b61
0x446b38..0x446b51: *out_position (edi) <- out+0x60 (position)
0x446b46..0x446b5e: *out_direction (ebp) <- out+0x3c (forward)
#endif
