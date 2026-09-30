// sound_driver_end_frame  (Ghidra: FUN_00546b80)
// address 0x546b80, size 1037 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: sound_driver.end_frame (vtable +0x14 of the DirectSound driver at 0x0069f4c8);
//   commits deferred 3D listener settings, ramps directsound_fade by 0.05 per frame toward the
//   paused state, refills streaming channels (0x546b40) and formats the two debug channel
//   reports ("%1.2f %1.2f %s(%s)" per channel, "%i / %i mono 3D channels." etc.).
// Phase-4 review: rewritten from the disassembly appended below. Fixes against the draft:
//   CommitDeferredSettings takes only `this`; while fading out every active channel gets a
//   SetVolume, -10000 when fade * gain is 0 (the draft skipped those), with the log10 * 2000
//   conversion inlined here and clamped to [-10000, 0]; the per-channel detail line prints the
//   current and next permutations' names (SoundPermutation starts with its TagString name) or
//   "" (0x0065512c); the summary counts active channels per type by comparing type_flags with
//   sound_channel_type_flag_table[4] and prints them against sound_driver_parameters.channel_counts;
//   both report paths set two text-draw globals (0x006e4748 = 1, 0x006e474a = 0x118).
// register convention: plain __cdecl, no parameters (the frame is 8-aligned, 0x2018 bytes of
//   locals: an 0x2000-byte text buffer).

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern uint8_t directsound_deferred_dirty; // 0x00746132
extern void *directsound_listener;          // 0x00746114, IDirectSound3DListener *
extern uint8_t directsound_paused;          // 0x00746118
extern float directsound_fade;              // 0x0074611c
extern int16_t directsound_channel_count;   // 0x00725428
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430
extern uint8_t debug_sound_channels;        // 0x00724a4c, per-type channel totals
extern uint8_t debug_sound_channel_details; // 0x007251f8, per-channel lines
extern uint16_t sound_channel_type_flag_table[4]; // 0x0069f528, { 9, 8, 0xa, 0xe }
extern sound_driver_parameters driver_parameters; // 0x0069f514
extern int16_t hud_text_draw_background_mode; // 0x006e4748 (src/game declares it uint32_t; a word is stored here)
extern int16_t text_tab_stops;     // 0x006e474a (src/game declares it void *; a word is stored here)
extern char k_empty_string[];               // 0x0065512c


extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693
extern uint32_t strlen(const char *string);
extern double log10(double x);


void sound_driver_end_frame(void)
{
    char text[0x2000];
    int16_t i;

    if (directsound_deferred_dirty != 0) {
        ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[0x44 / 4])(directsound_listener);
        directsound_deferred_dirty = 0;
    }

    if (directsound_paused == 0) {
        if (directsound_fade != 1.0f) {
            double fade = (double)directsound_fade + 0.05;

            directsound_fade = (float)(fade < 1.0 ? fade : 1.0);
        }
    } else if (directsound_fade != 0.0f) {
        double fade = (double)directsound_fade - 0.05;

        directsound_fade = (float)(fade > 0.0 ? fade : 0.0);

        for (i = 0; i < directsound_channel_count; i++) {
            directsound_channel *channel = &directsound_channels[i];

            if (channel->state != _directsound_channel_idle || channel->streaming != 0) {
                float gain = directsound_fade * channel->gain;
                int32_t volume = -10000;

                if (gain != 0.0f) {
                    volume = (int32_t)(log10((double)gain) * 2000.0);
                    if (volume < -10000) {
                        volume = -10000;
                    } else if (volume > 0) {
                        volume = 0;
                    }
                }
                ((directsound_buffer_set_volume_proc)(*(void ***)channel->buffer)[0x3c / 4])(channel->buffer, volume);
            }
        }

        if (directsound_fade == 0.0f) {
            for (i = 0; i < directsound_channel_count; i++) {
                directsound_channel *channel = &directsound_channels[i];

                if (channel->state != _directsound_channel_idle || channel->streaming != 0) {
                    ((directsound_buffer_stop_proc)(*(void ***)channel->buffer)[0x48 / 4])(channel->buffer);
                }
            }
        }
    }

    sound_update_streaming_channels();

    if (debug_sound_channel_details != 0) {
        hud_text_draw_background_mode = 1;
        text_tab_stops = 0x118;
        text[0] = '\0';
        for (i = 0; i < directsound_channel_count; i++) {
            directsound_channel *channel = &directsound_channels[i];

            if (i >= 0x20) {
                continue;
            }
            if (channel->state != _directsound_channel_idle) {
                const char *current = channel->source != 0 ? (const char *)channel->source : k_empty_string;
                const char *next = channel->next_source != 0 ? (const char *)channel->next_source : k_empty_string;

                sprintf(text + strlen(text), "%1.2f %1.2f %s(%s)", (double)channel->gain, (double)channel->pitch,
                    current, next);
            }
            sprintf(text + strlen(text), "|t");
            if ((i & 1) != 0) {
                sprintf(text + strlen(text), "|n");
            }
        }
    } else if (debug_sound_channels != 0) {
        int32_t counts[4] = { 0, 0, 0, 0 };

        text_tab_stops = 0x118;
        hud_text_draw_background_mode = 1;
        text[0] = '\0';
        for (i = 0; i < directsound_channel_count; i++) {
            directsound_channel *channel = &directsound_channels[i];

            if (channel->state == _directsound_channel_idle) {
                continue;
            }
            if (channel->type_flags == sound_channel_type_flag_table[0]) {
                counts[0]++;
            } else if (channel->type_flags == sound_channel_type_flag_table[1]) {
                counts[1]++;
            } else if (channel->type_flags == sound_channel_type_flag_table[2]) {
                counts[2]++;
            } else if (channel->type_flags == sound_channel_type_flag_table[3]) {
                counts[3]++;
            }
        }
        sprintf(text, "|n|n%i / %i mono 3D channels.|n", counts[0], (int32_t)driver_parameters.channel_counts[0]);
        sprintf(text + strlen(text), "%i / %i mono channels.|n", counts[1], (int32_t)driver_parameters.channel_counts[1]);
        sprintf(text + strlen(text), "%i / %i stereo channels.|n", counts[2], (int32_t)driver_parameters.channel_counts[2]);
        sprintf(text + strlen(text), "%i / %i 44k stereo channels.|n", counts[3],
            (int32_t)driver_parameters.channel_counts[3]);
    }
    // the text is built and then dropped: the release build compiled the debug draw call out
}

#if 0
Original Ghidra decompilation (0x546b80):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00546b80(void)

{
  char *pcVar1;
  char *pcVar2;
  ushort uVar3;
  uint uVar4;
  int iVar5;
  short sVar6;
  char acStack_2008 [8188];
  undefined4 uStack_c;

  uStack_c = 0x546b90;
  if (DAT_00746132 != '\0') {
    (**(code **)(*DAT_00746114 + 0x44))();
    DAT_00746132 = '\0';
  }
  if (DAT_00746118 == '\0') {
    if ((_DAT_0074611c != 1.0) && (_DAT_0074611c = _DAT_0074611c + 0.05, 1.0 <= _DAT_0074611c)) {
      _DAT_0074611c = 1.0;
    }
  }
  else if (_DAT_0074611c != 0.0) {
    _DAT_0074611c = _DAT_0074611c - 0.05;
    if (_DAT_0074611c < 0.0) {
      _DAT_0074611c = 0.0;
    }
    sVar6 = 0;
    uVar3 = DAT_00725428;
    if (0 < (short)DAT_00725428) {
      do {
        iVar5 = (int)sVar6;
        if (((&DAT_00725430)[iVar5 * 0x33c] != 0) || ((&DAT_00725439)[iVar5 * 0x678] != '\0')) {
          if ((float10)_DAT_0074611c * (float10)(float)(&DAT_0072546c)[iVar5 * 0x19e] !=
              (float10)0.0) {
            log2((float10)_DAT_0074611c * (float10)(float)(&DAT_0072546c)[iVar5 * 0x19e]);
            __ftol();
          }
          (**(code **)(*(int *)(&DAT_00725aa0)[iVar5 * 0x19e] + 0x3c))();
          uVar3 = DAT_00725428;
        }
        sVar6 = sVar6 + 1;
      } while (sVar6 < (short)uVar3);
    }
    if ((_DAT_0074611c == 0.0) && (sVar6 = 0, 0 < (short)uVar3)) {
      do {
        iVar5 = (int)sVar6;
        if (((&DAT_00725430)[iVar5 * 0x33c] != 0) || ((&DAT_00725439)[iVar5 * 0x678] != '\0')) {
          (**(code **)(*(int *)(&DAT_00725aa0)[iVar5 * 0x19e] + 0x48))();
          uVar3 = DAT_00725428;
        }
        sVar6 = sVar6 + 1;
      } while (sVar6 < (short)uVar3);
    }
  }
  FUN_00546b40();
  if (DAT_007251f8 == '\0') {
    if (DAT_00724a4c != '\0') {
      _DAT_006e474a = 0x118;
      DAT_006e4748 = 1;
      acStack_2008[0] = '\0';
      if (0 < (short)DAT_00725428) {
        uVar4 = (uint)DAT_00725428;
        do {
          uVar4 = uVar4 - 1;
        } while (uVar4 != 0);
      }
      _sprintf(acStack_2008,"|n|n%i / %i mono 3D channels.|n");
      pcVar1 = acStack_2008;
      do {
        pcVar2 = pcVar1;
        pcVar1 = pcVar2 + 1;
      } while (*pcVar2 != '\0');
      _sprintf(pcVar2,"%i / %i mono channels.|n");
      pcVar1 = acStack_2008;
      do {
        pcVar2 = pcVar1;
        pcVar1 = pcVar2 + 1;
      } while (*pcVar2 != '\0');
      _sprintf(pcVar2,"%i / %i stereo channels.|n");
      pcVar1 = acStack_2008;
      do {
        pcVar2 = pcVar1;
        pcVar1 = pcVar2 + 1;
      } while (*pcVar2 != '\0');
      _sprintf(pcVar2,"%i / %i 44k stereo channels.|n");
    }
  }
  else {
    uVar3 = 0;
    DAT_006e4748 = 1;
    _DAT_006e474a = 0x118;
    acStack_2008[0] = '\0';
    if (0 < (short)DAT_00725428) {
      do {
        iVar5 = (int)(short)uVar3;
        if ((short)uVar3 < 0x20) {
          if ((&DAT_00725430)[iVar5 * 0x33c] != 0) {
            pcVar1 = acStack_2008;
            do {
              pcVar2 = pcVar1;
              pcVar1 = pcVar2 + 1;
            } while (*pcVar2 != '\0');
            _sprintf(pcVar2,"%1.2f %1.2f %s(%s)",(double)(float)(&DAT_0072546c)[iVar5 * 0x19e],
                     (double)(float)(&DAT_00725470)[iVar5 * 0x19e]);
          }
          pcVar1 = acStack_2008;
          do {
            pcVar2 = pcVar1;
            pcVar1 = pcVar2 + 1;
          } while (*pcVar2 != '\0');
          _sprintf(pcVar2,"|t");
          if ((uVar3 & 1) != 0) {
            pcVar1 = acStack_2008;
            do {
              pcVar2 = pcVar1;
              pcVar1 = pcVar2 + 1;
            } while (*pcVar2 != '\0');
            _sprintf(pcVar2,"|n");
          }
        }
        uVar3 = uVar3 + 1;
      } while ((short)uVar3 < (short)DAT_00725428);
      return;
    }
  }
  return;
}

Disassembly (0x546b80..0x546f8d, capstone; phase-4 review):

0x546b80: push ebp
0x546b81: mov ebp, esp
0x546b83: and esp, 0xfffffff8
0x546b86: mov eax, 0x2018
0x546b8b: call 0x628240
0x546b90: mov al, byte ptr [0x746132]
0x546b95: test al, al
0x546b97: push ebx
0x546b98: push ebp
0x546b99: push esi
0x546b9a: push edi
0x546b9b: je 0x546baf
0x546b9d: mov eax, dword ptr [0x746114]
0x546ba2: mov ecx, dword ptr [eax]
0x546ba4: push eax
0x546ba5: call dword ptr [ecx + 0x44]
0x546ba8: mov byte ptr [0x746132], 0
0x546baf: mov al, byte ptr [0x746118]
0x546bb4: test al, al
0x546bb6: je 0x546c53
0x546bbc: fld dword ptr [0x672ac0]
0x546bc2: fld dword ptr [0x74611c]
0x546bc8: fucompp 
0x546bca: fnstsw ax
0x546bcc: test ah, 0x44
0x546bcf: jnp 0x546d34
0x546bd5: fld dword ptr [0x74611c]
0x546bdb: fsub qword ptr [0x672da8]
0x546be1: fld qword ptr [0x672c08]
0x546be7: fcomp st(1)
0x546be9: fnstsw ax
0x546beb: test ah, 0x41
0x546bee: jne 0x546bf8
0x546bf0: fstp st(0)
0x546bf2: fld qword ptr [0x672c08]
0x546bf8: mov cx, word ptr [0x725428]
0x546bff: fstp dword ptr [0x74611c]
0x546c05: xor edi, edi
0x546c07: test cx, cx
0x546c0a: jle 0x546cdd
0x546c10: movsx esi, di
0x546c13: imul esi, esi, 0x678
0x546c19: add esi, 0x725430
0x546c1f: cmp word ptr [esi], 0
0x546c23: jne 0x546c30
0x546c25: mov al, byte ptr [esi + 9]
0x546c28: test al, al
0x546c2a: je 0x546cd3
0x546c30: fld dword ptr [0x74611c]
0x546c36: fmul dword ptr [esi + 0x3c]
0x546c39: fld dword ptr [0x672ac0]
0x546c3f: fld st(1)
0x546c41: fucompp 
0x546c43: fnstsw ax
0x546c45: test ah, 0x44
0x546c48: jp 0x546c9a
0x546c4a: fstp st(0)
0x546c4c: mov eax, 0xffffd8f0
0x546c51: jmp 0x546cbf
0x546c53: fld dword ptr [0x672ac4]
0x546c59: fld dword ptr [0x74611c]
0x546c5f: fucompp 
0x546c61: fnstsw ax
0x546c63: test ah, 0x44
0x546c66: jnp 0x546d34
0x546c6c: fld dword ptr [0x74611c]
0x546c72: fadd qword ptr [0x672da8]
0x546c78: fld qword ptr [0x672af8]
0x546c7e: fcomp st(1)
0x546c80: fnstsw ax
0x546c82: test ah, 0x41
0x546c85: je 0x546c8f
0x546c87: fstp st(0)
0x546c89: fld qword ptr [0x672af8]
0x546c8f: fstp dword ptr [0x74611c]
0x546c95: jmp 0x546d34
0x546c9a: fldlg2 
0x546c9c: fxch st(1)
0x546c9e: fyl2x 
0x546ca0: fmul qword ptr [0x672b10]
0x546ca6: call 0x6391b4
0x546cab: cmp eax, 0xffffd8f0
0x546cb0: jge 0x546cb9
0x546cb2: mov eax, 0xffffd8f0
0x546cb7: jmp 0x546cbf
0x546cb9: test eax, eax
0x546cbb: jle 0x546cbf
0x546cbd: xor eax, eax
0x546cbf: mov esi, dword ptr [esi + 0x670]
0x546cc5: mov edx, dword ptr [esi]
0x546cc7: push eax
0x546cc8: push esi
0x546cc9: call dword ptr [edx + 0x3c]
0x546ccc: mov cx, word ptr [0x725428]
0x546cd3: inc edi
0x546cd4: cmp di, cx
0x546cd7: jl 0x546c10
0x546cdd: fld dword ptr [0x672ac0]
0x546ce3: fld dword ptr [0x74611c]
0x546ce9: fucompp 
0x546ceb: fnstsw ax
0x546ced: test ah, 0x44
0x546cf0: jp 0x546d34
0x546cf2: xor esi, esi
0x546cf4: test cx, cx
0x546cf7: jle 0x546d34
0x546cf9: lea esp, [esp]
0x546d00: movsx eax, si
0x546d03: imul eax, eax, 0x678
0x546d09: add eax, 0x725430
0x546d0e: cmp word ptr [eax], 0
0x546d12: jne 0x546d1b
0x546d14: mov dl, byte ptr [eax + 9]
0x546d17: test dl, dl
0x546d19: je 0x546d2e
0x546d1b: mov eax, dword ptr [eax + 0x670]
0x546d21: mov ecx, dword ptr [eax]
0x546d23: push eax
0x546d24: call dword ptr [ecx + 0x48]
0x546d27: mov cx, word ptr [0x725428]
0x546d2e: inc esi
0x546d2f: cmp si, cx
0x546d32: jl 0x546d00
0x546d34: call 0x546b40
0x546d39: mov al, byte ptr [0x7251f8]
0x546d3e: test al, al
0x546d40: je 0x546e48
0x546d46: xor ebx, ebx
0x546d48: cmp word ptr [0x725428], bx
0x546d4f: mov ax, 0x118
0x546d53: mov dword ptr [esp + 0x1a], ebx
0x546d57: mov word ptr [0x6e4748], 1
0x546d60: mov word ptr [0x6e474a], ax
0x546d66: mov byte ptr [esp + 0x28], bl
0x546d6a: jle 0x546f88
0x546d70: movsx ecx, bx
0x546d73: imul ecx, ecx, 0x678
0x546d79: add ecx, 0x725430
0x546d7f: cmp bx, 0x20
0x546d83: jge 0x546e32
0x546d89: cmp word ptr [ecx], 0
0x546d8d: je 0x546de5
0x546d8f: mov eax, dword ptr [ecx + 0x8c]
0x546d95: test eax, eax
0x546d97: mov ebp, eax
0x546d99: jne 0x546da0
0x546d9b: mov ebp, 0x65512c
0x546da0: mov eax, dword ptr [ecx + 0x88]
0x546da6: test eax, eax
0x546da8: mov esi, eax
0x546daa: jne 0x546db1
0x546dac: mov esi, 0x65512c
0x546db1: lea eax, [esp + 0x28]
0x546db5: lea edi, [eax + 1]
0x546db8: mov dl, byte ptr [eax]
0x546dba: inc eax
0x546dbb: test dl, dl
0x546dbd: jne 0x546db8
0x546dbf: fld dword ptr [ecx + 0x40]
0x546dc2: push ebp
0x546dc3: push esi
0x546dc4: sub esp, 0x10
0x546dc7: fstp qword ptr [esp + 8]
0x546dcb: sub eax, edi
0x546dcd: fld dword ptr [ecx + 0x3c]
0x546dd0: lea edx, [esp + eax + 0x40]
0x546dd4: fstp qword ptr [esp]
0x546dd7: push 0x671958
0x546ddc: push edx
0x546ddd: call 0x623693
0x546de2: add esp, 0x20
0x546de5: lea eax, [esp + 0x28]
0x546de9: lea edx, [eax + 1]
0x546dec: lea esp, [esp]
0x546df0: mov cl, byte ptr [eax]
0x546df2: inc eax
0x546df3: test cl, cl
0x546df5: jne 0x546df0
0x546df7: sub eax, edx
0x546df9: lea eax, [esp + eax + 0x28]
0x546dfd: push 0x669140
0x546e02: push eax
0x546e03: call 0x623693
0x546e08: add esp, 8
0x546e0b: test bl, 1
0x546e0e: je 0x546e32
0x546e10: lea eax, [esp + 0x28]
0x546e14: lea edx, [eax + 1]
0x546e17: mov cl, byte ptr [eax]
0x546e19: inc eax
0x546e1a: test cl, cl
0x546e1c: jne 0x546e17
0x546e1e: sub eax, edx
0x546e20: lea ecx, [esp + eax + 0x28]
0x546e24: push 0x669ae0
0x546e29: push ecx
0x546e2a: call 0x623693
0x546e2f: add esp, 8
0x546e32: inc ebx
0x546e33: cmp bx, word ptr [0x725428]
0x546e3a: jl 0x546d70
0x546e40: pop edi
0x546e41: pop esi
0x546e42: pop ebp
0x546e43: pop ebx
0x546e44: mov esp, ebp
0x546e46: pop ebp
0x546e47: ret 
0x546e48: mov al, byte ptr [0x724a4c]
0x546e4d: test al, al
0x546e4f: je 0x546f88
0x546e55: mov ax, 0x118
0x546e59: xor esi, esi
0x546e5b: mov word ptr [0x6e474a], ax
0x546e61: mov ax, word ptr [0x725428]
0x546e67: xor ebp, ebp
0x546e69: cmp ax, si
0x546e6c: mov dword ptr [esp + 0x14], esi
0x546e70: mov dword ptr [esp + 0x18], esi
0x546e74: mov dword ptr [esp + 0x22], esi
0x546e78: mov word ptr [0x6e4748], 1
0x546e81: mov byte ptr [esp + 0x28], 0
0x546e86: jle 0x546eda
0x546e88: mov di, word ptr [0x69f528]
0x546e8f: mov ecx, 0x725468
0x546e94: movzx edx, ax
0x546e97: cmp word ptr [ecx - 0x38], 0
0x546e9c: je 0x546ed1
0x546e9e: mov ax, word ptr [ecx]
0x546ea1: cmp ax, di
0x546ea4: jne 0x546ea9
0x546ea6: inc esi
0x546ea7: jmp 0x546ed1
0x546ea9: cmp ax, word ptr [0x69f52a]
0x546eb0: jne 0x546eb5
0x546eb2: inc ebp
0x546eb3: jmp 0x546ed1
0x546eb5: cmp ax, word ptr [0x69f52c]
0x546ebc: jne 0x546ec4
0x546ebe: inc dword ptr [esp + 0x14]
0x546ec2: jmp 0x546ed1
0x546ec4: cmp ax, word ptr [0x69f52e]
0x546ecb: jne 0x546ed1
0x546ecd: inc dword ptr [esp + 0x18]
0x546ed1: add ecx, 0x678
0x546ed7: dec edx
0x546ed8: jne 0x546e97
0x546eda: movsx edx, word ptr [0x69f516]
0x546ee1: push edx
0x546ee2: push esi
0x546ee3: lea eax, [esp + 0x30]
0x546ee7: push 0x671938
0x546eec: push eax
0x546eed: call 0x623693
0x546ef2: lea eax, [esp + 0x38]
0x546ef6: add esp, 0x10
0x546ef9: lea edx, [eax + 1]
0x546efc: lea esp, [esp]
0x546f00: mov cl, byte ptr [eax]
0x546f02: inc eax
0x546f03: test cl, cl
0x546f05: jne 0x546f00
0x546f07: movsx ecx, word ptr [0x69f518]
0x546f0e: push ecx
0x546f0f: sub eax, edx
0x546f11: push ebp
0x546f12: lea edx, [esp + eax + 0x30]
0x546f16: push 0x67191c
0x546f1b: push edx
0x546f1c: call 0x623693
0x546f21: lea eax, [esp + 0x38]
0x546f25: add esp, 0x10
0x546f28: lea edx, [eax + 1]
0x546f2b: jmp 0x546f30
0x546f2d: lea ecx, [ecx]
0x546f30: mov cl, byte ptr [eax]
0x546f32: inc eax
0x546f33: test cl, cl
0x546f35: jne 0x546f30
0x546f37: movsx ecx, word ptr [0x69f51a]
0x546f3e: sub eax, edx
0x546f40: mov edx, dword ptr [esp + 0x14]
0x546f44: push ecx
0x546f45: push edx
0x546f46: lea eax, [esp + eax + 0x30]
0x546f4a: push 0x671900
0x546f4f: push eax
0x546f50: call 0x623693
0x546f55: lea eax, [esp + 0x38]
0x546f59: add esp, 0x10
0x546f5c: lea edx, [eax + 1]
0x546f5f: nop 
0x546f60: mov cl, byte ptr [eax]
0x546f62: inc eax
0x546f63: test cl, cl
0x546f65: jne 0x546f60
0x546f67: movsx ecx, word ptr [0x69f51c]
0x546f6e: sub eax, edx
0x546f70: mov edx, dword ptr [esp + 0x18]
0x546f74: push ecx
0x546f75: push edx
0x546f76: lea eax, [esp + eax + 0x30]
0x546f7a: push 0x6718e0
0x546f7f: push eax
0x546f80: call 0x623693
0x546f85: add esp, 0x10
0x546f88: pop edi
0x546f89: pop esi
0x546f8a: pop ebp
0x546f8b: pop ebx
#endif
