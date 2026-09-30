// sound_looping_state_new  (Ghidra: FUN_0054d140, still unnamed there)
// address 0x54d140, size 293 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/sound_functions.md "Allocates a new looping-sound state datum for an
// object/sound pair, seeding its per-permutation random selection values."; types/sound.h
// looping_sound definition_index/owner/finished/active_sound_count (0x04/0x08/0x4e/0x50) and
// detail_next_time[] (0x54); its only caller sound_looping_set_state (0x549fa0) pushes
// (location, owner, definition) right to left.
// register convention: plain stack, (definition_index, owner, location).
// FPU sequence resolved from the disassembly below (phase-4 review): the scale is
// location->scale ([arg3+4]); per detail the random value is the inlined LCG of
// random_real_range_seeded (0x4cd170, seed 0x00719cd4) over random_period_bounds, and the
// result is (int)((float)sound_time + random * period * 1000.0) with
// period = zero_detail_sound_period + (one - zero) * location->scale.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "sound.h"
#include "fn_sound.h"

extern uint8_t sound_initialized;  // 0x00725200
extern uint8_t sound_enabled;      // 0x00725201
extern uint8_t sound_disabled;     // 0x007252b6
extern data_array *looping_sound_data; // 0x00724a50, "looping sounds" 0x80 x 0xe4
extern tag_instance *tag_instances;    // 0x0087bc14
extern random_seed effect_random_seed;  // 0x00719cd4
extern int32_t sound_time;             // 0x0072520c

extern datum_index datum_new(data_array *array); // 0x4d0480, memory module
extern real random_real_range_seeded(random_seed *seed, real min, real max); // 0x4cd170, math module (inlined here)

// blam-cc: stack -> (definition_index, owner, location)
// Allocates a new looping_sound datum for a (SoundLooping tag, owner) pair, if sound is
// currently enabled. Seeds detail_next_time[] for every detail sound with a randomized future
// time: random_period_bounds (seconds) scaled by the zero/one detail period lerp at
// location->scale.
datum_index sound_looping_state_new(datum_index definition_index, int32_t owner, sound_location *location)
{
    datum_index handle;
    looping_sound *state;
    SoundLooping *definition;
    int16_t detail_index;

    if (!sound_initialized || !sound_enabled || sound_disabled) {
        return 0xffffffff;
    }

    handle = datum_new(looping_sound_data);
    if (handle == 0xffffffff) {
        return handle;
    }

    state = (looping_sound *)((uint8_t *)looping_sound_data->data + (handle & 0xffff) * sizeof(looping_sound));
    definition = (SoundLooping *)tag_instances[definition_index & 0xffff].data;

    state->definition_index = definition_index;
    state->owner = owner;
    state->active_sound_count = 0;
    state->finished = 0;

    for (detail_index = 0; detail_index < (int32_t)definition->detail_sounds.count; detail_index++) {
        SoundLoopingDetail *detail = (SoundLoopingDetail *)definition->detail_sounds.pointer + detail_index;
        float random_value = random_real_range_seeded(&effect_random_seed, detail->random_period_bounds[0],
            detail->random_period_bounds[1]);
        float period = definition->zero_detail_sound_period +
            (definition->one_detail_sound_period - definition->zero_detail_sound_period) * location->scale;

        state->detail_next_time[detail_index] = (int32_t)((float)sound_time + random_value * period * 1000.0f);
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x54d140):

uint FUN_0054d140(uint param_1,undefined4 param_2)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  short sVar5;
  int iVar6;
  undefined8 uVar7;

  iVar4 = 0;
  uVar2 = 0xffffffff;
  if (((DAT_00725200 != '\0') && (DAT_00725201 != '\0')) && (DAT_007252b6 == '\0')) {
    uVar7 = datum_new();
    uVar2 = (uint)uVar7;
    if (uVar2 != 0xffffffff) {
      iVar6 = (uVar2 & 0xffff) * 0xe4 + *(int *)((int)((ulonglong)uVar7 >> 0x20) + 0x34);
      iVar1 = *(int *)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      *(uint *)(iVar6 + 4) = param_1;
      *(undefined4 *)(iVar6 + 8) = param_2;
      *(undefined2 *)(iVar6 + 0x50) = 0;
      *(undefined1 *)(iVar6 + 0x4e) = 0;
      sVar5 = 0;
      if (0 < *(int *)(iVar1 + 0x48)) {
        do {
          DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
          uVar3 = __ftol();
          sVar5 = sVar5 + 1;
          *(undefined4 *)(iVar6 + 0x54 + iVar4 * 4) = uVar3;
          iVar4 = (int)sVar5;
        } while (iVar4 < *(int *)(iVar1 + 0x48));
      }
    }
  }
  return uVar2;
}

Disassembly (0x54d140..0x54d265, capstone; phase-4 review):

0x54d140: mov cl, byte ptr [0x725200]
0x54d146: sub esp, 8
0x54d149: push ebx
0x54d14a: xor ebx, ebx
0x54d14c: or eax, 0xffffffff
0x54d14f: cmp cl, bl
0x54d151: push ebp
0x54d152: mov ebp, dword ptr [esp + 0x14]
0x54d156: je 0x54d25f
0x54d15c: cmp byte ptr [0x725201], bl
0x54d162: je 0x54d25f
0x54d168: cmp byte ptr [0x7252b6], bl
0x54d16e: jne 0x54d25f
0x54d174: mov edx, dword ptr [0x724a50]
0x54d17a: call 0x4d0480
0x54d17f: cmp eax, -1
0x54d182: mov dword ptr [esp + 0xc], eax
0x54d186: je 0x54d25f
0x54d18c: mov ecx, dword ptr [edx + 0x34]
0x54d18f: mov edx, dword ptr [0x87bc14]
0x54d195: push esi
0x54d196: mov esi, eax
0x54d198: and esi, 0xffff
0x54d19e: imul esi, esi, 0xe4
0x54d1a4: add esi, ecx
0x54d1a6: mov ecx, ebp
0x54d1a8: and ecx, 0xffff
0x54d1ae: shl ecx, 5
0x54d1b1: push edi
0x54d1b2: mov edi, dword ptr [ecx + edx + 0x14]
0x54d1b6: mov ecx, dword ptr [esp + 0x20]
0x54d1ba: mov dword ptr [esi + 4], ebp
0x54d1bd: mov dword ptr [esi + 8], ecx
0x54d1c0: mov word ptr [esi + 0x50], bx
0x54d1c4: mov byte ptr [esi + 0x4e], bl
0x54d1c7: mov ecx, dword ptr [edi + 0x48]
0x54d1ca: xor ebp, ebp
0x54d1cc: cmp ecx, ebx
0x54d1ce: jle 0x54d25d
0x54d1d4: mov ecx, dword ptr [edi + 0x4c]
0x54d1d7: mov edx, dword ptr [esp + 0x24]
0x54d1db: fld dword ptr [edx + 4]
0x54d1de: mov eax, ebx
0x54d1e0: fld dword ptr [edi + 0x10]
0x54d1e3: imul eax, eax, 0x68
0x54d1e6: fld dword ptr [edi + 4]
0x54d1e9: fld dword ptr [eax + ecx + 0x14]
0x54d1ed: fld dword ptr [eax + ecx + 0x10]
0x54d1f1: fxch st(1)
0x54d1f3: fsub st(1)
0x54d1f5: add eax, ecx
0x54d1f7: mov eax, dword ptr [0x719cd4]
0x54d1fc: imul eax, eax, 0x19660d
0x54d202: add eax, 0x3c6ef35f
0x54d207: mov ecx, eax
0x54d209: shr ecx, 0x10
0x54d20c: mov dword ptr [esp + 0x10], ecx
0x54d210: mov dword ptr [0x719cd4], eax
0x54d215: fild dword ptr [esp + 0x10]
0x54d219: fmul dword ptr [0x672b84]
0x54d21f: fmulp st(1)
0x54d221: fadd st(1)
0x54d223: fxch st(3)
0x54d225: fsub st(2)
0x54d227: fmul st(4)
0x54d229: fadd st(2)
0x54d22b: fmulp st(3)
0x54d22d: fxch st(2)
0x54d22f: fmul dword ptr [0x672ae8]
0x54d235: fiadd dword ptr [0x72520c]
0x54d23b: call 0x6391b4
0x54d240: fstp st(1)
0x54d242: inc ebp
0x54d243: fstp st(0)
0x54d245: mov dword ptr [esi + ebx*4 + 0x54], eax
0x54d249: fstp st(0)
0x54d24b: mov eax, dword ptr [edi + 0x48]
0x54d24e: movsx ebx, bp
0x54d251: cmp ebx, eax
0x54d253: jl 0x54d1d4
0x54d259: mov eax, dword ptr [esp + 0x14]
0x54d25d: pop edi
0x54d25e: pop esi
0x54d25f: pop ebp
0x54d260: pop ebx
0x54d261: add esp, 8
0x54d264: ret 
#endif
