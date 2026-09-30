// sound_schedule_gain_fade  (Ghidra: FUN_0054af60, still unnamed there)
// address 0x54af60, size 233 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Schedules a smooth gain transition on one or two
// pitch-range playback slots over a short time window."; fields written match types/sound.h
// sound.fade_curve/fade_start_gain/fade_end_gain/fade_start_time/fade_end_time (0x92/0x9c/0xa0/
// 0xa4/0xa8). Callers (0x54bd60, 0x54deb0, and 0x549ee0/0x5495f0/0x549fa0 in the other half of
// this module) pass literal float bit patterns (0x40000000=2.0, 0x3f000000=0.5, 0x3e99999a=0.3)
// for the duration argument.
// register convention: EBX -> fade_in_handle (the slot that fades to full gain, or
// k_datum_index_none for none), stack -> (fade_curve, duration_seconds, fade_out_handle (the
// slot that fades to silence, or k_datum_index_none for none)).
//
// Ghidra's decompilation shows the end-time computation as a bare, argument-less `__ftol()`
// call; disassembly (scratchpad/disasm/disasm.py, capstone) resolves the lost FPU dataflow:
//   fld [esp+0xc]            ; duration_seconds (the real param_2, after the prologue's `push ecx`
//                             ; shifts the stack-relative offset from +8 to +0xc)
//   fmul [0x672ae8]           ; * 1000.0 (confirmed by reading the float bits at that address)
//   fiadd [esp+8]             ; + (float)(sound_time - 1)
//   call __ftol               ; -> candidate_end_time
//   candidate_end_time = max(candidate_end_time, sound_time)
// i.e. the end time is `sound_time - 1 + duration_seconds*1000`, floored at `sound_time` (so a
// duration of 0 or a computed time no later than "now" collapses to an instantaneous fade).
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern data_array *sound_data;              // 0x007252c0, "sounds" 0x200 x 0xb0
extern int32_t sound_time;                  // 0x0072520c
extern float sound_fade_duration_scale;     // 0x00672ae8, 1000.0 (seconds -> milliseconds)


// blam-cc: EBX -> fade_in_handle, stack -> (fade_curve, duration_seconds, fade_out_handle)
// Starts a `duration_seconds`-long gain fade on up to two sound instances: `fade_in_handle`
// ramps from wherever its current fade left off (or from silence if it was not already fading)
// up to full gain, and `fade_out_handle` ramps from wherever its current fade left off down to
// silence. Either handle may be k_datum_index_none to skip that side.
void sound_schedule_gain_fade(datum_index fade_in_handle, int16_t fade_curve, float duration_seconds,
    datum_index fade_out_handle)
{
    int32_t fade_start_time;
    int32_t fade_end_time;
    sound *instance;

    fade_start_time = sound_time - 1;
    fade_end_time = (int32_t)(duration_seconds * sound_fade_duration_scale + (float)fade_start_time);
    if (fade_end_time <= sound_time) {
        fade_end_time = sound_time;
    }

    if (fade_in_handle != k_datum_index_none) {
        instance = (sound *)((uint8_t *)sound_data->data + (fade_in_handle & 0xffff) * sizeof(sound));
        if (instance->fade_start_time == instance->fade_end_time) {
            instance->fade_start_gain = 0.0f;
        } else {
            instance->fade_start_gain = sound_evaluate_fade_gain(fade_in_handle);
        }
        instance->fade_end_gain = 1.0f;
        instance->fade_curve = fade_curve;
        instance->fade_start_time = fade_start_time;
        instance->fade_end_time = fade_end_time;
    }

    if (fade_out_handle != k_datum_index_none) {
        instance = (sound *)((uint8_t *)sound_data->data + (fade_out_handle & 0xffff) * sizeof(sound));
        instance->fade_start_gain = sound_evaluate_fade_gain(fade_out_handle);
        instance->fade_end_gain = 0.0f;
        instance->fade_curve = fade_curve;
        instance->fade_start_time = fade_start_time;
        instance->fade_end_time = fade_end_time;
    }
}

#if 0
Original Ghidra decompilation (0x54af60):

void FUN_0054af60(undefined2 param_1,undefined4 param_2,uint param_3)

{
  int iVar1;
  int iVar2;
  uint unaff_EBX;
  int iVar3;
  int iVar4;
  float10 fVar5;

  iVar3 = DAT_0072520c;
  iVar1 = DAT_0072520c + -1;
  iVar2 = __ftol();
  if (iVar2 <= iVar3) {
    iVar2 = iVar3;
  }
  if (unaff_EBX != 0xffffffff) {
    iVar3 = (unaff_EBX & 0xffff) * 0xb0;
    iVar4 = iVar3 + *(int *)(DAT_007252c0 + 0x34);
    if (*(int *)(iVar4 + 0xa4) == *(int *)(iVar3 + 0xa8 + *(int *)(DAT_007252c0 + 0x34))) {
      *(undefined4 *)(iVar4 + 0x9c) = 0;
    }
    else {
      fVar5 = (float10)FUN_0054e3c0();
      *(float *)(iVar4 + 0x9c) = (float)fVar5;
    }
    *(undefined4 *)(iVar4 + 0xa0) = 0x3f800000;
    *(undefined2 *)(iVar4 + 0x92) = param_1;
    *(int *)(iVar4 + 0xa4) = iVar1;
    *(int *)(iVar4 + 0xa8) = iVar2;
  }
  if (param_3 != 0xffffffff) {
    iVar3 = (param_3 & 0xffff) * 0xb0 + *(int *)(DAT_007252c0 + 0x34);
    fVar5 = (float10)FUN_0054e3c0();
    *(float *)(iVar3 + 0x9c) = (float)fVar5;
    *(undefined4 *)(iVar3 + 0xa0) = 0;
    *(undefined2 *)(iVar3 + 0x92) = param_1;
    *(int *)(iVar3 + 0xa4) = iVar1;
    *(int *)(iVar3 + 0xa8) = iVar2;
  }
  return;
}

Disassembly (0x54af60..0x54b048, capstone), resolving the __ftol() dataflow:

0x54af60: push ecx
0x54af61: fld dword ptr [esp + 0xc]
0x54af65: push ebp
0x54af66: fmul dword ptr [0x672ae8]
0x54af6c: push esi
0x54af6d: mov esi, dword ptr [0x72520c]
0x54af73: lea ebp, [esi - 1]
0x54af76: mov dword ptr [esp + 8], ebp
0x54af7a: fiadd dword ptr [esp + 8]
0x54af7e: push edi
0x54af7f: call 0x6391b4
0x54af84: mov edi, eax
0x54af86: cmp edi, esi
0x54af88: jg 0x54af8c
0x54af8a: mov edi, esi
#endif
