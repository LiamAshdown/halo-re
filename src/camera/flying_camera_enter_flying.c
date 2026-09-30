// flying_camera_enter_flying  (no Ghidra function; new entry)
// address 0x4469a0, size 112 bytes (0x4469a0..0x446a0f)
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: flying_camera_transition_procs[1][0] (.data 0x00686ab8). It saves the current
//   record as the orbiting state (7 dword rep movs into 0x006f1850, flag 0x006f186c = 1), then
//   re-seeds the record for flying from the render camera (position, yaw, pitch) and tail-jumps
//   into flying_camera_attach_to_object with the attached object. No instruction or table slot
//   this build indexes reaches it (only [mode][1] is ever called), so it is dead code here.
// register convention: cdecl, the record as the only stack argument.
//   // blam-cc: stack -> data
// No Ghidra decompilation exists for this address; the #if 0 block carries the objdump.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "camera.h"
#include "fn_camera.h"

extern void *flying_camera_render_frame;                    // 0x00686aa4 -> 0x007c3100; +0x14 is a render_camera
extern datum_index flying_camera_attached_object;           // 0x00686aa0
extern orbiting_camera_data flying_camera_saved_orbiting;   // 0x006f1850
extern uint8_t flying_camera_saved_orbiting_valid;          // 0x006f186c

extern double atan2(double y, double x);
extern double sqrt(double x);

// blam-cc: EAX -> object_index


// blam-cc: stack -> data
void flying_camera_enter_flying(editor_camera_data *data)
{
    render_camera *camera = (render_camera *)((uint8_t *)flying_camera_render_frame + 0x14);

    flying_camera_saved_orbiting = *(orbiting_camera_data *)data;
    flying_camera_saved_orbiting_valid = 1;
    data->position = *(Point3D *)&camera->position;
    data->yaw = (float)atan2(camera->forward.j, camera->forward.i);
    data->pitch = (float)atan2(camera->forward.k,
        sqrt(camera->forward.i * camera->forward.i + camera->forward.j * camera->forward.j));
    flying_camera_attach_to_object(flying_camera_attached_object);
}

#if 0
No Ghidra function exists at 0x4469a0. objdump -d -M intel:

  4469a0: mov edx,[esp+0x4]              ; data
  4469a4: mov eax,ds:0x686aa4            ; render frame block
  4469ab: mov ecx,0x7 / mov esi,edx / mov edi,0x6f1850 / rep movs   ; save as orbiting
  4469b9: mov BYTE PTR ds:0x6f186c,0x1
  4469c0: lea ecx,[eax+0x14]             ; render_camera.position -> data->position
  4469d6: fld [eax+0x24] / fld [eax+0x20] / fpatan / fstp [edx+0xc]     ; yaw
  4469e2..446a06: atan2(forward.k, sqrt(i*i + j*j)) -> [edx+0x10]       ; pitch
  4469ef: mov eax,ds:0x686aa0
  446a0b: jmp 0x446470                   ; flying_camera_attach_to_object(EAX)
#endif
