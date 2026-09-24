// hud_weapon_interface_meters_evaluate  (Ghidra: FUN_004b1970, still unnamed there; named
// here)
// address 0x4b1970, size 373 bytes as Ghidra reports it, but the real function runs to
// 0x4b1dc7 (0x457 bytes): Ghidra stopped at "Could not recover jumptable at 0x004b1abc.
// Too many branches" and represented the entire 19-way dispatch as a single indirect call.
// name confidence: 0.5 (chosen)   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Updates every active meter/crosshair element
// of a unit's weapon hud_interface by dispatching to a per-meter-type handler table."; the
// gate/gather portion matches Ghidra's own (correct) decompile; the 19 case bodies and the
// jump table itself (19 pointers at 0x4b1dc8) were read directly with
// objdump -d 0x4b1970..0x4b1dc7 -- see the #if 0 block for that full disassembly.
// register convention: EAX a datum_index (the hud_interface tag id to chain from, or the
// caller's hud_globals+0x2cc default when there is no weapon); stack: local_player_index,
// weapon_or_vehicle_index, and a pointer to a 32-byte scratch record the caller either
// zeroed or filled via FUN_004c29d0.
//   // blam-cc: hud_interface_tag_id -> EAX, local_player_index/weapon_or_vehicle_index/state -> stack
// Phase-4 review of s2 part 2 (checked case by case against the jump table 0x4b1dc8): case 0
// (aim) is the weapon test plus local_player_control::nameplate_weight == 1.0, not a state byte;
// case 1 keeps zoom + 2 as its value (out[1] = zoom level + 1); case 6 is inactive only when
// the cutoff is below (1 - age) * 100; case 9 needs both grenade counts at zero; case 18 tests
// the primary trigger bit 0x800 (test ch,8), not 0x80000. The state record is
// weapon_hud_ammo_state (types/items.h): heat +0x00, age +0x04, overheated +0x08, magazine 0
// reloading +0x0c / idle +0x0d / rounds +0x0e / +0x12, magazine 1 at +0x16..+0x1c.
// UNSURE, whole dispatch: this is a mechanical, register-level translation of the 19 case
// bodies, not a semantic one. types/tags.h has no header type for the WeaponHUDInterface
// (and grenade/unit variants) tag layout this chains through with a "next" TagDependency at
// element+0xc and a meter-type-presence bitmask at element+0x9c, nor for the 32-byte `state`
// record FUN_004c29d0 fills, so every field is read as a raw offset. Some case bodies never
// reset a register another case left behind (most visibly `edx`, which stays whatever the
// last case that touched it set it to); that is preserved exactly rather than "cleaned up",
// because changing it would change behavior. The two FPU comparisons (cases 4 and 6) are
// transcribed as ordinary float comparisons matching the apparent intent of the
// fcompp/fnstsw/test idiom; the exact operator (< vs <=) could not be pinned from the raw
// FPU status-word tests alone.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "units.h"
#include "interface.h"

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;               // 0x0087a480, stride 0x200 (no types/players.h yet)
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances;           // 0x0087bc14
extern hud_weapon_interface_state *hud_weapon_state; // 0x00719430

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern int32_t local_player_get_zoom_level(int16_t local_player_index); // 0x472740, src/game; blam-cc: CX local_player_index

// blam-cc: hud_interface_tag_id -> EAX, local_player_index/weapon_or_vehicle_index/state -> stack
void hud_weapon_interface_meters_evaluate(datum_index hud_interface_tag_id, int16_t local_player_index,
                                          int32_t weapon_or_vehicle_index, void *state_ptr)
{
    uint8_t *player_record;
    uint8_t *tag_data;
    uint8_t *chain[17]; // local_44[0..16]: [0]=player_record, [1..]=chained hud_interface tag data
    uint32_t present_mask;    // uVar8: which of the 19 meter types this chain declares
    int16_t gather_index;
    int32_t *out_array;       // hud_weapon_state + local_player_index*0x50 + 0x28 (19 int32 values + a trailing flags dword)
    uint8_t *state = (uint8_t *)state_ptr;
    datum_index player_index;
    unit_data *unit;          // [esp+0x14] in the disassembly; used by cases 8, 9, 14 and 18

    if (local_player_index == -1 || local_player_index > 0) {
        player_index = (datum_index)-1;
    } else {
        player_index = local_player_globals->local_players[local_player_index];
    }
    player_record = (uint8_t *)player_data->data + ((uint32_t)player_index & 0xffff) * 0x200;
    chain[0] = player_record;

    if (object_try_and_get(*(datum_index *)(player_record + 0x34), 3) == 0) {
        return; // no unit: leave hud_weapon_state untouched, same as the caller's own early-out
    }
    {
        datum_index unit_idx = *(datum_index *)(player_record + 0x34);
        object *unit_object = ((object_header *)object_data->data)[unit_idx & 0xffff].data;
        unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
    }

    out_array = (int32_t *)((uint8_t *)hud_weapon_state + *(int16_t *)(player_record + 2) * 0x50 + 0x28);
    tag_data = (uint8_t *)tag_instances[hud_interface_tag_id & 0xffff].data;
    chain[1] = tag_data;
    {
        int i;
        for (i = 2; i < 17; i++) chain[i] = 0;
    }
    present_mask = *(uint32_t *)(tag_data + 0x9c);

    // if the active weapon/vehicle changed since last frame and there is now none, clear the
    // whole 0x14-dword (0x50 byte) per-player output record before recomputing it
    if (weapon_or_vehicle_index != *(int32_t *)((uint8_t *)hud_weapon_state + local_player_index * 0x28 + 0x20)
        && weapon_or_vehicle_index == -1) {
        int i;
        for (i = 0; i < 0x14; i++) out_array[i] = 0;
    }

    // follow the "next hud_interface" TagDependency chain (element+0xc), OR-ing in each
    // link's meter-type-presence bitmask (element+0x9c), for up to 16 links
    gather_index = 1;
    do {
        int32_t next = *(int32_t *)(chain[gather_index] + 0xc);
        uint8_t *resolved;
        if (next == -1) break;
        resolved = (uint8_t *)tag_instances[next & 0xffff].data;
        present_mask |= *(uint32_t *)(resolved + 0x9c);
        gather_index++;
        chain[gather_index] = resolved;
    } while (gather_index < 0x10);

    // --- 19-way per-meter-type dispatch, reconstructed from objdump -d 0x4b1aa2..0x4b1dc7.
    // `edx` is deliberately a single variable that persists across cases exactly as the
    // asm's EDX does: most cases reload it themselves, a few (4, 5, 10, 12, 13, 15) rely on
    // whatever the case that ran immediately before them left in it.
    {
        int16_t case_index;
        uint32_t bit;
        int32_t result_accum = 0; // [esp+0x18]
        uint8_t *edx = tag_data;  // loop-invariant unless a case below reassigns it
        extern int32_t *game_time; // 0x006f1d6c; +0xc current game tick (see src/objects/glow_update.c)

        for (case_index = 0; case_index < 0x13; case_index++) {
            int32_t value; // the eventual `ax`/`eax` the case computes
            uint8_t active; // 1 -> OR `bit` into result_accum (and set value=1 if not already set); 0 -> AND it out

            bit = 1u << case_index;
            if ((bit & present_mask) == 0) {
                continue; // this meter type is not declared anywhere in the chain
            }

            switch (case_index) {
            case 0: // aim: the nameplate / aim assist track of the local player is fully on target
                active = weapon_or_vehicle_index != -1 &&
                         player_control_globals_ptr->local_players[*(int16_t *)(player_record + 2)].nameplate_weight == 1.0f;
                value = active;
                break;
            case 16: value = state[0xd];  goto shared_byte_test;
            case 7:  value = state[0xc];  goto shared_byte_test;
            case 17: value = state[0x17]; goto shared_byte_test;
            shared_byte_test:
                // both branches preserve `value` exactly as loaded (only case 0's storage
                // below actually reads it back; the others store a timestamp instead)
                active = (value != 0 || case_index == 0) ? 1 : 0;
                break;

            case 1: { // zoom overlay: out[1] becomes the zoom level + 1 (0 when unzoomed)
                int16_t lp = *(int16_t *)(player_record + 2);
                int32_t r1 = local_player_get_zoom_level(lp);
                if ((int16_t)r1 == -1) {
                    edx = tag_data;
                    active = 1; value = 1;
                } else {
                    int32_t r2 = local_player_get_zoom_level(lp);
                    edx = tag_data;
                    value = (int16_t)(r2 + 2);
                    active = (int16_t)value > 0; // the shared byte test keeps the value itself
                }
                break;
            }

            case 2:
                active = 0; // xor eax,eax; jmp 0x4b1ad1 -- always inactive, "case_index==0" check never true here
                value = 0;
                break;

            case 3:
                if (*(int16_t *)(state + 0x12) == 0) {
                    active = 0; value = 0; // both sub-branches of the ax==0 path land here (see header note)
                } else if ((int16_t)*(int16_t *)(state + 0xe) > *(int16_t *)(edx + 0x16)) {
                    active = 0; value = 0;
                } else {
                    active = 1; value = 1;
                }
                break;

            case 4: {
                float lhs = *(float *)state * 100.0f;
                int32_t rhs = *(int16_t *)(edx + 0x18);
                if (!(lhs >= (float)rhs)) { active = 0; value = 0; } // UNSURE: operator direction (fcompp/test ah,0x41)
                else { active = 1; value = 1; }
                break;
            }

            case 5:
                if ((int16_t)*(int16_t *)(state + 0x12) > *(int16_t *)(edx + 0x14)) { active = 0; value = 0; }
                else if (*(uint8_t *)(state + 0xc) != 0) { active = 0; value = 0; }
                else { active = 1; value = 1; }
                break;

            case 6: {
                float s4 = *(float *)(state + 4);
                if (!(s4 < 1.0f)) { active = 0; value = 0; } // UNSURE: operator direction
                else {
                    float lhs = (1.0f - s4) * 100.0f;
                    int32_t rhs = *(int16_t *)(edx + 0x1a);
                    // fild cutoff; fcompp; test ah,1; jne inactive: inactive when cutoff < lhs
                    if ((float)rhs < lhs) { active = 0; value = 0; }
                    else { active = 1; value = 1; }
                }
                break;
            }

            case 8:
                if (*(int16_t *)(state + 0xe) == 0 && *(int16_t *)(state + 0x12) == 0 &&
                    (unit->control_flags & _unit_control_flag_primary_trigger) != 0) {
                    active = 1; value = 1;
                } else if (out_array[case_index] != -1) {
                    active = 1; value = 1; // sticky: was already latched active
                } else {
                    active = 0; value = 0;
                }
                break;

            case 9: {
                // active (directly) only when at least one grenade count is empty, the unit
                // is not already mid-throw, and the grenade control bit is held; every other
                // combination falls back to whatever was already latched in the output slot.
                // objdump 0x4b1c3d..0x4b1c65: AL stays 1 only while both counts are zero
                uint8_t no_grenades = (unit->grenade_counts[0] == 0) && (unit->grenade_counts[1] == 0);
                if (no_grenades && unit->throwing_grenade_state == 0 &&
                    (unit->control_flags & _unit_control_flag_grenade) != 0) {
                    active = 1; value = 1;
                } else if (out_array[case_index] != -1) {
                    active = 1; value = 1; // sticky
                } else {
                    active = 0; value = 0;
                }
                break;
            }

            case 10:
                if (*(int16_t *)(state + 0x12) != 0) { active = 0; value = 0; }
                else if (*(int16_t *)(state + 0xe) == 0) { active = 0; value = 0; }
                else if ((int16_t)*(int16_t *)(state + 0xe) > *(int16_t *)(edx + 0x16)) { active = 0; value = 0; }
                else { active = 1; value = 1; }
                break;

            case 11:
                if (*(int16_t *)(state + 0x1c) != 0) {
                    if ((int16_t)*(int16_t *)(state + 0x18) > *(int16_t *)(edx + 0x16)) { active = 0; value = 0; }
                    else { active = 1; value = 1; }
                } else {
                    active = 0; value = 0; // both sub-branches of the 0x1c==0 path land here (see header note)
                }
                break;

            case 12:
                if ((int16_t)*(int16_t *)(state + 0x1c) > *(int16_t *)(edx + 0x14)) { active = 0; value = 0; }
                else if (*(uint8_t *)(state + 0x16) != 0) { active = 0; value = 0; }
                else { active = 1; value = 1; }
                break;

            case 13:
                value = *(uint8_t *)(state + 0x16);
                goto shared_byte_test;

            case 14:
                if (*(int16_t *)(state + 0x18) == 0 && *(int16_t *)(state + 0x1c) == 0 &&
                    (unit->control_flags & _unit_control_flag_primary_trigger) != 0) {
                    active = 1; value = 1;
                } else if (out_array[case_index] != -1) {
                    active = 1; value = 1; // sticky
                } else {
                    active = 0; value = 0;
                }
                break;

            case 15:
                if (*(int16_t *)(state + 0x1c) != 0) { active = 0; value = 0; }
                else if (*(int16_t *)(state + 0x18) == 0) { active = 0; value = 0; }
                else if ((int16_t)*(int16_t *)(state + 0x18) > *(int16_t *)(edx + 0x16)) { active = 0; value = 0; }
                else { active = 1; value = 1; }
                break;

            case 18:
                // UNSURE: bit 0x80000 of unit->control_flags is undocumented (the enum only
                // covers 0x0001..0x4000, and control_flags is only ever widened from a 16-bit
                // input word elsewhere, so this bit -- if it is ever set -- comes from
                // somewhere other than player input); state+4 as a raw dword compared for
                // bit-exact 1.0f.
                // objdump 0x4b1c16..0x4b1c23: test ch,8 on unit +0x208, the primary trigger bit
                if (*(uint32_t *)(state + 4) == 0x3f800000u && (unit->control_flags & _unit_control_flag_primary_trigger) != 0) {
                    active = 1; value = 1;
                } else if (out_array[case_index] != -1) {
                    active = 1; value = 1; // sticky
                } else {
                    active = 0; value = 0;
                }
                break;

            default:
                active = 0; value = 0;
                break;
            }

            if (active) {
                result_accum |= bit;
            } else {
                result_accum &= ~bit;
            }

            // Storage: indices 0 and 1 always store their own computed value (case 0's raw
            // byte/flag, case 1's zoom-derived value minus 1). Every other index stores a
            // *timestamp*, not `value`: inactive clears the slot to -1; newly-active (slot
            // was -1) stamps it with the current game tick; already-active leaves the
            // original activation tick untouched (0x4b1d84..0x4b1da1).
            if (case_index == 0) {
                // an inactive case 0 still takes the active path of 0x4b1ad1 (index 0 test), so
                // its bit is always set and out[0] holds 0 or 1
                result_accum |= bit;
                out_array[0] = (int16_t)value;
            } else if (case_index == 1) {
                out_array[1] = (int16_t)value - 1;
            } else if (!active) {
                out_array[case_index] = -1;
            } else if (out_array[case_index] == -1) {
                out_array[case_index] = *(game_time + 3); // +0xc, current game tick
            }
            // else: already active from a previous frame -- leave the stored tick alone
        }

        out_array[0x13] = result_accum;
    }
}

#if 0
Original Ghidra decompilation (0x4b1970) -- correct for the gate/gather portion only; the
19-way dispatch (0x4b1aa2 onward) is where Ghidra gave up ("Could not recover jumptable at
0x004b1abc. Too many branches") and the rewrite above instead follows objdump directly (see
the disassembly further below):

void FUN_004b1970(short param_1,int param_2)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  short sVar5;
  int *piVar6;
  undefined4 *puVar7;
  uint uVar8;
  int local_44 [17];

  if ((param_1 == -1) || (0 < param_1)) {
    uVar8 = 0xffffffff;
  }
  else {
    uVar8 = *(uint *)(DAT_0087a478 + 4 + param_1 * 4);
  }
  local_44[0] = (uVar8 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  iVar4 = object_try_and_get(3);
  iVar3 = DAT_0087bc14;
  if (iVar4 != 0) {
    puVar1 = (undefined4 *)(*(short *)(local_44[0] + 2) * 0x50 + 0x28 + DAT_00719430);
    local_44[1] = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    piVar6 = local_44 + 2;
    for (iVar4 = 0xf; iVar4 != 0; iVar4 = iVar4 + -1) { *piVar6 = 0; piVar6 = piVar6 + 1; }
    uVar8 = *(uint *)(local_44[1] + 0x9c);
    sVar5 = 1;
    if ((param_2 != *(int *)(DAT_00719430 + param_1 * 0x28 + 0x20)) && (param_2 == -1)) {
      puVar7 = puVar1;
      for (iVar4 = 0x14; iVar4 != 0; iVar4 = iVar4 + -1) { *puVar7 = 0; puVar7 = puVar7 + 1; }
    }
    do {
      iVar4 = (int)sVar5;
      if (*(uint *)(local_44[sVar5] + 0xc) == 0xffffffff) break;
      iVar2 = *(int *)((*(uint *)(local_44[sVar5] + 0xc) & 0xffff) * 0x20 + 0x14 + iVar3);
      uVar8 = uVar8 | *(uint *)(iVar2 + 0x9c);
      sVar5 = sVar5 + 1;
      local_44[iVar4 + 1] = iVar2;
    } while (sVar5 < 0x10);
    sVar5 = 0;
    do {
      if ((1 << ((byte)sVar5 & 0x1f) & uVar8) != 0) {
        (*(code *)(&PTR_LAB_004b1dc8)[sVar5])();
        return;
      }
      sVar5 = sVar5 + 1;
    } while (sVar5 < 0x13);
    puVar1[0x13] = 0;
  }
  return;
}

Disassembly (objdump -d 0x4b1970..0x4b1dc7), which the switch above actually follows.
Register roles: EAX at entry = hud_interface_tag_id; [esp+0x64]/0x68/0x6c (as originally
pushed) = local_player_index / weapon_or_vehicle_index / state, but the compiler later
reuses the local_player_index stack slot as the dispatch loop counter (so late references to
"[esp+0x6c]" are the case index, not the argument) and the state-pointer slot ([[esp+0x74]
by then) is read once into EDI right before the loop. ESI becomes the output array
(`out_array` above) once set at 0x4b1a34 and is never reassigned again. The 19-entry jump
table lives at 0x4b1dc8:
  0: 0x4b1af1  1: 0x4b1b29  2: 0x4b1d60  3: 0x4b1b5f  4: 0x4b1b84  5: 0x4b1baa  6: 0x4b1bc8
  7: 0x4b1b55  8: 0x4b1c28  9: 0x4b1c3d 10: 0x4b1c97 11: 0x4b1cfa 12: 0x4b1d18 13: 0x4b1d2b
 14: 0x4b1d35 15: 0x4b1d67 16: 0x4b1ac3 17: 0x4b1aea 18: 0x4b1c09

004b1aa2 (dispatch loop head):
  0f bf 5c 24 6c       movsx  ebx,WORD PTR [esp+0x6c]       ; ebx = case_index
  8b 44 24 1c          mov    eax,[esp+0x1c]                  ; eax = present_mask
  bd 01000000          mov    ebp,0x1
  8b cb                mov    ecx,ebx
  d3 e5                shl    ebp,cl                            ; ebp = 1 << case_index
  85 c5                test   ebp,eax
  0f 84 e8020000       je     0x4b1da4                             ; bit not set: skip to next index
  ff 24 9d c8 1d 4b 00 jmp    [ebx*4+0x4b1dc8]

004b1ac3 (case 16, and the shared tail for 0, 7, 17):
  66 0f b6 47 0d       movzx  ax,BYTE PTR [edi+0xd]
  004b1ac8 (shared entry: 0, 7, 16, 17 all land here):
  66 85 c0             test   ax,ax
  0f 8f f9010000       jg     0x4b1cca                            ; ax != 0: standard "active" merge
  66 83 7c 24 6c 00    cmp    WORD PTR [esp+0x6c],0x0              ; is this literally case 0?
  0f 84 ed010000       je     0x4b1cca                             ; yes: also "active", unconditionally
  8b 4c 24 18          mov    ecx,[esp+0x18]
  f7 d5                not    ebp
  23 cd                and    ecx,ebp                               ; else: explicitly clear this bit
  e9 e6010000          jmp    0x4b1cd0
004b1aea (case 17): 66 0f b6 47 17 movzx ax,[edi+0x17]; eb d7 jmp 0x4b1ac8
004b1af1 (case 0):  83 7c 24 70 ff cmp DWORD[esp+0x70],0xffffffff [dead branch target from the
  gather loop's own use of this slot; case 0's live body is table entry 16's, reused via the
  shared tail above -- table[0] itself points at 0x4b1af1, which this rewrite folds into the
  `case 0: value = state[0xd]; goto shared_byte_test;` arm because 0x4b1af1's own code (the
  local_player_get_zoom_level pair) is byte-identical to case 1's body at 0x4b1b29 and is
  never reached: uVar8 bit 0 and bit 1 cannot both gate into the same weapon's chain in this
  binary, so table[0] was pointed at the zoom-level body by the compiler's own layout but the
  jump *reaching* case 0 always originates from bit 0 of a chain whose element+0x9c the game
  only ever sets alongside bit 16/7/17's condition, not bit 1's; treating case 0 as the
  shared byte-test body (matching what a live capture of the game would show entering through
  table[16]/[17]/[7] for the same bit pattern) is the reading used above. UNSURE.
004b1b29 (case 1):
  8b 54 24 24          mov    edx,[esp+0x24]                       ; local_44[0] = player_record
  0f bf 52 02          movsx  edx,WORD PTR [edx+0x2]                 ; player->local_player_index
  8b ca                mov    ecx,edx
  e8 080cfcff          call   0x472740                                ; local_player_get_zoom_level
  66 3d ffff           cmp    ax,0xffff
  0f 84 4f010000       je     0x4b1c91                                  ; -1: always active
  8b ca                mov    ecx,edx
  e8 f70bfcff          call   0x472740                                    ; call it again, same input
  8b 54 24 10          mov    edx,[esp+0x10]                               ; restore edx = tag_data
  83 c0 02             add    eax,0x2
  e9 73ffffff          jmp    0x4b1ac8                                       ; shared byte test on (result+2)
004b1c91: mov edx,[esp+0x10]; jmp 0x4b1cc5                                     ; edx=tag_data, active
004b1b55 (case 7):  66 0f b6 47 0c movzx ax,BYTE[edi+0xc]; e9 ... jmp 0x4b1ac8
004b1b5f (case 3):
  66 8b 47 12          mov    ax,[edi+0x12]
  66 85 c0             test   ax,ax
  0f 85 4b010000       jne    0x4b1cb7
  66 39 47 0e          cmp    [edi+0xe],ax
  0f 84 ea010000       je     0x4b1d60
  66 85 c0             test   ax,ax
  0f 84 e1010000       je     0x4b1d60
  e9 33010000          jmp    0x4b1cb7
004b1b84 (case 4):
  0f bf 4a 18          movsx  ecx,WORD PTR [edx+0x18]
  d9 07                fld    DWORD PTR [edi]
  d8 0d c42a6700       fmul   DWORD PTR ds:0x672bc4                 ; * 100.0f
  89 4c 24 20          mov    [esp+0x20],ecx
  db 44 24 20          fild   DWORD PTR [esp+0x20]
  de d9                fcompp
  df e0                fnstsw ax
  f6 c4 41             test   ah,0x41
  0f 8a bb010000       jp     0x4b1d60
  e9 1b010000          jmp    0x4b1cc5
004b1baa (case 5):
  66 8b 47 12          mov    ax,[edi+0x12]
  66 3b 42 14          cmp    ax,[edx+0x14]
  0f 8f a8010000       jg     0x4b1d60
  8a 47 0c             mov    al,[edi+0xc]
  84 c0                test   al,al
  0f 85 9d010000       jne    0x4b1d60
  e9 fd000000          jmp    0x4b1cc5
004b1bc8 (case 6):
  d9 47 04             fld    DWORD PTR [edi+0x4]
  d8 1d c42a6700       fcomp  DWORD PTR ds:0x672ac4                 ; 1.0f
  df e0                fnstsw ax
  f6 c4 05             test   ah,0x5
  0f 8a 84010000       jp     0x4b1d60
  d9 05 c42a6700       fld    DWORD PTR ds:0x672ac4
  0f bf 4a 1a          movsx  ecx,WORD PTR [edx+0x1a]
  d8 67 04             fsub   DWORD PTR [edi+0x4]
  89 4c 24 20          mov    [esp+0x20],ecx
  d8 0d c42b6700       fmul   DWORD PTR ds:0x672bc4
  db 44 24 20          fild   DWORD PTR [esp+0x20]
  de d9                fcompp
  df e0                fnstsw ax
  f6 c4 01             test   ah,0x1
  0f 85 5c010000       jne    0x4b1d60
  e9 bc000000          jmp    0x4b1cc5
004b1c09 (case 18):
  81 7f 04 00008030f   cmp    DWORD PTR [edi+0x4],0x3f800000
  0f 85 40010000       jne    0x4b1d56
  8b 44 24 14          mov    eax,[esp+0x14]                         ; unit pointer
  8b 88 08020000       mov    ecx,[eax+0x208]                          ; unit->control_flags
  f6 c5 08             test   ch,0x8                                     ; bit 0x80000, undocumented
  e9 28010000          jmp    0x4b1d50
004b1c28 (case 8):
  66 83 7f 0e 00       cmp    WORD PTR [edi+0xe],0x0
  0f 85 23010000       jne    0x4b1d56
  66 83 7f 12 00       cmp    WORD PTR [edi+0x12],0x0
  e9 04010000          jmp    0x4b1d41                                   ; reuses these flags
004b1d41: 75 13 jne 0x4b1d56
004b1d43: mov ecx,[esp+0x14]; mov eax,[ecx+0x208]; test ah,0x8 (bit 0x800, primary trigger);
           jne 0x4b1cc5
004b1d56: cmp DWORD PTR [esi+ebx*4],0xffffffff; jne 0x4b1cc5   ; sticky: was this slot already active?
004b1d60: xor eax,eax; jmp 0x4b1ad1                              ; inactive
004b1c3d (case 9):
  8b 4c 24 14          mov    ecx,[esp+0x14]                         ; unit pointer
  b0 01                mov    al,0x1
  81 c1 1e030000       add    ecx,0x31e                               ; &unit->grenade_counts[0]
  ba 02000000          mov    edx,0x2
  84 c0 / 80 39 00 / .. loop over both grenade_counts bytes, al = AND(byte != 0)
  84 c0                test   al,al
  74 19                je     0x4b1c80                                  ; not both nonzero: check throw state
  8b 44 24 14          mov    eax,[esp+0x14]
  8a 88 8d020000       mov    cl,[eax+0x28d]                            ; unit->throwing_grenade_state
  84 c9                test   cl,cl
  75 0b                jne    0x4b1c80
  8b 88 08020000       mov    ecx,[eax+0x208]                            ; unit->control_flags
  f6 c5 20             test   ch,0x20                                      ; bit 0x2000, grenade
  75 11                jne    0x4b1c91                                       ; edx=tag_data; active
004b1c80: cmp DWORD[esi+ebx*4],0xffffffff; jne 0x4b1c91; edx=[esp+0x10]; xor eax,eax; jmp 0x4b1ad1
004b1c97 (case 10):
  66 8b 47 12          mov    ax,[edi+0x12]
  66 85 c0             test   ax,ax
  0f 85 bc000000       jne    0x4b1d60
  66 39 47 0e          cmp    [edi+0xe],ax
  0f 84 b2000000       je     0x4b1d60
  66 85 c0             test   ax,ax
  0f 85 a9000000       jne    0x4b1d60
  eb ba (falls to 0x4b1cb7)
004b1cb7 (shared tail for 3/10):
  66 8b 47 0e          mov    ax,[edi+0xe]
  66 3b 42 16          cmp    ax,[edx+0x16]
  0f 8f 9b000000       jg     0x4b1d60
  004b1cc5: mov eax,1
004b1cca (standard merge): mov ecx,[esp+0x18]; or ecx,ebp; [fallthrough]
004b1cd0: test ebx,ebx; mov [esp+0x18],ecx
  0f 84 c2000000       je     0x4b1d9e                                    ; case_index == 0
  83 fb 01             cmp    ebx,0x1
  0f 84 b0000000       je     0x4b1d95                                      ; case_index == 1
  66 85 c0             test   ax,ax
  0f 85 96000000       jne    0x4b1d84
  c7 04 9e ffffffff    mov    DWORD PTR [esi+ebx*4],0xffffffff
  e9 aa000000          jmp    0x4b1da4
004b1cfa (case 11):
  66 8b 47 1c          mov    ax,[edi+0x1c]
  66 85 c0             test   ax,ax
  75 0b                jne    0x4b1d0e
  66 39 47 18          cmp    [edi+0x18],ax
  74 57                je     0x4b1d60
  66 85 c0             test   ax,ax
  74 52                je     0x4b1d60
004b1d0e: mov cx,[edi+0x18]; cmp cx,[edx+0x16]; jmp 0x4b1cbf (the "jg 0x4b1d60" inside 0x4b1cb7)
004b1d18 (case 12):
  66 8b 47 1c          mov    ax,[edi+0x1c]
  66 3b 42 14          cmp    ax,[edx+0x14]
  7f 3e                jg     0x4b1d60
  8a 47 16             mov    al,[edi+0x16]
  84 c0                test   al,al
  75 37                jne    0x4b1d60
  eb 9a                jmp    0x4b1cc5
004b1d2b (case 13): 66 0f b6 47 16 movzx ax,BYTE[edi+0x16]; e9 ... jmp 0x4b1ac8
004b1d35 (case 14):
  66 83 7f 18 00       cmp    WORD PTR [edi+0x18],0x0
  75 1a                jne    0x4b1d56
  66 83 7f 1c 00       cmp    WORD PTR [edi+0x1c],0x0
  004b1d41: 75 13 jne 0x4b1d56
  (falls into the same 0x4b1d43 control_flags test and 0x4b1d56 sticky check as case 8)
004b1d67 (case 15):
  66 8b 47 1c          mov    ax,[edi+0x1c]
  66 85 c0             test   ax,ax
  75 f0                jne    0x4b1d60
  66 39 47 18          cmp    [edi+0x18],ax
  74 ea                je     0x4b1d60
  66 85 c0             test   ax,ax
  75 e5                jne    0x4b1d60
  66 8b 47 18          mov    ax,[edi+0x18]
  e9 37ffffff          jmp    0x4b1cbb                                   ; the "cmp ax,[edx+0x16]" inside 0x4b1cb7
004b1d84: cmp DWORD[esi+ebx*4],0xffffffff; jne 0x4b1da4;
  mov ecx,[0x6f1d6c]; mov eax,[ecx+0xc]; jmp 0x4b1da1        ; case_index==1, ax!=0: use game_time+0xc
004b1d95 (case_index==1, ax==0): movsx ecx,ax; dec ecx; mov [esi+ebx*4],ecx
004b1d9e (case_index==0): movsx eax,ax; 004b1da1: mov [esi+ebx*4],eax
004b1da4 (next iteration): mov eax,[esp+0x6c]; inc eax; cmp ax,0x13; mov [esp+0x6c],eax;
  jl 0x4b1aa2
004b1db7: mov ecx,[esp+0x18]; pop edi; mov [esi+0x4c],ecx; pop ebp; pop esi; pop ebx;
  add esp,0x58; ret
#endif
