// sound_channel_create  (Ghidra: sound_channel_create, already named)
// address 0x546760, size 763 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/sound_functions.md summary "Creates a single DirectSound secondary buffer
//   for a sound channel, optionally obtaining its 3D-buffer interface and default 3D properties.";
//   types/sound.h directsound_channel (sound_channel_index 0x002, sound_class 0x004, free 0x008,
//   streaming 0x009, type_flags 0x038, buffer_size 0x068, streaming_bytes 0x084,
//   source/next_source 0x088/0x08c, buffer/buffer_3d 0x670/0x674); WAVEFORMATEX / DSBUFFERDESC
//   (sound_wave_format / sound_buffer_description) for a 3-second 16-bit buffer.
//   IDirectSound vtable +0xc is CreateSoundBuffer, IDirectSoundBuffer +0xc is GetCaps (DSBCAPS,
//   dwSize 0x14), +0 is QueryInterface with IID_IDirectSound3DBuffer (0x0064e21c =
//   {279afa86-4981-11ce-a521-0020af0be560}); the 3D algorithm written when EAX is off is
//   DS3DALG_HRTF_FULL (0x0064e1fc = {c241333f-1c1b-11d2-94f5-00c04fc28aca}).
// Phase-4 review: rewritten from the disassembly appended below. The Ghidra C (and the earlier
//   draft built on it) lost three facts: a 3D buffer whose DSBCAPS has DSBCAPS_LOCHARDWARE (4)
//   increments the global at 0x00746124, which is therefore the count of hardware 3D channels
//   (directsound_hardware_3d_channel_count, types/sound.h); the "is 3D" test before the
//   QueryInterface is the channel's 3D type bit; the EAX gate compares the channel index against
//   that hardware count, and sound_effects_object_detect_mode / _initialize_channel receive the
//   channel (EAX = index, EDI = channel / EDX = index). The final parameter block is
//   {0, 0, pitch 1.0, gain 1.0, 0, 0, 0, 0}, not all zeros.
// register convention: stack -> channel_index, CX -> type_flags; returns AL.
// Returns 0 when a 3D buffer or its caps cannot be created; a failed 2D CreateSoundBuffer
// returns 1 (binary behaviour, kept).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "sound.h"

extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430
extern void *directsound;                    // 0x0074610c, IDirectSound*
extern uint8_t directsound_eax_available;    // 0x00746120
extern uint8_t directsound_eax_enabled;      // 0x00746121
extern int16_t directsound_hardware_3d_channel_count; // 0x00746124
extern int32_t directsound_hardware_mode;    // 0x0074612c, sound effect object mode, 3 adds DSBCAPS 0x200
extern int16_t sound_effect_object_state;    // 0x00746130
extern uint8_t iid_directsound_3d_buffer[16]; // 0x0064e21c, IID_IDirectSound3DBuffer
extern uint32_t ds3dalg_hrtf_full[4];        // 0x0064e1fc, DS3DALG_HRTF_FULL
extern const real_vector3d *global_forward3d_pointer; // 0x00696718

extern int32_t sound_effects_object_detect_mode(int16_t channel_index, directsound_channel *channel); // 0x551270, blam-cc: EAX, EDI
extern void sound_effects_object_shutdown(void);        // 0x551420
extern int32_t sound_effects_object_initialize_channel(int16_t channel_index); // 0x551460, blam-cc: EDX
extern void sound_channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial,
    float obstruction, float occlusion, uint8_t underwater, int16_t sound_class); // 0x5472d0, blam-cc: stack, BL, EDI, stack
extern void sound_channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update);
    // 0x5475b0, blam-cc: stack, EDI, stack


// blam-cc: stack -> channel_index, CX -> type_flags
uint8_t sound_channel_create(int16_t channel_index, uint16_t type_flags)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    sound_wave_format format;
    sound_buffer_description desc;
    uint16_t channels;
    uint32_t samples_per_second;
    int32_t hr;
    void **vtable;

    channel->sound_channel_index = -1;
    channel->sound_class = -1;
    channel->streaming_bytes = -1;
    channel->type_flags = type_flags;
    channel->free = 0;
    channel->source = (SoundPermutation *)0;
    channel->next_source = (SoundPermutation *)0;
    channel->state = _directsound_channel_idle;
    channel->streaming = 0;

    channels = (uint16_t)(((type_flags & _sound_channel_stereo_bit) != 0) + 1);
    samples_per_second = (type_flags & _sound_channel_44khz_bit) != 0 ? 44100 : 22050;

    format.format_tag = 1;
    format.channels = channels;
    format.samples_per_second = samples_per_second;
    format.average_bytes_per_second = (uint32_t)(uint16_t)(channels * 2) * samples_per_second;
    format.block_align = (uint16_t)(channels * 2);
    format.bits_per_sample = 16;
    format.extra_size = 0;
    channel->buffer_size = (int32_t)(format.average_bytes_per_second * 3);

    desc.size = 0x24;
    desc.flags = (directsound_eax_enabled == 0 || directsound_eax_available == 0) ? 0x100a8 : 0x100a0;
    desc.buffer_bytes = (uint32_t)channel->buffer_size;
    desc.reserved = 0;
    desc.format = &format;
    desc.algorithm_3d[0] = 0;
    desc.algorithm_3d[1] = 0;
    desc.algorithm_3d[2] = 0;
    desc.algorithm_3d[3] = 0;

    if ((type_flags & _sound_channel_3d_bit) != 0) {
        win32_dsbcaps caps;

        desc.flags |= directsound_hardware_mode == 3 ? 0x210 : 0x10;
        if (directsound_eax_enabled == 0 || directsound_eax_available == 0) {
            desc.algorithm_3d[0] = ds3dalg_hrtf_full[0];
            desc.algorithm_3d[1] = ds3dalg_hrtf_full[1];
            desc.algorithm_3d[2] = ds3dalg_hrtf_full[2];
            desc.algorithm_3d[3] = ds3dalg_hrtf_full[3];
        }

        vtable = *(void ***)directsound;
        if (((directsound_create_sound_buffer_proc)vtable[3])(directsound, &desc, &channel->buffer, (void *)0) < 0) {
            return 0;
        }

        caps.size = 0x14;
        caps.flags = 0;
        caps.buffer_bytes = 0;
        caps.unlock_transfer_rate = 0;
        caps.play_cpu_overhead = 0;
        vtable = *(void ***)channel->buffer;
        hr = ((directsound_buffer_get_caps_proc)vtable[3])(channel->buffer, &caps);
        if (hr < 0) {
            return 0;
        }
        if ((caps.flags & 4) != 0) {
            directsound_hardware_3d_channel_count += 1;
        }
    } else {
        vtable = *(void ***)directsound;
        hr = ((directsound_create_sound_buffer_proc)vtable[3])(directsound, &desc, &channel->buffer, (void *)0);
    }

    if (hr < 0) {
        return 1;
    }

    if ((type_flags & _sound_channel_3d_bit) == 0) {
        channel->buffer_3d = (void *)0;
    } else {
        vtable = *(void ***)channel->buffer;
        if (((directsound_query_interface_proc)vtable[0])(channel->buffer, iid_directsound_3d_buffer,
                &channel->buffer_3d) < 0) {
            channel->buffer_3d = (void *)0;
        } else {
            sound_channel_spatial spatial = { 0 };

            spatial.forward = *(Vector3D *)global_forward3d_pointer;

            if (sound_effect_object_state == 0) {
                sound_effect_object_state = 2;
                directsound_hardware_mode = sound_effects_object_detect_mode(channel_index, channel);
                if (directsound_hardware_mode != -1) {
                    sound_effect_object_state = 1;
                }
            }
            if (sound_effect_object_state == 1 &&
                channel_index < directsound_hardware_3d_channel_count &&
                sound_effects_object_initialize_channel(channel_index) == 0) {
                sound_effects_object_shutdown();
                sound_effect_object_state = 2;
            }

            sound_channel_set_spatial(channel_index, 0, &spatial, 0.0f, 0.0f, 0, 0);
        }
    }

    {
        sound_channel_parameters parameters = { 0 };

        parameters.pitch = 1.0f;
        parameters.gain = 1.0f;
        sound_channel_set_parameters(channel_index, &parameters, 0);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x546760):

/* WARNING: Removing unreachable block (ram,0x005468fd) */

undefined4 sound_channel_create(short param_1)

{
  ushort in_CX;
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  short sVar4;
  undefined4 *puVar5;
  undefined2 local_60;
  short local_5e;
  undefined4 uStack_5c;
  int local_58;
  undefined4 local_54;
  undefined4 uStack_50;
  undefined4 uStack_4c;
  undefined4 local_24;
  uint local_20;
  int local_1c;
  undefined4 local_18;
  undefined2 *local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = (int)param_1;
  iVar2 = iVar1 * 0x678;
  *(undefined2 *)(&DAT_00725432 + iVar2) = 0xffff;
  *(undefined2 *)(&DAT_00725434 + iVar2) = 0xffff;
  *(undefined4 *)(&DAT_007254b4 + iVar2) = 0xffffffff;
  *(ushort *)(&DAT_00725468 + iVar2) = in_CX;
  (&DAT_00725438)[iVar2] = 0;
  (&DAT_007254b8)[iVar1 * 0x19e] = 0;
  (&DAT_007254bc)[iVar1 * 0x19e] = 0;
  local_5e = ((in_CX & 2) != 0) + 1;
  (&DAT_00725430)[iVar1 * 0x33c] = 0;
  (&DAT_00725439)[iVar2] = 0;
  uStack_5c = (-(uint)((in_CX & 4) != 0) & 0x5622) + 0x5622;
  local_58 = (uint)(ushort)(local_5e * 2) * uStack_5c;
  local_18 = 0;
  local_1c = local_58 * 3;
  local_10 = 0;
  *(int *)(&DAT_00725498 + iVar2) = local_1c;
  local_14 = &local_60;
  local_c = 0;
  local_8 = 0;
  local_4 = 0;
  local_60 = 1;
  local_54 = CONCAT22(0x10,local_5e * 2);
  local_24 = 0x24;
  local_20 = 0x100a0;
  if ((DAT_00746121 == '\0') || (DAT_00746120 == '\0')) {
    local_20 = 0x100a8;
  }
  if ((in_CX & 1) == 0) {
    puVar3 = &DAT_00725aa0 + iVar1 * 0x19e;
    iVar2 = (**(code **)(*DAT_0074610c + 0xc))(DAT_0074610c,&local_24,puVar3,0);
    sVar4 = (short)puVar3;
  }
  else {
    if (DAT_0074612c == 3) {
      local_20 = local_20 | 0x210;
    }
    else {
      local_20 = local_20 | 0x10;
    }
    if ((DAT_00746121 == '\0') || (DAT_00746120 == '\0')) {
      local_10 = 0xc241333f;
      local_c = 0x11d21c1b;
      local_8 = 0xc000f594;
      local_4 = 0xca8ac24f;
    }
    puVar3 = &DAT_00725aa0 + iVar1 * 0x19e;
    puVar5 = puVar3;
    iVar2 = (**(code **)(*DAT_0074610c + 0xc))(DAT_0074610c,&local_24,puVar3,0);
    sVar4 = (short)puVar5;
    if (iVar2 < 0) {
      return 0;
    }
    uStack_5c._0_2_ = 0;
    uStack_5c._2_2_ = 0;
    local_58 = 0;
    local_54 = 0;
    uStack_50 = 0;
    local_60 = 0x14;
    local_5e = 0;
    iVar2 = (**(code **)(*(int *)*puVar3 + 0xc))((int *)*puVar3,&local_60);
    if (iVar2 < 0) {
      return 0;
    }
  }
  if (iVar2 < 0) {
    return 1;
  }
  if (sVar4 == 0) {
    (&DAT_00725aa4)[iVar1 * 0x19e] = 0;
    goto LAB_00546a0d;
  }
  puVar3 = &DAT_00725aa4 + iVar1 * 0x19e;
  iVar1 = (*(code *)**(undefined4 **)(&DAT_00725aa0)[iVar1 * 0x19e])
                    ((undefined4 *)(&DAT_00725aa0)[iVar1 * 0x19e],&DAT_0064e21c,puVar3);
  if (iVar1 < 0) {
    *puVar3 = 0;
    goto LAB_00546a0d;
  }
  puVar3 = (undefined4 *)&stack0xffffff98;
  for (iVar1 = 0xb; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  local_58 = *(int *)(PTR_DAT_00696718 + 4);
  local_54 = *(undefined4 *)(PTR_DAT_00696718 + 8);
  uStack_5c._0_2_ = (undefined2)*(undefined4 *)PTR_DAT_00696718;
  uStack_5c._2_2_ = (undefined2)((uint)*(undefined4 *)PTR_DAT_00696718 >> 0x10);
  if (DAT_00746130 == 0) {
    DAT_00746130 = 2;
    DAT_0074612c = sound_effects_object_detect_mode();
    if (DAT_0074612c == -1) goto LAB_005469b3;
    DAT_00746130 = 1;
LAB_005469bd:
    if (((short)local_14 < DAT_00746124) && (iVar1 = FUN_00551460(), iVar1 == 0)) {
      sound_effects_object_shutdown();
      DAT_00746130 = 2;
    }
  }
  else {
LAB_005469b3:
    if (DAT_00746130 == 1) goto LAB_005469bd;
  }
  FUN_005472d0(local_14,0,0,0,0);
LAB_00546a0d:
  local_60 = 0;
  local_5e = 0;
  uStack_5c._0_2_ = 0;
  uStack_5c._2_2_ = 0;
  local_58 = 0;
  local_54 = 0;
  uStack_50 = 0;
  uStack_4c = 0;
  FUN_005475b0(local_14,0);
  return 1;
}

Disassembly (0x546760..0x546a5b, capstone; phase-4 review):

0x546760: sub esp, 0x64
0x546763: push ebx
0x546764: push ebp
0x546765: push esi
0x546766: push edi
0x546767: movsx ebp, word ptr [esp + 0x78]
0x54676c: or eax, 0xffffffff
0x54676f: mov esi, ebp
0x546771: imul esi, esi, 0x678
0x546777: add esi, 0x725430
0x54677d: mov word ptr [esi + 2], ax
0x546781: mov word ptr [esi + 4], ax
0x546785: xor ebx, ebx
0x546787: mov dword ptr [esi + 0x84], eax
0x54678d: test cl, 2
0x546790: mov eax, ebx
0x546792: setne al
0x546795: mov dl, cl
0x546797: and dl, 4
0x54679a: mov word ptr [esi + 0x38], cx
0x54679e: mov byte ptr [esi + 8], bl
0x5467a1: mov dword ptr [esi + 0x88], ebx
0x5467a7: mov dword ptr [esi + 0x8c], ebx
0x5467ad: inc eax
0x5467ae: mov word ptr [esp + 0x16], ax
0x5467b3: mov eax, dword ptr [esp + 0x16]
0x5467b7: add eax, eax
0x5467b9: neg dl
0x5467bb: mov word ptr [esp + 0x20], ax
0x5467c0: movzx eax, ax
0x5467c3: mov word ptr [esi], bx
0x5467c6: mov byte ptr [esi + 9], bl
0x5467c9: sbb edx, edx
0x5467cb: and edx, 0x5622
0x5467d1: add edx, 0x5622
0x5467d7: imul eax, edx
0x5467da: mov dword ptr [esp + 0x18], edx
0x5467de: xor edx, edx
0x5467e0: mov dword ptr [esp + 0x50], edx
0x5467e4: mov dword ptr [esp + 0x54], edx
0x5467e8: mov dword ptr [esp + 0x58], edx
0x5467ec: mov dword ptr [esp + 0x5c], edx
0x5467f0: mov dword ptr [esp + 0x60], edx
0x5467f4: mov dword ptr [esp + 0x1c], eax
0x5467f8: lea eax, [eax + eax*2]
0x5467fb: mov dword ptr [esp + 0x64], edx
0x5467ff: mov dword ptr [esi + 0x68], eax
0x546802: mov dword ptr [esp + 0x58], eax
0x546806: lea eax, [esp + 0x14]
0x54680a: mov dword ptr [esp + 0x68], edx
0x54680e: mov dword ptr [esp + 0x6c], edx
0x546812: mov dword ptr [esp + 0x60], eax
0x546816: mov al, byte ptr [0x746121]
0x54681b: cmp al, bl
0x54681d: mov dword ptr [esp + 0x70], edx
0x546821: mov dl, byte ptr [0x746120]
0x546827: mov word ptr [esp + 0x14], 1
0x54682e: mov word ptr [esp + 0x22], 0x10
0x546835: mov dword ptr [esp + 0x50], 0x24
0x54683d: mov dword ptr [esp + 0x54], 0x100a0
0x546845: je 0x54684b
0x546847: cmp dl, bl
0x546849: jne 0x546853
0x54684b: mov dword ptr [esp + 0x54], 0x100a8
0x546853: and ecx, 1
0x546856: mov dword ptr [esp + 0x10], ecx
0x54685a: je 0x546910
0x546860: cmp dword ptr [0x74612c], 3
0x546867: mov ecx, dword ptr [esp + 0x54]
0x54686b: jne 0x546875
0x54686d: or ecx, 0x210
0x546873: jmp 0x546878
0x546875: or ecx, 0x10
0x546878: cmp al, bl
0x54687a: mov dword ptr [esp + 0x54], ecx
0x54687e: je 0x546884
0x546880: cmp dl, bl
0x546882: jne 0x5468ab
0x546884: mov ecx, dword ptr [0x64e1fc]
0x54688a: mov edx, dword ptr [0x64e200]
0x546890: mov eax, dword ptr [0x64e204]
0x546895: mov dword ptr [esp + 0x64], ecx
0x546899: mov ecx, dword ptr [0x64e208]
0x54689f: mov dword ptr [esp + 0x68], edx
0x5468a3: mov dword ptr [esp + 0x6c], eax
0x5468a7: mov dword ptr [esp + 0x70], ecx
0x5468ab: mov eax, dword ptr [0x74610c]
0x5468b0: mov edx, dword ptr [eax]
0x5468b2: push ebx
0x5468b3: lea edi, [esi + 0x670]
0x5468b9: push edi
0x5468ba: lea ecx, [esp + 0x58]
0x5468be: push ecx
0x5468bf: push eax
0x5468c0: call dword ptr [edx + 0xc]
0x5468c3: test eax, eax
0x5468c5: jl 0x546906
0x5468c7: mov eax, dword ptr [edi]
0x5468c9: xor edx, edx
0x5468cb: mov dword ptr [esp + 0x24], edx
0x5468cf: mov dword ptr [esp + 0x28], edx
0x5468d3: mov dword ptr [esp + 0x2c], edx
0x5468d7: mov dword ptr [esp + 0x30], edx
0x5468db: mov dword ptr [esp + 0x34], edx
0x5468df: lea edx, [esp + 0x24]
0x5468e3: push edx
0x5468e4: mov dword ptr [esp + 0x28], 0x14
0x5468ec: mov ecx, dword ptr [eax]
0x5468ee: push eax
0x5468ef: call dword ptr [ecx + 0xc]
0x5468f2: cmp eax, ebx
0x5468f4: jl 0x546906
0x5468f6: test byte ptr [esp + 0x28], 4
0x5468fb: je 0x546928
0x5468fd: inc word ptr [0x746124]
0x546904: jmp 0x546928
0x546906: pop edi
0x546907: pop esi
0x546908: pop ebp
0x546909: xor al, al
0x54690b: pop ebx
0x54690c: add esp, 0x64
0x54690f: ret 
0x546910: mov eax, dword ptr [0x74610c]
0x546915: mov ecx, dword ptr [eax]
0x546917: push ebx
0x546918: lea edi, [esi + 0x670]
0x54691e: push edi
0x54691f: lea edx, [esp + 0x58]
0x546923: push edx
0x546924: push eax
0x546925: call dword ptr [ecx + 0xc]
0x546928: cmp eax, ebx
0x54692a: jl 0x546a51
0x546930: cmp word ptr [esp + 0x10], bx
0x546935: je 0x546a07
0x54693b: mov edi, dword ptr [edi]
0x54693d: mov ecx, dword ptr [edi]
0x54693f: lea eax, [esi + 0x674]
0x546945: push eax
0x546946: push 0x64e21c
0x54694b: push edi
0x54694c: mov dword ptr [esp + 0x1c], eax
0x546950: call dword ptr [ecx]
0x546952: test eax, eax
0x546954: jl 0x5469ff
0x54695a: mov edx, dword ptr [0x696718]
0x546960: xor eax, eax
0x546962: cmp word ptr [0x746130], bx
0x546969: mov ecx, 0xb
0x54696e: lea edi, [esp + 0x24]
0x546972: rep stosd dword ptr es:[edi], eax
0x546974: mov eax, dword ptr [edx]
0x546976: mov ecx, dword ptr [edx + 4]
0x546979: mov edx, dword ptr [edx + 8]
0x54697c: mov dword ptr [esp + 0x30], eax
0x546980: mov dword ptr [esp + 0x34], ecx
0x546984: mov dword ptr [esp + 0x38], edx
0x546988: jne 0x5469b3
0x54698a: mov eax, dword ptr [esp + 0x78]
0x54698e: mov edi, esi
0x546990: mov word ptr [0x746130], 2
0x546999: call 0x551270
0x54699e: cmp eax, -1
0x5469a1: mov dword ptr [0x74612c], eax
0x5469a6: je 0x5469b3
0x5469a8: mov word ptr [0x746130], 1
0x5469b1: jmp 0x5469bd
0x5469b3: cmp word ptr [0x746130], 1
0x5469bb: jne 0x5469e4
0x5469bd: mov ax, word ptr [esp + 0x78]
0x5469c2: cmp ax, word ptr [0x746124]
0x5469c9: jge 0x5469e4
0x5469cb: mov edx, ebp
0x5469cd: call 0x551460
0x5469d2: test eax, eax
0x5469d4: jne 0x5469e4
0x5469d6: call 0x551420
0x5469db: mov word ptr [0x746130], 2
0x5469e4: mov ecx, dword ptr [esp + 0x78]
0x5469e8: push ebx
0x5469e9: push ebx
0x5469ea: push ebx
0x5469eb: push ebx
0x5469ec: push ecx
0x5469ed: lea edi, [esp + 0x38]
0x5469f1: xor bl, bl
0x5469f3: call 0x5472d0
0x5469f8: add esp, 0x14
0x5469fb: xor ebx, ebx
0x5469fd: jmp 0x546a0d
0x5469ff: mov edx, dword ptr [esp + 0x10]
0x546a03: mov dword ptr [edx], ebx
0x546a05: jmp 0x546a0d
0x546a07: mov dword ptr [esi + 0x674], ebx
0x546a0d: mov ecx, dword ptr [esp + 0x78]
0x546a11: xor eax, eax
0x546a13: mov dword ptr [esp + 0x24], eax
0x546a17: mov dword ptr [esp + 0x28], eax
0x546a1b: mov dword ptr [esp + 0x2c], eax
0x546a1f: mov dword ptr [esp + 0x30], eax
0x546a23: mov dword ptr [esp + 0x34], eax
0x546a27: mov dword ptr [esp + 0x38], eax
0x546a2b: mov dword ptr [esp + 0x3c], eax
0x546a2f: push ebx
0x546a30: push ecx
0x546a31: lea edi, [esp + 0x2c]
0x546a35: mov dword ptr [esp + 0x48], eax
0x546a39: mov dword ptr [esp + 0x2c], 0x3f800000
0x546a41: mov dword ptr [esp + 0x30], 0x3f800000
0x546a49: call 0x5475b0
0x546a4e: add esp, 8
0x546a51: pop edi
0x546a52: pop esi
0x546a53: pop ebp
0x546a54: mov al, 1
0x546a56: pop ebx
0x546a57: add esp, 0x64
0x546a5a: ret 
#endif
