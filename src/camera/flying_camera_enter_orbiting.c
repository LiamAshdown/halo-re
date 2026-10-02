// flying_camera_enter_orbiting  (no Ghidra function; new entry)
// address 0x446a10, size 123 bytes (0x446a10..0x446a8a)
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: flying_camera_transition_procs[1][1] (.data 0x00686abc), the slot that
//   flying_camera_initialize (0x446350) and flying_camera_compute_pov (0x4464f0) call when the
//   flying sub-mode is 1 (orbiting). It saves the record as the flying state (7 dwords into
//   0x006f1830) and either restores the saved orbiting record (0x006f1850, when 0x006f186c is
//   set) or seeds a new one: {0, 1.0, 0, yaw, pitch} from the render camera forward.
// register convention: cdecl, the record as the only stack argument.
//   // blam-cc: stack -> data
// No Ghidra decompilation exists for this address; the #if 0 block carries the objdump.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *flying_camera_render_frame;                    // 0x00686aa4 -> 0x007c3100; +0x14 is a render_camera
extern editor_camera_data flying_camera_saved_flying;       // 0x006f1830
extern orbiting_camera_data flying_camera_saved_orbiting;   // 0x006f1850
extern uint8_t flying_camera_saved_orbiting_valid;          // 0x006f186c

extern double atan2(double y, double x);
extern double sqrt(double x);

// blam-cc: stack -> data
void flying_camera_enter_orbiting(editor_camera_data *data)
{
    orbiting_camera_data *orbit = (orbiting_camera_data *)data;
    render_camera *camera;

    flying_camera_saved_flying = *data;
    if (flying_camera_saved_orbiting_valid) {
        *orbit = flying_camera_saved_orbiting;
        return;
    }
    camera = (render_camera *)((uint8_t *)flying_camera_render_frame + 0x14);
    orbit->unknown_00 = 0.0f;
    orbit->distance = 1.0f;
    orbit->unknown_08 = 0.0f;
    orbit->yaw = (float)atan2(camera->forward.j, camera->forward.i);
    orbit->pitch = (float)atan2(camera->forward.k,
        sqrt(camera->forward.i * camera->forward.i + camera->forward.j * camera->forward.j));
}

#if 0
No Ghidra function exists at 0x446a10. objdump -d -M intel:

  446a10: mov eax,[esp+0x4]              ; data
  446a16: mov ecx,0x7 / mov esi,eax / mov edi,0x6f1830 / rep movs   ; save as flying
  446a24: mov cl,ds:0x6f186c
  446a2c: je 0x446a3f
  446a2e: mov ecx,0x7 / mov esi,0x6f1850 / mov edi,eax / rep movs   ; restore orbiting
  446a3e: ret
  446a3f: mov ecx,ds:0x686aa4
  446a45: mov DWORD PTR [eax],0x0
  446a4b: mov DWORD PTR [eax+0x4],0x3f800000
  446a52: mov DWORD PTR [eax+0x8],0x0
  446a59: fld [ecx+0x24] / fld [ecx+0x20] / fpatan / fstp [eax+0xc]   ; yaw
  446a66..446a85: atan2(forward.k, sqrt(i*i + j*j)) -> [eax+0x10]     ; pitch
  446a8a: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
