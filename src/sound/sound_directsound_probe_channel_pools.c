// sound_directsound_probe_channel_pools  (Ghidra: FUN_00545a30, merged with the tail Ghidra split
//   off as its own "function" at 0x545b70 -- see out/phase4/sound_types_notes.md: "0x545b70
//   shell_get_command_line_argument: not a shell function and not a real entry. It is the tail of
//   FUN_00545a30 (0x545a30 + 320 = 0x545b70) that Ghidra split off: it continues the same EBP
//   frame, the same DSBUFFERDESC builds and releases the same local interface arrays." That
//   address is not rewritten as its own file; its logic is folded into this one and it is listed
//   as a skip in this batch's summary.)
// address 0x545a30, size 320 bytes (+ 0x545b70, 673 bytes, folded in: 993 bytes total)
// VERIFIED against disassembly 0x545a30..0x545b70 (2026-09-30). 0x545a30..0x545e0f plus the folded tail; the 3D pool
//   stores its buffers one slot above the array base (see the call)
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Allocates the full set of DirectSound channel
//   buffer pools at startup and releases any interfaces left over from a failed allocation."
//   (0x545a30's own summary, "Creates several pools ... selected by bitmask, recording counts
//   into the caller's output parameters", describes the same code before the split); every
//   created buffer is stored only in this function's own local arrays and unconditionally
//   released before it returns, with the *counts* being the only externally observable output --
//   i.e. this is a hardware capability probe ("how many buffers of each type can the driver
//   actually create"), not a real allocator. DirectSound vtable slot 0xc is
//   IDirectSound::CreateSoundBuffer (pDesc, ppBuffer, pUnkOuter); vtable slot 0 (via the created
//   buffer) is IUnknown::QueryInterface; vtable slot 8 is IUnknown::Release. The constants written
//   into the two stack structs decode cleanly against types/sound.h sound_buffer_description /
//   sound_wave_format: size 0x24; buffer_bytes == average_bytes_per_second (a one-second buffer);
//   format_tag 1 (PCM), channels 1 or 2, bits_per_sample 16; sample rates 22050 (mono/mono-3d/
//   stereo) and 44100 (the "44k stereo" pool only sample rate, and every pool's average_bytes_per
//   _second = samples_per_second * block_align).
// register convention: plain __cdecl; four (out_count*, requested_count) pairs and a bitmask of
//   which pools to probe, all as recognized stack parameters.
// blam-cc: stack -> (mono3d_count, mono3d_requested, mono_count, mono_requested, stereo_count,
//   stereo_requested, stereo44k_count, stereo44k_requested, pool_mask)
// NOTE: the exact DSBUFFERDESC.flags values (0x180b4 for the 3D-mono pool, 0x180a4 for the
//   other three) are preserved as literal constants; types/sound.h's own speculative comment on
//   this field ("0x100a0 / 0x100a8 | 0x10 (3D) | 0x200") does not match these and is superseded
//   here by the values actually read from this decompile. DAT_0064e21c (queried only for the
//   mono-3d pool) is IID_IDirectSound3DBuffer (confirmed in the Phase-4 review below).
// Phase-4 review (disassembly appended below): the two local arrays hold 77 entries (the
//   release loop stops at 0x134 bytes; the draft used 79); each pool's buffers continue at the
//   index where the previous pool stopped, and a 3D buffer whose QueryInterface failed is left
//   in its slot for the next pool to overwrite (leaked), as in the binary. 0x0064e21c is
//   IID_IDirectSound3DBuffer {279afa86-4981-11ce-a521-0020af0be560}. The only caller is the
//   DirectSound driver's initialize (0x545e20, sound_driver.initialize), which Ghidra never made
//   a function of and which is outside the 134-function list.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *directsound;      // 0x0074610c, IDirectSound*
extern uint8_t iid_directsound_3d_buffer[16]; // 0x0064e21c, IID_IDirectSound3DBuffer

typedef struct { void **vtable; } com_object;

#define k_probe_pool_capacity 77 // 0x134 bytes per local array, the release loop's bound

// Probes up to `requested` buffers of one DirectSound format, counting successes into
// `*out_count`; when `query_3d` is set, also QueryInterfaces each created buffer for
// IDirectSound3DBuffer (a pool member that fails the query is still tracked for release, but not
// counted). Every created interface (and, for the 3D pool, every queried one) is released before
// returning; this function is purely a capability probe.
static void sound_directsound_probe_pool(int32_t *out_count, int32_t requested, uint32_t samples_per_second,
    uint16_t channels, uint16_t block_align, uint32_t flags, uint8_t query_3d,
    com_object **buffers, com_object **buffers_3d)
{
    sound_wave_format format;
    sound_buffer_description desc;
    int32_t i;

    format.format_tag = 1;
    format.channels = channels;
    format.samples_per_second = samples_per_second;
    format.average_bytes_per_second = samples_per_second * block_align;
    format.block_align = block_align;
    format.bits_per_sample = 16;
    format.extra_size = 0;

    desc.size = 0x24;
    desc.flags = flags;
    desc.buffer_bytes = format.average_bytes_per_second;
    desc.reserved = 0;
    desc.format = &format;
    desc.algorithm_3d[0] = 0;
    desc.algorithm_3d[1] = 0;
    desc.algorithm_3d[2] = 0;
    desc.algorithm_3d[3] = 0;

    for (i = 0; i < requested; i++) {
        void **directsound_vtable = *(void ***)directsound;
        int32_t (__stdcall *create_sound_buffer)(void *, sound_buffer_description *, com_object **, void *) =
            (int32_t (__stdcall *)(void *, sound_buffer_description *, com_object **, void *))directsound_vtable[3];
        int32_t hr = create_sound_buffer(directsound, &desc, &buffers[i], (void *)0);

        if (hr != 0) {
            break;
        }

        if (query_3d) {
            int32_t (__stdcall *query_interface)(com_object *, uint8_t *, com_object **) =
                (int32_t (__stdcall *)(com_object *, uint8_t *, com_object **))buffers[i]->vtable[0];
            hr = query_interface(buffers[i], iid_directsound_3d_buffer, &buffers_3d[i]);
            if (hr != 0) {
                break;
            }
        }

        *out_count += 1;
    }
}

// blam-cc: stack -> (mono3d_count, mono3d_requested, mono_count, mono_requested, stereo_count,
// stereo_requested, stereo44k_count, stereo44k_requested, pool_mask)
// Probes the DirectSound driver's support for up to four channel-buffer pools (mono 3D, mono,
// stereo, and 44kHz stereo), selected by `pool_mask` bits 0x02/0x04/0x10/0x100, writing how many
// buffers of each type could actually be created. Every probed interface is released before
// returning; nothing is kept.
void sound_directsound_probe_channel_pools(int32_t *mono3d_count, uint32_t mono3d_requested,
    int32_t *mono_count, uint32_t mono_requested, int32_t *stereo_count, uint32_t stereo_requested,
    int32_t *stereo44k_count, uint32_t stereo44k_requested, uint32_t pool_mask)
{
    com_object *buffers[k_probe_pool_capacity];
    com_object *buffers_3d[k_probe_pool_capacity];
    int32_t used = 0;
    int32_t i;

    for (i = 0; i < k_probe_pool_capacity; i++) {
        buffers[i] = (com_object *)0;
        buffers_3d[i] = (com_object *)0;
    }

    if ((pool_mask & 2) != 0 && mono3d_requested != 0) {
        int32_t count = 0;
        // 0x545b19: the 3D pool stores its buffers at [esp + i*4 + 0x4c], one slot above the array base 0x48 the other
        //   pools index from, so the next pool's first buffer overwrites (leaks) this pool's last one, as in the binary
        sound_directsound_probe_pool(&count, (int32_t)mono3d_requested, 22050, 1, 2, 0x180b4, 1,
            buffers + 1, buffers_3d);
        *mono3d_count += count;
        used += count;
    }

    if ((pool_mask & 4) != 0 && mono_requested != 0) {
        int32_t count = 0;
        sound_directsound_probe_pool(&count, (int32_t)mono_requested, 22050, 1, 2, 0x180a4, 0,
            buffers + used, buffers_3d);
        *mono_count += count;
        used += count;
    }

    if ((pool_mask & 0x10) != 0 && stereo_requested != 0) {
        int32_t count = 0;
        sound_directsound_probe_pool(&count, (int32_t)stereo_requested, 22050, 2, 4, 0x180a4, 0,
            buffers + used, buffers_3d);
        *stereo_count += count;
        used += count;
    }

    if ((pool_mask & 0x100) != 0 && stereo44k_requested != 0) {
        int32_t count = 0;
        sound_directsound_probe_pool(&count, (int32_t)stereo44k_requested, 44100, 2, 4, 0x180a4, 0,
            buffers + used, buffers_3d);
        *stereo44k_count += count;
        used += count;
    }

    for (i = 0; i < k_probe_pool_capacity; i++) {
        if (buffers_3d[i] != (com_object *)0) {
            void (__stdcall *release)(com_object *) = (void (__stdcall *)(com_object *))buffers_3d[i]->vtable[2];
            release(buffers_3d[i]);
            buffers_3d[i] = (com_object *)0;
        }
        if (buffers[i] != (com_object *)0) {
            void (__stdcall *release)(com_object *) = (void (__stdcall *)(com_object *))buffers[i]->vtable[2];
            release(buffers[i]);
            buffers[i] = (com_object *)0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x545a30):

void FUN_00545a30(int *param_1,uint param_2,int *param_3,uint param_4,int *param_5,uint param_6,
                 int *param_7,uint param_8,uint param_9)

{
  undefined4 *puVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int *piVar5;
  uint uVar6;
  undefined4 local_2b4;
  undefined4 local_2b0;
  undefined4 local_2ac;
  undefined4 local_2a8;
  undefined2 local_2a4;
  undefined4 local_2a0;
  undefined4 local_29c;
  undefined4 local_298;
  undefined4 local_294;
  undefined4 *local_290;
  undefined4 local_28c;
  undefined4 local_288;
  undefined4 local_284;
  undefined4 local_280;
  uint local_27c;
  int local_278 [78];
  int local_140 [79];

  uVar4 = 0;
  piVar5 = local_278;
  local_278[0] = 0;
  for (iVar2 = 0x4c; piVar5 = piVar5 + 1, iVar2 != 0; iVar2 = iVar2 + -1) {
    *piVar5 = 0;
  }
  piVar5 = local_140;
  local_140[0] = 0;
  for (iVar2 = 0x4c; piVar5 = piVar5 + 1, iVar2 != 0; iVar2 = iVar2 + -1) {
    *piVar5 = 0;
  }
  uVar3 = 0;
  local_27c = 0;
  if ((param_9 & 2) != 0) {
    local_294 = 0;
    local_28c = 0;
    local_288 = 0;
    local_284 = 0;
    local_280 = 0;
    local_290 = &local_2b4;
    local_298 = 0xac44;
    local_2ac = 0xac44;
    local_29c = 0x180b4;
    local_2a0 = 0x24;
    local_2b4 = 0x10001;
    local_2a8 = 0x100002;
    local_2b0 = 0x5622;
    local_2a4 = 0;
    if (param_2 != 0) {
      do {
        iVar2 = (**(code **)(*DAT_0074610c + 0xc))(DAT_0074610c,&local_2a0,local_278 + uVar4,0);
        if ((iVar2 != 0) ||
           (puVar1 = (undefined4 *)local_278[uVar4],
           iVar2 = (**(code **)*puVar1)(puVar1,&DAT_0064e21c,local_140 + uVar4), iVar2 != 0)) break;
        *param_1 = *param_1 + 1;
        uVar4 = uVar4 + 1;
      } while (uVar4 < param_2);
    }
  }
  if ((param_9 & 4) != 0) {
    local_294 = 0;
    local_28c = 0;
    local_288 = 0;
    local_284 = 0;
    local_280 = 0;
    local_290 = &local_2b4;
    local_298 = 0xac44;
    local_2ac = 0xac44;
    uVar3 = 0;
    local_29c = 0x180a4;
    local_2a0 = 0x24;
    local_2b4 = 0x10001;
    local_2a8 = 0x100002;
    local_2b0 = 0x5622;
    local_2a4 = 0;
    if (param_4 != 0) {
      piVar5 = local_278 + uVar4;
      do {
        iVar2 = (**(code **)(*DAT_0074610c + 0xc))(DAT_0074610c,&local_2a0,piVar5,0);
        if (iVar2 != 0) break;
        *param_3 = *param_3 + 1;
        uVar3 = uVar3 + 1;
        piVar5 = piVar5 + 1;
      } while (uVar3 < param_4);
    }
  }
  if ((param_9 & 0x10) != 0) {
    local_294 = 0;
    local_28c = 0;
    local_288 = 0;
    local_284 = 0;
    local_280 = 0;
    local_290 = &local_2b4;
    local_298 = 0x15888;
    local_2ac = 0x15888;
    local_29c = 0x180a4;
    local_2a0 = 0x24;
    local_2b4 = 0x20001;
    local_2a8 = 0x100004;
    local_2b0 = 0x5622;
    local_2a4 = 0;
    local_27c = 0;
    if (param_6 != 0) {
      piVar5 = local_278 + uVar3 + uVar4;
      do {
        iVar2 = (**(code **)(*DAT_0074610c + 0xc))(DAT_0074610c,&local_2a0,piVar5,0);
        if (iVar2 != 0) break;
        *param_5 = *param_5 + 1;
        local_27c = local_27c + 1;
        piVar5 = piVar5 + 1;
      } while (local_27c < param_6);
    }
  }
  if ((param_9 & 0x100) != 0) {
    local_294 = 0;
    local_28c = 0;
    local_288 = 0;
    local_284 = 0;
    local_280 = 0;
    local_298 = 0x2b110;
    local_2ac = 0x2b110;
    uVar6 = 0;
    local_290 = &local_2b4;
    local_29c = 0x180a4;
    local_2a0 = 0x24;
    local_2b4 = 0x20001;
    local_2a8 = 0x100004;
    local_2b0 = 0xac44;
    local_2a4 = 0;
    if (param_8 != 0) {
      piVar5 = local_278 + local_27c + uVar3 + uVar4;
      do {
        iVar2 = (**(code **)(*DAT_0074610c + 0xc))(DAT_0074610c,&local_2a0,piVar5,0);
        if (iVar2 != 0) break;
        *param_7 = *param_7 + 1;
        uVar6 = uVar6 + 1;
        piVar5 = piVar5 + 1;
      } while (uVar6 < param_8);
    }
  }
  uVar4 = 0;
  do {
    piVar5 = *(int **)((int)local_140 + uVar4);
    if (piVar5 != (int *)0x0) {
      (**(code **)(*piVar5 + 8))(piVar5);
      *(undefined4 *)((int)local_140 + uVar4) = 0;
    }
    piVar5 = *(int **)((int)local_278 + uVar4);
    if (piVar5 != (int *)0x0) {
      (**(code **)(*piVar5 + 8))(piVar5);
      *(undefined4 *)((int)local_278 + uVar4) = 0;
    }
    uVar4 = uVar4 + 4;
  } while (uVar4 < 0x134);
  return;
}

Tail merged in from Ghidra's separately-split "shell_get_command_line_argument" @ 0x545b70
(same EBP frame, same DSBUFFERDESC builds, same release loop -- see file header):

void shell_get_command_line_argument(void)

{
  int *piVar1;
  undefined4 in_EAX;
  int iVar2;
  uint uVar3;
  int unaff_EBP;
  int unaff_ESI;
  undefined1 *puVar4;
  uint uVar5;
  undefined4 uStack0000000c;
  undefined4 uStack00000010;
  undefined4 uStack00000014;
  undefined4 uStack00000018;
  undefined2 uStack0000001c;
  undefined4 uStack00000020;
  undefined4 uStack00000024;
  undefined4 uStack00000028;
  undefined4 in_stack_0000002c;
  undefined1 *puStack00000030;
  undefined4 uStack00000034;
  undefined4 uStack00000038;
  undefined4 uStack0000003c;
  undefined4 uStack00000040;
  uint in_stack_00000044;

  puStack00000030 = (undefined1 *)&stack0x0000000c;
  uStack00000028 = 0xac44;
  uStack00000014 = 0xac44;
  uVar3 = 0;
  uStack00000024 = 0x180a4;
  uStack00000020 = 0x24;
  uStack0000000c = 0x10001;
  uStack00000018 = 0x100002;
  uStack00000010 = 0x5622;
  uStack0000001c = 0;
  uStack00000034 = in_EAX;
  uStack00000038 = in_EAX;
  uStack0000003c = in_EAX;
  uStack00000040 = in_EAX;
  if (*(int *)(unaff_EBP + 0x14) != 0) {
    puVar4 = &stack0x00000048 + unaff_ESI * 4;
    do {
      iVar2 = (**(code **)(*DAT_0074610c + 0xc))(DAT_0074610c,&stack0x00000020,puVar4,0);
      if (iVar2 != 0) break;
      **(int **)(unaff_EBP + 0x10) = **(int **)(unaff_EBP + 0x10) + 1;
      uVar3 = uVar3 + 1;
      puVar4 = puVar4 + 4;
    } while (uVar3 < *(uint *)(unaff_EBP + 0x14));
  }
  if ((*(byte *)(unaff_EBP + 0x28) & 0x10) != 0) {
    in_stack_0000002c = 0;
    uStack00000034 = 0;
    uStack00000038 = 0;
    uStack0000003c = 0;
    uStack00000040 = 0;
    puStack00000030 = (undefined1 *)&stack0x0000000c;
    uStack00000028 = 0x15888;
    uStack00000014 = 0x15888;
    uStack00000024 = 0x180a4;
    uStack00000020 = 0x24;
    uStack0000000c = 0x20001;
    uStack00000018 = 0x100004;
    uStack00000010 = 0x5622;
    uStack0000001c = 0;
    in_stack_00000044 = 0;
    if (*(int *)(unaff_EBP + 0x1c) != 0) {
      puVar4 = &stack0x00000048 + (uVar3 + unaff_ESI) * 4;
      do {
        iVar2 = (**(code **)(*DAT_0074610c + 0xc))(DAT_0074610c,&stack0x00000020,puVar4,0);
        if (iVar2 != 0) break;
        uVar5 = *(uint *)(unaff_EBP + 0x1c);
        **(int **)(unaff_EBP + 0x18) = **(int **)(unaff_EBP + 0x18) + 1;
        in_stack_00000044 = in_stack_00000044 + 1;
        puVar4 = puVar4 + 4;
      } while (in_stack_00000044 < uVar5);
    }
  }
  if ((*(uint *)(unaff_EBP + 0x28) & 0x100) != 0) {
    in_stack_0000002c = 0;
    uStack00000034 = 0;
    uStack00000038 = 0;
    uStack0000003c = 0;
    uStack00000040 = 0;
    uStack00000028 = 0x2b110;
    uStack00000014 = 0x2b110;
    uVar5 = 0;
    puStack00000030 = (undefined1 *)&stack0x0000000c;
    uStack00000024 = 0x180a4;
    uStack00000020 = 0x24;
    uStack0000000c = 0x20001;
    uStack00000018 = 0x100004;
    uStack00000010 = 0xac44;
    uStack0000001c = 0;
    if (*(int *)(unaff_EBP + 0x24) != 0) {
      puVar4 = &stack0x00000048 + (in_stack_00000044 + uVar3 + unaff_ESI) * 4;
      do {
        iVar2 = (**(code **)(*DAT_0074610c + 0xc))(DAT_0074610c,&stack0x00000020,puVar4,0);
        if (iVar2 != 0) break;
        **(int **)(unaff_EBP + 0x20) = **(int **)(unaff_EBP + 0x20) + 1;
        uVar5 = uVar5 + 1;
        puVar4 = puVar4 + 4;
      } while (uVar5 < *(uint *)(unaff_EBP + 0x24));
    }
  }
  uVar3 = 0;
  do {
    piVar1 = *(int **)(&stack0x00000180 + uVar3);
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      *(undefined4 *)(&stack0x00000180 + uVar3) = 0;
    }
    piVar1 = *(int **)(&stack0x00000048 + uVar3);
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      *(undefined4 *)(&stack0x00000048 + uVar3) = 0;
    }
    uVar3 = uVar3 + 4;
  } while (uVar3 < 0x134);
  return;
}

Disassembly (0x545a30..0x545e13, capstone; phase-4 review):

0x545a30: push ebp
0x545a31: mov ebp, esp
0x545a33: and esp, 0xfffffff8
0x545a36: sub esp, 0x2ac
0x545a3c: push ebx
0x545a3d: push esi
0x545a3e: push edi
0x545a3f: xor esi, esi
0x545a41: xor eax, eax
0x545a43: mov ecx, 0x4c
0x545a48: lea edi, [esp + 0x4c]
0x545a4c: mov dword ptr [esp + 0x48], esi
0x545a50: rep stosd dword ptr es:[edi], eax
0x545a52: mov ecx, 0x4c
0x545a57: lea edi, [esp + 0x184]
0x545a5e: mov dword ptr [esp + 0x180], esi
0x545a65: rep stosd dword ptr es:[edi], eax
0x545a67: mov al, byte ptr [ebp + 0x28]
0x545a6a: xor ebx, ebx
0x545a6c: test al, 2
0x545a6e: mov dword ptr [esp + 0x44], esi
0x545a72: je 0x545b50
0x545a78: xor eax, eax
0x545a7a: mov dword ptr [esp + 0x20], eax
0x545a7e: mov dword ptr [esp + 0x24], eax
0x545a82: mov dword ptr [esp + 0x28], eax
0x545a86: mov dword ptr [esp + 0x2c], eax
0x545a8a: mov dword ptr [esp + 0x30], eax
0x545a8e: mov dword ptr [esp + 0x34], eax
0x545a92: xor edx, edx
0x545a94: mov dword ptr [esp + 0x38], eax
0x545a98: mov dword ptr [esp + 0xc], edx
0x545a9c: mov dword ptr [esp + 0x3c], eax
0x545aa0: mov dword ptr [esp + 0x10], edx
0x545aa4: mov dword ptr [esp + 0x40], eax
0x545aa8: mov eax, 0xac44
0x545aad: lea ecx, [esp + 0xc]
0x545ab1: mov dword ptr [esp + 0x14], edx
0x545ab5: mov dword ptr [esp + 0x18], edx
0x545ab9: mov dword ptr [esp + 0x28], eax
0x545abd: mov dword ptr [esp + 0x14], eax
0x545ac1: cmp dword ptr [ebp + 0xc], esi
0x545ac4: mov dword ptr [esp + 0x30], ecx
0x545ac8: mov ecx, 1
0x545acd: mov word ptr [esp + 0x1c], dx
0x545ad2: mov dword ptr [esp + 0x24], 0x180b4
0x545ada: mov dword ptr [esp + 0x20], 0x24
0x545ae2: mov word ptr [esp + 0xc], cx
0x545ae7: mov word ptr [esp + 0xe], cx
0x545aec: mov word ptr [esp + 0x1a], 0x10
0x545af3: mov word ptr [esp + 0x18], 2
0x545afa: mov dword ptr [esp + 0x10], 0x5622
0x545b02: mov word ptr [esp + 0x1c], si
0x545b07: jbe 0x545b50
0x545b09: lea esp, [esp]
0x545b10: mov eax, dword ptr [0x74610c]
0x545b15: mov ecx, dword ptr [eax]
0x545b17: push 0
0x545b19: lea edi, [esp + esi*4 + 0x4c]
0x545b1d: push edi
0x545b1e: lea edx, [esp + 0x28]
0x545b22: push edx
0x545b23: push eax
0x545b24: call dword ptr [ecx + 0xc]
0x545b27: test eax, eax
0x545b29: jne 0x545b50
0x545b2b: mov eax, dword ptr [edi]
0x545b2d: mov ecx, dword ptr [eax]
0x545b2f: lea edx, [esp + esi*4 + 0x180]
0x545b36: push edx
0x545b37: push 0x64e21c
0x545b3c: push eax
0x545b3d: call dword ptr [ecx]
0x545b3f: test eax, eax
0x545b41: jne 0x545b50
0x545b43: mov eax, dword ptr [ebp + 8]
0x545b46: inc dword ptr [eax]
0x545b48: mov eax, dword ptr [ebp + 0xc]
0x545b4b: inc esi
0x545b4c: cmp esi, eax
0x545b4e: jb 0x545b10
0x545b50: test byte ptr [ebp + 0x28], 4
0x545b54: je 0x545c1a
0x545b5a: xor eax, eax
0x545b5c: mov dword ptr [esp + 0x20], eax
0x545b60: mov dword ptr [esp + 0x24], eax
0x545b64: mov dword ptr [esp + 0x28], eax
0x545b68: mov dword ptr [esp + 0x2c], eax
0x545b6c: mov dword ptr [esp + 0x30], eax
0x545b70: mov dword ptr [esp + 0x34], eax
0x545b74: xor edx, edx
0x545b76: mov dword ptr [esp + 0x38], eax
0x545b7a: mov dword ptr [esp + 0xc], edx
0x545b7e: mov dword ptr [esp + 0x3c], eax
0x545b82: mov dword ptr [esp + 0x10], edx
0x545b86: mov dword ptr [esp + 0x40], eax
0x545b8a: mov eax, 0xac44
0x545b8f: lea ecx, [esp + 0xc]
0x545b93: mov dword ptr [esp + 0x14], edx
0x545b97: mov dword ptr [esp + 0x18], edx
0x545b9b: mov dword ptr [esp + 0x28], eax
0x545b9f: mov dword ptr [esp + 0x14], eax
0x545ba3: mov eax, dword ptr [ebp + 0x14]
0x545ba6: xor ebx, ebx
0x545ba8: cmp eax, ebx
0x545baa: mov dword ptr [esp + 0x30], ecx
0x545bae: mov ecx, 1
0x545bb3: mov word ptr [esp + 0x1c], dx
0x545bb8: mov dword ptr [esp + 0x24], 0x180a4
0x545bc0: mov dword ptr [esp + 0x20], 0x24
0x545bc8: mov word ptr [esp + 0xc], cx
0x545bcd: mov word ptr [esp + 0xe], cx
0x545bd2: mov word ptr [esp + 0x1a], 0x10
0x545bd9: mov word ptr [esp + 0x18], 2
0x545be0: mov dword ptr [esp + 0x10], 0x5622
0x545be8: mov word ptr [esp + 0x1c], bx
0x545bed: jbe 0x545c1a
0x545bef: lea edi, [esp + esi*4 + 0x48]
0x545bf3: mov eax, dword ptr [0x74610c]
0x545bf8: mov ecx, dword ptr [eax]
0x545bfa: push 0
0x545bfc: push edi
0x545bfd: lea edx, [esp + 0x28]
0x545c01: push edx
0x545c02: push eax
0x545c03: call dword ptr [ecx + 0xc]
0x545c06: test eax, eax
0x545c08: jne 0x545c1a
0x545c0a: mov eax, dword ptr [ebp + 0x10]
0x545c0d: inc dword ptr [eax]
0x545c0f: mov eax, dword ptr [ebp + 0x14]
0x545c12: inc ebx
0x545c13: add edi, 4
0x545c16: cmp ebx, eax
0x545c18: jb 0x545bf3
0x545c1a: test byte ptr [ebp + 0x28], 0x10
0x545c1e: je 0x545cf5
0x545c24: xor eax, eax
0x545c26: mov dword ptr [esp + 0x20], eax
0x545c2a: mov dword ptr [esp + 0x24], eax
0x545c2e: mov dword ptr [esp + 0x28], eax
0x545c32: mov dword ptr [esp + 0x2c], eax
0x545c36: mov dword ptr [esp + 0x30], eax
0x545c3a: mov dword ptr [esp + 0x34], eax
0x545c3e: xor edx, edx
0x545c40: mov dword ptr [esp + 0x38], eax
0x545c44: mov dword ptr [esp + 0xc], edx
0x545c48: mov dword ptr [esp + 0x3c], eax
0x545c4c: mov dword ptr [esp + 0x10], edx
0x545c50: mov dword ptr [esp + 0x40], eax
0x545c54: mov eax, 0x15888
0x545c59: lea ecx, [esp + 0xc]
0x545c5d: mov dword ptr [esp + 0x14], edx
0x545c61: mov dword ptr [esp + 0x18], edx
0x545c65: mov dword ptr [esp + 0x28], eax
0x545c69: mov dword ptr [esp + 0x30], ecx
0x545c6d: mov ecx, dword ptr [ebp + 0x1c]
0x545c70: mov dword ptr [esp + 0x14], eax
0x545c74: xor eax, eax
0x545c76: cmp ecx, eax
0x545c78: mov word ptr [esp + 0x1c], dx
0x545c7d: mov dword ptr [esp + 0x24], 0x180a4
0x545c85: mov dword ptr [esp + 0x20], 0x24
0x545c8d: mov word ptr [esp + 0xc], 1
0x545c94: mov word ptr [esp + 0xe], 2
0x545c9b: mov word ptr [esp + 0x1a], 0x10
0x545ca2: mov word ptr [esp + 0x18], 4
0x545ca9: mov dword ptr [esp + 0x10], 0x5622
0x545cb1: mov word ptr [esp + 0x1c], ax
0x545cb6: mov dword ptr [esp + 0x44], eax
0x545cba: jbe 0x545cf5
0x545cbc: lea edi, [ebx + esi]
0x545cbf: lea edi, [esp + edi*4 + 0x48]
0x545cc3: mov eax, dword ptr [0x74610c]
0x545cc8: mov ecx, dword ptr [eax]
0x545cca: push 0
0x545ccc: push edi
0x545ccd: lea edx, [esp + 0x28]
0x545cd1: push edx
0x545cd2: push eax
0x545cd3: call dword ptr [ecx + 0xc]
0x545cd6: test eax, eax
0x545cd8: jne 0x545cf5
0x545cda: mov eax, dword ptr [ebp + 0x18]
0x545cdd: mov edx, dword ptr [eax]
0x545cdf: mov ecx, dword ptr [ebp + 0x1c]
0x545ce2: inc edx
0x545ce3: mov dword ptr [eax], edx
0x545ce5: mov eax, dword ptr [esp + 0x44]
0x545ce9: inc eax
0x545cea: add edi, 4
0x545ced: cmp eax, ecx
0x545cef: mov dword ptr [esp + 0x44], eax
0x545cf3: jb 0x545cc3
0x545cf5: mov eax, dword ptr [ebp + 0x28]
0x545cf8: test ah, 1
0x545cfb: je 0x545dc9
0x545d01: xor eax, eax
0x545d03: mov dword ptr [esp + 0x20], eax
0x545d07: mov dword ptr [esp + 0x24], eax
0x545d0b: mov dword ptr [esp + 0x28], eax
0x545d0f: mov dword ptr [esp + 0x2c], eax
0x545d13: mov dword ptr [esp + 0x30], eax
0x545d17: mov dword ptr [esp + 0x34], eax
0x545d1b: xor edx, edx
0x545d1d: mov dword ptr [esp + 0x38], eax
0x545d21: mov dword ptr [esp + 0xc], edx
0x545d25: mov dword ptr [esp + 0x3c], eax
0x545d29: mov dword ptr [esp + 0x10], edx
0x545d2d: mov dword ptr [esp + 0x40], eax
0x545d31: mov eax, 0x2b110
0x545d36: mov dword ptr [esp + 0x14], edx
0x545d3a: mov dword ptr [esp + 0x18], edx
0x545d3e: mov dword ptr [esp + 0x28], eax
0x545d42: mov dword ptr [esp + 0x14], eax
0x545d46: mov eax, dword ptr [ebp + 0x24]
0x545d49: xor edi, edi
0x545d4b: cmp eax, edi
0x545d4d: lea ecx, [esp + 0xc]
0x545d51: mov word ptr [esp + 0x1c], dx
0x545d56: mov dword ptr [esp + 0x24], 0x180a4
0x545d5e: mov dword ptr [esp + 0x20], 0x24
0x545d66: mov dword ptr [esp + 0x30], ecx
0x545d6a: mov word ptr [esp + 0xc], 1
0x545d71: mov word ptr [esp + 0xe], 2
0x545d78: mov word ptr [esp + 0x1a], 0x10
0x545d7f: mov word ptr [esp + 0x18], 4
0x545d86: mov dword ptr [esp + 0x10], 0xac44
0x545d8e: mov word ptr [esp + 0x1c], di
0x545d93: jbe 0x545dc9
0x545d95: mov eax, dword ptr [esp + 0x44]
0x545d99: lea ecx, [eax + ebx]
0x545d9c: add ecx, esi
0x545d9e: lea esi, [esp + ecx*4 + 0x48]
0x545da2: mov eax, dword ptr [0x74610c]
0x545da7: mov edx, dword ptr [eax]
0x545da9: push 0
0x545dab: push esi
0x545dac: lea ecx, [esp + 0x28]
0x545db0: push ecx
0x545db1: push eax
0x545db2: call dword ptr [edx + 0xc]
0x545db5: test eax, eax
0x545db7: jne 0x545dc9
0x545db9: mov eax, dword ptr [ebp + 0x20]
0x545dbc: inc dword ptr [eax]
0x545dbe: mov eax, dword ptr [ebp + 0x24]
0x545dc1: inc edi
0x545dc2: add esi, 4
0x545dc5: cmp edi, eax
0x545dc7: jb 0x545da2
0x545dc9: xor esi, esi
0x545dcb: jmp 0x545dd0
0x545dcd: lea ecx, [ecx]
0x545dd0: mov eax, dword ptr [esp + esi + 0x180]
0x545dd7: test eax, eax
0x545dd9: je 0x545dec
0x545ddb: mov edx, dword ptr [eax]
0x545ddd: push eax
0x545dde: call dword ptr [edx + 8]
0x545de1: mov dword ptr [esp + esi + 0x180], 0
0x545dec: mov eax, dword ptr [esp + esi + 0x48]
0x545df0: test eax, eax
0x545df2: je 0x545e02
0x545df4: mov ecx, dword ptr [eax]
0x545df6: push eax
0x545df7: call dword ptr [ecx + 8]
0x545dfa: mov dword ptr [esp + esi + 0x48], 0
0x545e02: add esi, 4
0x545e05: cmp esi, 0x134
0x545e0b: jb 0x545dd0
0x545e0d: pop edi
0x545e0e: pop esi
0x545e0f: pop ebx
0x545e10: mov esp, ebp
0x545e12: pop ebp
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
