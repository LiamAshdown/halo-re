// game_engine_digitize_control_input  (Ghidra: FUN_00472760; renamed, no established name)
// address 0x472760, size 543 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x472760..0x47297e: every flag bit, field offset and both edge-detection branches.)
// evidence: out/phase4/game_functions.md ("Converts a raw unit-control input structure into
// digitized action-flag bits accumulated in the global local-player control header");
// types/game.h player_action_flags enum, whose own comment names this exact function
// ("The bits 0x472760 sets in player_control_globals::action_flags from a unit_control_data")
// and gives the source field/offset for every bit -- that mapping is used directly below in
// preference to re-deriving it. types/game.h player_control_globals (action_flags +0x00,
// action_flags_latched +0x04, action_flags_edge +0x08).
// register convention: the raw input record pointer is Ghidra's `in_EDX`.
//   // blam-cc: EDX -> input
// CORRECTED (phase 4 review): the input record is types/game.h player_control_input, the record
// game_engine_build_local_player_control_input (0x4710b0) builds and passes in EDX, so the raw
// byte offsets this file used are now field accesses. It is NOT a types/units.h
// unit_control_data, whose throttle starts at +0x0c rather than +0x00; the player_action_flags
// comments in types/game.h that named unit_control_data fields were corrected to match.
// CORRECTED (phase 4 review): the second edge-detection block in each of the two branches used to
// clear the input control flag unconditionally once action_flags_latched had its bit clear. The
// disassembly skips that clear entirely when action_flags_edge also has the bit clear
// (objdump 0x4728f1 "je 0x472907" and 0x472940 "je 0x472956", both jumping past the shared
// "and DWORD PTR [edx+0x18],esi" at 0x472904 / 0x472953). Ghidra shows this as a goto past the
// shared clear, which the previous rewrite dropped; the two clear_button flags below restore it.
// UNSURE: what the crouch / exchange-weapon edge-detection tail is for is still not established;
// only its exact control flow is.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern uint8_t game_engine_input_source_flag; // 0x006887a8, UNSURE exact meaning (gates two
                                               // near-identical branches below)
extern uint8_t unknown_007124a0;   // 0x007124a0 == local_player_input_states[0].buttons[8]
extern uint8_t game_state_write_in_progress;    // UNSURE raw flag
extern uint8_t *cinematic_globals_ptr;    // UNSURE raw pointer (offset +10 tested)
extern uint16_t split_screen_quit_prompt_string;     // UNSURE raw value (low 16 bits set to -1)
extern uint8_t network_join_error_reason;       // UNSURE raw flag
extern uint8_t unknown_0071973b;        // UNSURE raw flag

// blam-cc: EDX -> input
// See the header: sets player_action_flags bits in player_control_globals_ptr->action_flags per
// the exact mapping types/game.h's player_action_flags enum documents for this function, then
// runs a crouch/exchange-weapon edge-detection pass against action_flags_latched/action_flags_edge.
void game_engine_digitize_control_input(player_control_input *input)
{
    uint32_t *flags = &player_control_globals_ptr->action_flags;
    uint32_t control_flags = input->control_flags;
    uint32_t button_flags = input->button_flags;

    if ((input->melee != 0 || unknown_007124a0 != 0) && game_state_write_in_progress == 0 &&
        *(int8_t *)(cinematic_globals_ptr + 10) != 0) {
        split_screen_quit_prompt_string = 0xffff;
        network_join_error_reason = 0;
        unknown_0071973b = 1;
    }

    if (control_flags & 0x40) { *flags |= _player_action_jump; }
    if (control_flags & 0x02) { *flags |= _player_action_flashlight; }
    if (input->action != 0) { *flags |= _player_action_action; }
    if (input->melee != 0) { *flags |= _player_action_melee; }
    if (input->primary_trigger > 0.0f) { *flags |= _player_action_primary_trigger; }
    if (control_flags & 0x2000) { *flags |= _player_action_reload; }
    if (button_flags & 0x04) { *flags |= _player_action_exchange_weapon; }

    if (input->pitch_delta > 0.0f) { *flags |= _player_action_look_up; }
    else if (input->pitch_delta < 0.0f) { *flags |= _player_action_look_down; }

    if (input->yaw_delta > 0.0f) { *flags |= _player_action_look_left; }
    else if (input->yaw_delta < 0.0f) { *flags |= _player_action_look_right; }

    if (input->throttle_x > 0.0f) { *flags |= _player_action_forward; }
    else if (input->throttle_x < 0.0f) { *flags |= _player_action_backward; }

    if (input->throttle_y > 0.0f) { *flags |= _player_action_left; }
    else if (input->throttle_y < 0.0f) { *flags |= _player_action_right; }

    // Edge-detection tail. Exact control flow: see the two CORRECTED notes in the header.
    {
        uint32_t *latched = &player_control_globals_ptr->action_flags_latched;
        uint32_t *edge = &player_control_globals_ptr->action_flags_edge;

        if ((*latched & 1) == 0) {
            uint32_t v = *edge;

            if (v & 1) {
                if ((input->control_flags & 0x40) == 0) {
                    v = v & ~1u;
                } else {
                    v = v | 1;
                }
                *edge = v;
                input->control_flags &= ~0x40u;
            }
        } else {
            input->control_flags &= ~0x40u;
        }

        if (game_engine_input_source_flag != 0) {
            int32_t clear_button = 1;

            if ((*latched & 4) == 0) {
                uint32_t v = *edge;

                if ((v & 4) == 0) {
                    clear_button = 0; // objdump 0x472940 jumps past the shared clear
                } else {
                    if ((input->control_flags & 2) == 0) {
                        v = v & ~4u;
                    } else {
                        v = v | 4;
                    }
                    *edge = v;
                }
            }
            if (clear_button) {
                input->control_flags &= ~2u;
            }

            if ((*latched & 8) == 0) {
                uint32_t v = *edge;

                if ((v & 4) == 0) {
                    return;
                }
                if (input->button_flags & 2) {
                    *edge = v | 4;
                    input->button_flags &= ~2u;
                    return;
                }
                *edge = v & ~4u;
            }
            input->button_flags &= ~2u;
            return;
        }

        {
            int32_t clear_button = 1;

            if ((*latched & 4) == 0) {
                uint32_t v = *edge;

                if ((v & 4) == 0) {
                    clear_button = 0; // objdump 0x4728f1 jumps past the shared clear
                } else {
                    if ((input->control_flags & 0x40) == 0) {
                        v = v & ~4u;
                    } else {
                        v = v | 4;
                    }
                    *edge = v;
                }
            }
            if (clear_button) {
                input->control_flags &= ~0x40u;
            }
        }

        if ((*latched & 8) == 0) {
            uint32_t v = *edge;

            if ((v & 8) == 0) {
                return;
            }
            if (input->button_flags & 1) {
                *edge = v | 8;
                input->button_flags &= ~1u;
                return;
            }
            *edge = v & ~8u;
        }
        input->button_flags &= ~1u;
    }
}

#if 0
Original Ghidra decompilation (0x472760), from tools/pack.py 0x472760:

void FUN_00472760(void)

{
  uint *puVar1;
  uint uVar2;
  float *in_EDX;

  puVar1 = DAT_006b145c;
  if ((((*(char *)((int)in_EDX + 0x15) != '\0') || (DAT_007124a0 != '\0')) && (DAT_006e3000 == '\0')
      ) && (*(char *)(DAT_006f187c + 10) != '\0')) {
    DAT_00719754._0_2_ = 0xffff;
    DAT_0071973c = 0;
    DAT_0071973b = 1;
  }
  if (((uint)in_EDX[6] & 0x40) != 0) {
    *DAT_006b145c = *DAT_006b145c | 1;
  }
  if (((uint)in_EDX[6] & 2) != 0) {
    *puVar1 = *puVar1 | 2;
  }
  if (*(char *)(in_EDX + 5) != '\0') {
    *puVar1 = *puVar1 | 4;
  }
  if (*(char *)((int)in_EDX + 0x15) != '\0') {
    *puVar1 = *puVar1 | 8;
  }
  if (0.0 < in_EDX[2]) {
    *puVar1 = *puVar1 | 0x10;
  }
  if (((uint)in_EDX[6] & 0x2000) != 0) {
    *puVar1 = *puVar1 | 0x20;
  }
  if (((uint)in_EDX[7] & 4) != 0) {
    *puVar1 = *puVar1 | 0x40;
  }
  if (in_EDX[4] <= 0.0) {
    if (in_EDX[4] < 0.0) {
      uVar2 = *puVar1 | 0x100;
      goto LAB_0047281f;
    }
  }
  else {
    uVar2 = *puVar1 | 0x80;
LAB_0047281f:
    *puVar1 = uVar2;
  }
  if (in_EDX[3] <= 0.0) {
    if (in_EDX[3] < 0.0) {
      uVar2 = *puVar1 | 0x400;
      goto LAB_00472851;
    }
  }
  else {
    uVar2 = *puVar1 | 0x200;
LAB_00472851:
    *puVar1 = uVar2;
  }
  if (*in_EDX <= 0.0) {
    if (*in_EDX < 0.0) {
      uVar2 = *puVar1 | 0x1000;
      goto LAB_00472881;
    }
  }
  else {
    uVar2 = *puVar1 | 0x800;
LAB_00472881:
    *puVar1 = uVar2;
  }
  if (in_EDX[1] <= 0.0) {
    if (in_EDX[1] < 0.0) {
      uVar2 = *puVar1 | 0x4000;
      goto LAB_004728b3;
    }
  }
  else {
    uVar2 = *puVar1 | 0x2000;
LAB_004728b3:
    *puVar1 = uVar2;
  }
  if ((puVar1[1] & 1) == 0) {
    uVar2 = puVar1[2];
    if ((uVar2 & 1) != 0) {
      if (((uint)in_EDX[6] & 0x40) == 0) {
        uVar2 = uVar2 & 0xfffffffe;
      }
      else {
        uVar2 = uVar2 | 1;
      }
      puVar1[2] = uVar2;
      goto LAB_004728d9;
    }
  }
  else {
LAB_004728d9:
    in_EDX[6] = (float)((uint)in_EDX[6] & 0xffffffbf);
  }
  if (DAT_006887a8 != '\0') {
    if ((puVar1[1] & 4) == 0) {
      uVar2 = puVar1[2];
      if ((uVar2 & 4) == 0) goto LAB_00472956;
      if (((uint)in_EDX[6] & 2) == 0) {
        uVar2 = uVar2 & 0xfffffffb;
      }
      else {
        uVar2 = uVar2 | 4;
      }
      puVar1[2] = uVar2;
    }
    in_EDX[6] = (float)((uint)in_EDX[6] & 0xfffffffd);
LAB_00472956:
    if ((puVar1[1] & 8) == 0) {
      uVar2 = puVar1[2];
      if ((uVar2 & 4) == 0) {
        return;
      }
      if (((uint)in_EDX[7] & 2) != 0) {
        puVar1[2] = uVar2 | 4;
        in_EDX[7] = (float)((uint)in_EDX[7] & 0xfffffffd);
        return;
      }
      puVar1[2] = uVar2 & 0xfffffffb;
    }
    in_EDX[7] = (float)((uint)in_EDX[7] & 0xfffffffd);
    return;
  }
  if ((puVar1[1] & 4) == 0) {
    uVar2 = puVar1[2];
    if ((uVar2 & 4) == 0) goto LAB_00472907;
    if (((uint)in_EDX[6] & 0x40) == 0) {
      uVar2 = uVar2 & 0xfffffffb;
    }
    else {
      uVar2 = uVar2 | 4;
    }
    puVar1[2] = uVar2;
  }
  in_EDX[6] = (float)((uint)in_EDX[6] & 0xffffffbf);
LAB_00472907:
  if ((puVar1[1] & 8) == 0) {
    uVar2 = puVar1[2];
    if ((uVar2 & 8) == 0) {
      return;
    }
    if (((uint)in_EDX[7] & 1) != 0) {
      puVar1[2] = uVar2 | 8;
      in_EDX[7] = (float)((uint)in_EDX[7] & 0xfffffffe);
      return;
    }
    puVar1[2] = uVar2 & 0xfffffff7;
  }
  in_EDX[7] = (float)((uint)in_EDX[7] & 0xfffffffe);
  return;
}
#endif
