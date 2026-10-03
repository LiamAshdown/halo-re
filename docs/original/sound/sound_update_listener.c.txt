// sound_update_listener  (Ghidra: sound_update_listener, already named)
// address 0x54b970, size 431 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Recomputes the 3D audio listener's orientation and
// position from the local player/camera each update and pushes it to the sound driver.";
// disassembly (scratchpad/disasm/disasm.py) resolves every implicit register argument:
// matrix4x3_from_forward_up(up=camera_row+0x2c, forward=camera_row+0x20, out=&listener->scale);
// matrix4x3_inverse_transform_vector(out=&listener->velocity, v=camera_row+0x14, m=&listener->scale).
// observers[0].camera (camera.h observer_camera at 0x006ac6d0, R17) supplies the
// position (+0x00), the {leaf, cluster} location handed to the underwater test (+0x0c), the
// velocity (+0x14), forward (+0x20) and up (+0x2c). Globals->sounds[0]/[1] (types/tags.h Globals,
// 0xf8 TagReflexive of GlobalsSound) are the enter/exit-water sounds, started unspatialized
// with first_person_hint 1 when their tag is set.
// Phase-4 review (disassembly appended below): the camera table is an array, not a pointer;
// the water sound location is a full 0x40-byte sound_location, not a 12-byte stub.
// register convention: void, no parameters.
// UNSURE: scenario_location_get_water_and_weather (0x53ed60, outside this module) is called with (&camera->leaf_index, 0)
//   on the stack and EBX = &observers[0].camera; whether EBX is an input is unknown.
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)
// reconciled: R17 sound_observer_camera -> camera.h observer_camera via observers[0].camera (same bytes)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "camera.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_time_globals *game_time;              // 0x006f1d6c
extern player_globals *local_player_globals;       // 0x0087a478
extern Globals *global_globals;                     // 0x00746fa0
extern observer observers[1];    // 0x006ac65c, camera.h; observers[0].camera is the 0x006ac6d0 row (R17);
                                                     // this function always reads slot 0
extern sound_listener sound_listeners[1];           // 0x00725218
extern SoundEnvironment sound_environment;          // 0x0072525c
extern sound_driver *current_sound_driver;          // 0x00725208, header calls this "sound_driver";
                                                     // renamed here, sound_driver is already the type

extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf,
    int16_t *weather_index_out); // 0x53ed60, EBX point, stack (leaf, weather_index_out)
extern datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index,
    sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint); // 0x549af0
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970, math module
extern void matrix4x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); // 0x4cc010, math module

extern const real_vector3d *global_forward3d_pointer; // 0x00696718
extern const real_vector3d *global_up3d_pointer;      // 0x00696720
extern const real_point3d *global_zero_vector3d_pointer;  // 0x006966f8, listener-space position is
                                                           // always the origin (sources are moved
                                                           // into listener space instead)
extern const real_point3d *global_origin3d_pointer;  // 0x00696714, zero velocity

// Recomputes the local player's listener orientation, position and velocity from the observer
// camera once per update (skipped while the game clock is stopped, or with no local player),
// triggers the enter/exit-water ambient sound on an underwater transition, and pushes the fixed
// (always-origin) listener parameters to the sound driver.
void sound_update_listener(void)
{
    uint8_t underwater;
    observer_camera *camera;
    sound_listener *listener;
    sound_listener_parameters params;
    datum_index water_sound_tag;

    listener = &sound_listeners[0];

    if (!game_time->initialized) {
        return;
    }
    if (!game_time->active && !game_time->paused) {
        return;
    }

    if (local_player_globals->local_players[0] == 0xffffffff) {
        listener->valid = 0;
        goto push_listener_parameters;
    }
    listener->valid = 1;

    camera = &observers[0].camera;
    // 0x54b9a3..0x54b9b7: EBX = 0x6ac6d0 (camera->position), push 0, push 0x6ac6dc (&camera->leaf_index)
    underwater = scenario_location_get_water_and_weather((real_point3d *)&camera->position,
        (bsp_leaf_reference *)&camera->leaf_index, (int16_t *)0);
    if (listener->underwater != underwater) {
        // Underwater state just changed: play the matg enter/exit water sound if one is assigned.
        // The stack block is a full 0x40-byte sound_location ([esp+0x38], the frame's last 0x40
        // bytes); only type, scale and gain are written (sound_play_new copies all 0x40 bytes).
        sound_location location = { 0 };
        location.type = _sound_location_none;
        location.scale = 1.0f;
        location.gain = 1.0f;

        if (underwater) {
            if (0 < global_globals->sounds.count) {
                water_sound_tag = *(datum_index *)&((GlobalsSound *)global_globals->sounds.pointer)[0].sound.tag_id;
                if (water_sound_tag != k_datum_index_none) {
                    sound_play_new(water_sound_tag, &location, k_datum_index_none, (sound_location_proc)0, (void *)0, 0, 1);
                }
            }
        } else {
            if (1 < global_globals->sounds.count) {
                water_sound_tag = *(datum_index *)&((GlobalsSound *)global_globals->sounds.pointer)[1].sound.tag_id;
                if (water_sound_tag != k_datum_index_none) {
                    sound_play_new(water_sound_tag, &location, k_datum_index_none, (sound_location_proc)0, (void *)0, 0, 1);
                }
            }
        }
    }
    listener->underwater = underwater;

    matrix4x3_from_forward_up((real_vector3d *)&camera->up, (real_vector3d *)&camera->forward,
        (real_matrix4x3 *)&listener->scale);

    listener->position = camera->position;

    matrix4x3_inverse_transform_vector((real_vector3d *)&listener->velocity, (real_vector3d *)&camera->velocity,
        (real_matrix4x3 *)&listener->scale);

push_listener_parameters:
    params.position = *(Point3D *)global_zero_vector3d_pointer;
    params.forward = *(Vector3D *)global_forward3d_pointer;
    params.up = *(Vector3D *)global_up3d_pointer;
    params.velocity = *(Vector3D *)global_origin3d_pointer;
    params.environment = &sound_environment;
    current_sound_driver->set_listener(&params);
}

#if 0
Original Ghidra decompilation (0x54b970):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void sound_update_listener(void)

{
  int iVar1;
  char cVar2;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 *local_44;
  undefined2 local_40 [2];
  undefined4 local_3c;
  undefined4 local_38;

  if (*DAT_006f1d6c == '\0') {
    return;
  }
  if ((DAT_006f1d6c[1] == '\0') && (DAT_006f1d6c[2] == '\0')) {
    return;
  }
  if (*(int *)(DAT_0087a478 + 4) == -1) {
    DAT_00725218 = 0;
    goto LAB_0054ba9e;
  }
  DAT_00725218 = 1;
  cVar2 = FUN_0053ed60(&DAT_006ac6dc,0);
  if (DAT_00725219 != cVar2) {
    local_40[0] = 0;
    local_3c = 0x3f800000;
    local_38 = 0x3f800000;
    if (cVar2 == '\0') {
      if (1 < *(int *)(DAT_00746fa0 + 0xf8)) {
        iVar1 = *(int *)(*(int *)(DAT_00746fa0 + 0xfc) + 0x1c);
        goto joined_r0x0054ba27;
      }
    }
    else if (0 < *(int *)(DAT_00746fa0 + 0xf8)) {
      iVar1 = *(int *)(*(int *)(DAT_00746fa0 + 0xfc) + 0xc);
joined_r0x0054ba27:
      if (iVar1 != -1) {
        FUN_00549af0(iVar1,local_40,0xffffffff,0,0,0,1);
      }
    }
  }
  DAT_00725219 = cVar2;
  matrix4x3_from_forward_up(&DAT_0072521c);
  _DAT_00725244 = DAT_006ac6d0;
  _DAT_0072524c = DAT_006ac6d8;
  _DAT_00725248 = DAT_006ac6d4;
  matrix4x3_inverse_transform_vector(&DAT_0072521c);
LAB_0054ba9e:
  local_68 = *(undefined4 *)PTR_DAT_00696718;
  local_64 = *(undefined4 *)(PTR_DAT_00696718 + 4);
  local_60 = *(undefined4 *)(PTR_DAT_00696718 + 8);
  local_5c = *(undefined4 *)PTR_DAT_00696720;
  local_58 = *(undefined4 *)(PTR_DAT_00696720 + 4);
  local_54 = *(undefined4 *)(PTR_DAT_00696720 + 8);
  local_74 = *(undefined4 *)PTR_DAT_006966f8;
  local_70 = *(undefined4 *)(PTR_DAT_006966f8 + 4);
  local_6c = *(undefined4 *)(PTR_DAT_006966f8 + 8);
  local_50 = *(undefined4 *)PTR_DAT_00696714;
  local_4c = *(undefined4 *)(PTR_DAT_00696714 + 4);
  local_48 = *(undefined4 *)(PTR_DAT_00696714 + 8);
  local_44 = &DAT_0072525c;
  (**(code **)(DAT_00725208 + 0xc))(&local_74);
  return;
}

Disassembly (0x54b970..0x54bb1e, capstone) resolving the implicit register arguments of
matrix4x3_from_forward_up and matrix4x3_inverse_transform_vector:

0x54ba46: mov eax, 0x6ac6fc
0x54ba4b: mov ecx, 0x6ac6f0
0x54ba56: call 0x4cb970
0x54ba7c: mov eax, 0x725250
0x54ba81: mov edx, 0x6ac6e4
0x54ba8c: call 0x4cc010

Disassembly (0x54b970..0x54bb1f, capstone; phase-4 review):

0x54b970: mov eax, dword ptr [0x6f1d6c]
0x54b975: mov cl, byte ptr [eax]
0x54b977: sub esp, 0x74
0x54b97a: test cl, cl
0x54b97c: je 0x54bb1b
0x54b982: mov cl, byte ptr [eax + 1]
0x54b985: test cl, cl
0x54b987: jne 0x54b994
0x54b989: mov cl, byte ptr [eax + 2]
0x54b98c: test cl, cl
0x54b98e: je 0x54bb1b
0x54b994: mov eax, dword ptr [0x87a478]
0x54b999: cmp dword ptr [eax + 4], -1
0x54b99d: je 0x54ba97
0x54b9a3: push ebx
0x54b9a4: push 0
0x54b9a6: push 0x6ac6dc
0x54b9ab: mov ebx, 0x6ac6d0
0x54b9b0: mov byte ptr [0x725218], 1
0x54b9b7: call 0x53ed60
0x54b9bc: mov bl, al
0x54b9be: mov al, byte ptr [0x725219]
0x54b9c3: add esp, 8
0x54b9c6: cmp al, bl
0x54b9c8: je 0x54ba41
0x54b9ca: test bl, bl
0x54b9cc: mov eax, dword ptr [0x746fa0]
0x54b9d1: mov ecx, dword ptr [eax + 0xf8]
0x54b9d7: mov word ptr [esp + 0x38], 0
0x54b9de: mov dword ptr [esp + 0x3c], 0x3f800000
0x54b9e6: mov dword ptr [esp + 0x40], 0x3f800000
0x54b9ee: je 0x54ba13
0x54b9f0: test ecx, ecx
0x54b9f2: jle 0x54ba41
0x54b9f4: mov eax, dword ptr [eax + 0xfc]
0x54b9fa: mov eax, dword ptr [eax + 0xc]
0x54b9fd: cmp eax, -1
0x54ba00: je 0x54ba41
0x54ba02: push 1
0x54ba04: push 0
0x54ba06: push 0
0x54ba08: push 0
0x54ba0a: push -1
0x54ba0c: lea ecx, [esp + 0x4c]
0x54ba10: push ecx
0x54ba11: jmp 0x54ba38
0x54ba13: cmp ecx, 1
0x54ba16: jle 0x54ba41
0x54ba18: mov eax, dword ptr [eax + 0xfc]
0x54ba1e: add eax, 0x10
0x54ba21: mov eax, dword ptr [eax + 0xc]
0x54ba24: cmp eax, -1
0x54ba27: je 0x54ba41
0x54ba29: push 1
0x54ba2b: push 0
0x54ba2d: push 0
0x54ba2f: push 0
0x54ba31: push -1
0x54ba33: lea edx, [esp + 0x4c]
0x54ba37: push edx
0x54ba38: push eax
0x54ba39: call 0x549af0
0x54ba3e: add esp, 0x1c
0x54ba41: push 0x72521c
0x54ba46: mov eax, 0x6ac6fc
0x54ba4b: mov ecx, 0x6ac6f0
0x54ba50: mov byte ptr [0x725219], bl
0x54ba56: call 0x4cb970
0x54ba5b: mov eax, dword ptr [0x6ac6d0]
0x54ba60: mov edx, dword ptr [0x6ac6d8]
0x54ba66: mov ecx, dword ptr [0x6ac6d4]
0x54ba6c: mov dword ptr [0x725244], eax
0x54ba71: mov dword ptr [0x72524c], edx
0x54ba77: push 0x72521c
0x54ba7c: mov eax, 0x725250
0x54ba81: mov edx, 0x6ac6e4
0x54ba86: mov dword ptr [0x725248], ecx
0x54ba8c: call 0x4cc010
0x54ba91: add esp, 8
0x54ba94: pop ebx
0x54ba95: jmp 0x54ba9e
0x54ba97: mov byte ptr [0x725218], 0
0x54ba9e: mov eax, dword ptr [0x696718]
0x54baa3: mov ecx, dword ptr [eax]
0x54baa5: mov dword ptr [esp + 0xc], ecx
0x54baa9: mov edx, dword ptr [eax + 4]
0x54baac: mov ecx, dword ptr [0x696720]
0x54bab2: mov dword ptr [esp + 0x10], edx
0x54bab6: mov eax, dword ptr [eax + 8]
0x54bab9: mov dword ptr [esp + 0x14], eax
0x54babd: mov edx, dword ptr [ecx]
0x54babf: mov dword ptr [esp + 0x18], edx
0x54bac3: mov eax, dword ptr [ecx + 4]
0x54bac6: mov edx, dword ptr [0x6966f8]
0x54bacc: mov dword ptr [esp + 0x1c], eax
0x54bad0: mov ecx, dword ptr [ecx + 8]
0x54bad3: mov dword ptr [esp + 0x20], ecx
0x54bad7: mov eax, dword ptr [edx]
0x54bad9: mov dword ptr [esp], eax
0x54badc: mov ecx, dword ptr [edx + 4]
0x54badf: mov eax, dword ptr [0x696714]
0x54bae4: mov dword ptr [esp + 4], ecx
0x54bae8: mov edx, dword ptr [edx + 8]
0x54baeb: mov dword ptr [esp + 8], edx
0x54baef: mov ecx, dword ptr [eax]
0x54baf1: mov dword ptr [esp + 0x24], ecx
0x54baf5: mov edx, dword ptr [eax + 4]
0x54baf8: mov dword ptr [esp + 0x28], edx
0x54bafc: mov eax, dword ptr [eax + 8]
0x54baff: mov edx, dword ptr [0x725208]
0x54bb05: lea ecx, [esp]
0x54bb08: push ecx
0x54bb09: mov dword ptr [esp + 0x30], eax
0x54bb0d: mov dword ptr [esp + 0x34], 0x72525c
0x54bb15: call dword ptr [edx + 0xc]
0x54bb18: add esp, 4
0x54bb1b: add esp, 0x74
0x54bb1e: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
