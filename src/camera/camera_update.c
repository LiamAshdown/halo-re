// camera_update  (Ghidra: camera_update, already named)
// address 0x445640, size 554 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: top-level per-frame camera update. Field mapping (director_globals, director,
// observer_command, observer) confirmed field-for-field against types/camera.h. Ghidra's own
// decompile is structurally sound here; this rewrite is checked against objdump for the parts
// it renders ambiguously (the stack observer_command's exact byte offsets and the jump table).
// register convention: __cdecl, one float stack parameter (dt).
// review fix (phase 4 gate): the three director calls now carry their register arguments from
//   objdump (0x445676..0x445687 ESI = &input, pushed local player 0; 0x4456ad DI = 0 and the
//   mode_changed byte; 0x44576e AX = 0, CL = mode_changed). Previously the stack camera_input
//   handed to the pov procedure was never initialised.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"
#include "fn_camera.h"

extern uint8_t controls_input_capture_flags;                    // 0x00712542
extern player_globals *local_player_globals;        // 0x0087a478
extern uint8_t director_camera_switching;           // 0x00686a98
extern director_globals camera_director_globals;    // 0x006ac558
extern director directors[1];                       // 0x006ac560
extern director_pov_proc director_last_pov_proc;    // 0x006f17f8
extern observer observers[1];                       // 0x006ac65c


// blam-cc: DI -> local_player_index, stack -> reset

// blam-cc: AX -> local_player_index, CL -> force

// blam-cc: stack -> local_player_index, ESI -> input; result in AL

                                                // the result is stored to a dead stack slot

// Top-level per-frame camera update: reads input, applies pending mode changes, computes the
// active camera's point of view via its mode function pointer, and blends it into the observer.
void camera_update(float dt)
{
    camera_input input;
    observer_command command;

    director_camera_switching = (controls_input_capture_flags == 1);
    camera_director_globals.dt = dt;

    if (local_player_globals->local_players[0] == k_datum_index_none) {
        return;
    }

    directors[0].suppress_look_update = 0;
    directors[0].look_input_consumed = 0;
    director_build_camera_input(0, &input); // 0x445676 push 0 / lea esi,[esp+0x7c]

    switch (camera_director_globals.mode) {
    case _director_camera_mode_following:
    case _director_camera_mode_orbiting:
        director_choose_gameplay_camera(0, camera_director_globals.mode_changed); // 0x4456ad xor edi,edi
        break;
    case _director_camera_mode_flying:
        director_set_flying_camera(0, camera_director_globals.mode_changed); // 0x44576e cl = mode_changed, eax = 0
        break;
    case _director_camera_mode_first_person:
        if (camera_director_globals.mode_changed != 0) {
            directors[0].data.first_person.field_of_view = 0.0f; // 0x445791 dword store
            directors[0].pov_proc = camera_first_person_compute_pov;
            directors[0].look_scale = 1.0f;
            directors[0].unknown_c0 = 0;
        }
        break;
    default:
        break; // mode 3 (editor) has no case in the original jump table
    }

    camera_director_globals.mode_changed = 0;

    {
        uint8_t *zero = (uint8_t *)&command;
        int i;
        for (i = 0; i < (int)sizeof(command); i++) {
            zero[i] = 0;
        }
    }

    if (directors[0].pov_proc != (director_pov_proc)0 &&
        (directors[0].pov_proc != camera_debug_compute_pov ||
         local_player_globals->local_players[0] != k_datum_index_none)) {
        directors[0].pov_proc(&directors[0].data, &input, &command);
    }
    director_last_pov_proc = directors[0].pov_proc;

    if ((command.flags & 1) == 0) {
        directors[0].command.flags &= ~1u;
    } else {
        if (directors[0].transition_time != 0.0f) {
            if (directors[0].transition_time >= 0.2f || directors[0].pov_proc != camera_first_person_compute_pov) {
                if (command.timer <= directors[0].transition_time) {
                    command.timer = directors[0].transition_time;
                }
            } else {
                directors[0].transition_time = 0.0f;
                command.interpolation_flags[_observer_parameter_position] = 3;
                command.channel_times[_observer_parameter_position] = 0.0f;
                command.interpolation_flags[_observer_parameter_distance] = 3;
                command.channel_times[_observer_parameter_distance] = 0.0f;
            }
            directors[0].transition_time -= dt;
            if (directors[0].transition_time < 0.0f) {
                directors[0].transition_time = 0.0f;
            }
        }
        directors[0].command = command;
    }

    observers[0].updated = 0;
    observers[0].command = &directors[0].command;

    if (observers[0].has_command == 0) {
        directors[0].command.flags |= 8; // snap bit: the very first command ever always snaps
        directors[0].command.channel_times[0] = 0.0f;
        directors[0].command.channel_times[1] = 0.0f;
        directors[0].command.channel_times[2] = 0.0f;
        directors[0].command.channel_times[3] = 0.0f;
        directors[0].command.channel_times[4] = 0.0f;
        directors[0].command.timer = 0.0f;
        observers[0].has_command = 1;
    }
}

#if 0
Original Ghidra decompilation (0x445640):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void camera_update(float param_1)

{
  int iVar1;
  byte *pbVar2;
  undefined4 *puVar3;
  byte local_8c [72];
  float local_44;
  undefined1 local_40;
  undefined1 local_3e;
  undefined4 local_38;
  undefined4 local_30;
  undefined1 local_24 [36];

  DAT_00686a98 = DAT_00712542 == '\x01';
  DAT_006ac558 = param_1;
  if (*(int *)(DAT_0087a478 + 4) == -1) {
    return;
  }
  DAT_006ac5b1 = 0;
  DAT_006ac5b2 = 0;
  FUN_00445f90(0);
  switch(DAT_006ac55c) {
  case 0:
  case 1:
    FUN_00445dc0(DAT_006ac55e);
    break;
  case 2:
    FUN_00445f40();
    break;
  case 4:
    if (DAT_006ac55e != '\0') {
      _DAT_006ac56c = 0;
      DAT_006ac568 = camera_first_person_compute_pov;
      _DAT_006ac624 = 0x3f800000;
      DAT_006ac620 = 0;
      goto LAB_004456be;
    }
  }
LAB_004456be:
  DAT_006ac55e = 0;
  pbVar2 = local_8c;
  for (iVar1 = 0x1a; iVar1 != 0; iVar1 = iVar1 + -1) {
    pbVar2[0] = 0;
    pbVar2[1] = 0;
    pbVar2[2] = 0;
    pbVar2[3] = 0;
    pbVar2 = pbVar2 + 4;
  }
  if ((DAT_006ac568 != (code *)0x0) &&
     ((DAT_006ac568 != camera_debug_compute_pov || (*(int *)(DAT_0087a478 + 4) != -1)))) {
    (*DAT_006ac568)(&DAT_006ac56c,local_24,local_8c);
  }
  DAT_006f17f8 = DAT_006ac568;
  if ((local_8c[0] & 1) == 0) {
    DAT_006ac5b8 = DAT_006ac5b8 & 0xfffffffe;
  }
  else {
    if (DAT_006ac564 != 0.0) {
      if ((0.2 <= DAT_006ac564) || (DAT_006ac568 != camera_first_person_compute_pov)) {
        if (local_44 <= DAT_006ac564) {
          local_44 = DAT_006ac564;
        }
      }
      else {
        DAT_006ac564 = 0.0;
        local_38 = 0;
        local_40 = 3;
        local_30 = 0;
        local_3e = 3;
      }
      DAT_006ac564 = DAT_006ac564 - param_1;
      if (DAT_006ac564 < 0.0) {
        DAT_006ac564 = 0.0;
      }
    }
    pbVar2 = local_8c;
    puVar3 = &DAT_006ac5b8;
    for (iVar1 = 0x1a; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *(undefined4 *)pbVar2;
      pbVar2 = pbVar2 + 4;
      puVar3 = puVar3 + 1;
    }
  }
  DAT_006ac6cc = 0;
  _DAT_006ac660 = &DAT_006ac5b8;
  if (DAT_006ac6cd == '\0') {
    DAT_006ac5b8 = DAT_006ac5b8 | 8;
    _DAT_006ac60c = 0;
    _DAT_006ac610 = 0;
    _DAT_006ac614 = 0;
    _DAT_006ac618 = 0;
    DAT_006ac6cd = '\x01';
    _DAT_006ac600 = 0;
    _DAT_006ac61c = 0;
  }
  return;
}

Disassembly (objdump -d -M intel) resolving "local_44" (which Ghidra could not attribute) as
command.timer, and local_40/3e/38/30 as the position/distance channel snap fields:

0x4456e9: lea eax, [esp + 0x10]           ; &command  (3rd pov_proc arg)
0x4456ee: lea ecx, [esp + 0x7c]           ; &input    (2nd pov_proc arg)
0x4456f3: push 0x6ac56c                    ; &directors[0].data (1st pov_proc arg)
0x4456f8: call edx                          ; directors[0].pov_proc(...)
0x4457b6: fld dword ptr [esp + 0x58]        ; [esp+0x58] - [esp+0x10] == 0x48 == observer_command.timer
0x4457ba: fcomp dword ptr [0x6ac564]        ; compare against directors[0].transition_time
0x445754: mov dword ptr [esp + 0x64], 0     ; 0x64-0x10 = 0x54 == channel_times[0] (position)
0x44575c: mov byte ptr [esp + 0x5c], al     ; 0x5c-0x10 = 0x4c == interpolation_flags[0] (position), al == 3
0x445760: mov dword ptr [esp + 0x6c], 0     ; 0x6c-0x10 = 0x5c == channel_times[2] (distance)
0x445768: mov byte ptr [esp + 0x5e], al     ; 0x5e-0x10 = 0x4e == interpolation_flags[2] (distance)
#endif
