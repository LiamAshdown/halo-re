// sound_compute_class_gain  (Ghidra: sound_compute_class_gain, already named)
// address 0x54b100, size 116 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Looks up a sound class's base gain and scales it by
// the appropriate combination of master/music/effects gain sliders."; sound_class_gains
// (types/sound.h, 0x00746140, 51 entries of sound_class_gain) current_gain at +4; class values
// 0x2c/0x2e/0x2f/0x20/0x2d/0x13 match SoundClass scripted_dialog_player/scripted_dialog_other/
// scripted_dialog_force_unspatialized/music/scripted_effect/unit_dialog (types/tags.h SoundClass).
// register convention: AX -> sound_class.
// Phase-4 review: 0x00746140 is a pointer to the class gain table (the code loads it with
// `mov reg, [0x746140]` and indexes from there); the draft declared the table itself there.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern sound_class_gain *sound_class_gains; // 0x00746140 holds a pointer to the 51-entry table (mov eax, [0x746140])
extern float sound_ducking_gain;  // 0x007252a4, ramps toward 0.7 while dialog plays
extern float sound_music_gain;    // 0x007252a8
extern float sound_master_gain;   // 0x007252ac
extern float sound_effects_gain;  // 0x007252b0

// blam-cc: AX -> sound_class
// Combines a sound class's current fade gain with the appropriate gain sliders: the scripted
// dialog classes scale by master gain only, music scales by music * ducking * master, unit
// dialog and scripted effects scale by ducking * master (no category slider), and every other
// class scales by effects * ducking * master.
float sound_compute_class_gain(SoundClass_t sound_class)
{
    float base_gain;

    base_gain = sound_class_gains[sound_class].current_gain;

    if (sound_class == soundclass_scripted_dialog_player || sound_class == soundclass_scripted_dialog_other ||
        sound_class == soundclass_scripted_dialog_force_unspatialized) {
        return base_gain * sound_master_gain;
    }
    if (sound_class == soundclass_music) {
        return sound_music_gain * sound_ducking_gain * sound_master_gain * base_gain;
    }
    if (sound_class == soundclass_scripted_effect || sound_class == soundclass_unit_dialog) {
        return sound_ducking_gain * sound_master_gain * base_gain;
    }
    return sound_effects_gain * sound_ducking_gain * sound_master_gain * base_gain;
}

#if 0
Original Ghidra decompilation (0x54b100):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 sound_compute_class_gain(void)

{
  short in_AX;
  float10 fVar1;

  fVar1 = (float10)*(float *)(DAT_00746140 + 4 + in_AX * 0xc);
  if (((in_AX == 0x2c) || (in_AX == 0x2e)) || (in_AX == 0x2f)) {
    return fVar1 * (float10)DAT_007252ac;
  }
  if (in_AX == 0x20) {
    return (float10)DAT_007252a8 * (float10)_DAT_007252a4 * (float10)DAT_007252ac * fVar1;
  }
  if ((in_AX != 0x2d) && (in_AX != 0x13)) {
    return (float10)DAT_007252b0 * (float10)_DAT_007252a4 * (float10)DAT_007252ac * fVar1;
  }
  return (float10)_DAT_007252a4 * (float10)DAT_007252ac * fVar1;
}

Disassembly (0x54b100..0x54b174, capstone; phase-4 review):

0x54b100: cmp ax, 0x2c
0x54b104: mov edx, dword ptr [0x746140]
0x54b10a: movsx ecx, ax
0x54b10d: lea ecx, [ecx + ecx*2]
0x54b110: fld dword ptr [edx + ecx*4 + 4]
0x54b114: je 0x54b16d
0x54b116: cmp ax, 0x2e
0x54b11a: je 0x54b16d
0x54b11c: cmp ax, 0x2f
0x54b120: je 0x54b16d
0x54b122: cmp ax, 0x20
0x54b126: jne 0x54b13d
0x54b128: fld dword ptr [0x7252a8]
0x54b12e: fmul dword ptr [0x7252a4]
0x54b134: fmul dword ptr [0x7252ac]
0x54b13a: fmulp st(1)
0x54b13c: ret 
0x54b13d: cmp ax, 0x2d
0x54b141: je 0x54b15e
0x54b143: cmp ax, 0x13
0x54b147: je 0x54b15e
0x54b149: fld dword ptr [0x7252b0]
0x54b14f: fmul dword ptr [0x7252a4]
0x54b155: fmul dword ptr [0x7252ac]
0x54b15b: fmulp st(1)
0x54b15d: ret 
0x54b15e: fld dword ptr [0x7252a4]
0x54b164: fmul dword ptr [0x7252ac]
0x54b16a: fmulp st(1)
0x54b16c: ret 
0x54b16d: fmul dword ptr [0x7252ac]
0x54b173: ret 
#endif
