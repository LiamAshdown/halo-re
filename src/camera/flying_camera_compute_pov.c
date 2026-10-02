// flying_camera_compute_pov  (no Ghidra function; new entry)
// address 0x4464f0, size 215 bytes (0x4464f0..0x4465c6)
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: director_set_flying_camera (0x445f40) installs it as director.pov_proc
//   (0x445f69 mov [esi+0x8],0x4464f0), and it is the only other code address stored there
//   (.data references at file offsets 0x45f57 / 0x45f6c). It dispatches through
//   flying_camera_update_procs[flying_camera_current_mode] (0x00686aa8: 0x4465d0 flying,
//   0x446870 orbiting). While flying_camera_follow_script is set it defers to the scripted pov
//   (called with a NULL mode data pointer, 0x446510 push 0) until look input arrives, then
//   re-seeds the flying record from the render camera and forces the command to snap.
// register convention: director_pov_proc (cdecl, three stack arguments; edi = [esp+0x10] input
//   after the two pushes, the data / command arguments are re-read at 0x446591 / 0x446595).
//   // blam-cc: stack -> (data, input, command)
// No Ghidra decompilation exists for this address; the #if 0 block carries the objdump.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t flying_camera_follow_script;                 // 0x006f17fd, UNSURE name
extern void *flying_camera_render_frame;                    // 0x00686aa4 -> 0x007c3100; +0x14 is a render_camera
extern editor_camera_data *flying_camera_data;              // 0x006f1814
extern datum_index flying_camera_attached_object;           // 0x00686aa0
extern int16_t flying_camera_current_mode;                  // 0x006f1828
extern director_pov_proc flying_camera_update_procs[2];     // 0x00686aa8
extern flying_camera_transition_proc flying_camera_transition_procs[2][2]; // 0x00686ab0

extern double atan2(double y, double x);
extern double sqrt(double x);

extern void camera_debug_compute_pov(director_camera_data *data, camera_input *input,
    observer_command *command); // 0x444d50, this module (the scripted camera pov)
// blam-cc: EAX -> object_index
extern void flying_camera_attach_to_object(datum_index object_index); // 0x446470, this module

// blam-cc: stack -> (data, input, command)
void flying_camera_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    if (flying_camera_follow_script) {
        render_camera *camera;
        editor_camera_data *flying;

        if (!input->has_look_input) {
            camera_debug_compute_pov((director_camera_data *)0, input, command);
            return;
        }
        camera = (render_camera *)((uint8_t *)flying_camera_render_frame + 0x14);
        flying = flying_camera_data;
        flying->position = *(Point3D *)&camera->position;
        flying->yaw = (float)atan2(camera->forward.j, camera->forward.i);
        flying->pitch = (float)atan2(camera->forward.k,
            sqrt(camera->forward.i * camera->forward.i + camera->forward.j * camera->forward.j));
        flying_camera_attach_to_object(flying_camera_attached_object);
        if (flying_camera_current_mode != 0) {
            flying_camera_transition_procs[flying_camera_current_mode][1](flying);
        }
    }

    flying_camera_update_procs[flying_camera_current_mode](data, input, command);

    if (flying_camera_follow_script) {
        command->flags |= _observer_command_valid_bit | _observer_command_snap_bit;
        command->timer = 0.0f;
    }
}

#if 0
No Ghidra function exists at 0x4464f0. objdump -d -M intel:

  4464f0: mov al,ds:0x6f17fd                 ; flying_camera_follow_script
  4464f5: test al,al
  4464f7: push esi
  4464f8: push edi
  4464f9: mov edi,DWORD PTR [esp+0x10]      ; input
  4464fd: je 0x446591
  446503: mov al,BYTE PTR [edi+0x2]          ; input->has_look_input
  446506: test al,al
  446508: jne 0x44651d
  44650a: mov eax,DWORD PTR [esp+0x14]
  44650e: push eax                           ; command
  44650f: push edi                           ; input
  446510: push 0x0                           ; data = NULL
  446512: call 0x444d50
  446517: add esp,0xc
  44651a: pop edi
  44651b: pop esi
  44651c: ret
  44651d: mov eax,ds:0x686aa4                ; render frame block
  446522: mov esi,DWORD PTR ds:0x6f1814      ; flying_camera_data
  446528: lea ecx,[eax+0x14]                 ; render_camera.position
  44652b..44653b: copy 3 dwords to [esi]
  44653e: fld [eax+0x24] / fld [eax+0x20] / fpatan / fstp [esi+0xc]     ; yaw
  446549..44656d: atan2(forward.k, sqrt(i*i + j*j)) -> [esi+0x10]         ; pitch
  446556: mov eax,ds:0x686aa0
  446572: call 0x446470                      ; flying_camera_attach_to_object(EAX)
  446577: mov ax,ds:0x6f1828
  44657d: test ax,ax
  446581: je 0x446591
  446583: movsx edx,ax
  446586: push esi
  446587: call DWORD PTR [edx*8+0x686ab4]    ; flying_camera_transition_procs[mode][1](flying)
  44658e: add esp,0x4
  446591: mov esi,DWORD PTR [esp+0x14]       ; command
  446595: mov ecx,DWORD PTR [esp+0xc]        ; data
  446599: movsx eax,WORD PTR ds:0x6f1828
  4465a0: push esi / push edi / push ecx
  4465a3: call DWORD PTR [eax*4+0x686aa8]    ; flying_camera_update_procs[mode]
  4465aa: mov al,ds:0x6f17fd
  4465af: add esp,0xc
  4465b2: test al,al
  4465b4: je 0x4465c4
  4465b6: mov eax,DWORD PTR [esi]
  4465b8: or eax,0x9
  4465bb: mov DWORD PTR [esi+0x48],0x0       ; command->timer
  4465c2: mov DWORD PTR [esi],eax
  4465c4: pop edi
  4465c5: pop esi
  4465c6: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
