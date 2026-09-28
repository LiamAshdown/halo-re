// sound_channel_restore_buffer  (Ghidra: FUN_00547c10)
// address 0x547c10, size 97 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: out/phase4/sound_functions.md summary "Restores a DirectSound buffer interface and,
//   if requested, spin-waits for a pending operation on it to complete." (the "if requested" is
//   really "if the buffer was lost", see below); IDirectSoundBuffer
//   vtable slot 0x24 is GetStatus, slot 0x50 is Restore; -0x7787ff6a matches DSERR_BUFFERLOST
//   (0x88780096 as a signed HRESULT).
// register convention: buffer pointer in ESI (unaff_ESI), optional "was restored" output flag
//   pointer in EBX (unaff_EBX).
// blam-cc: ESI -> buffer, EBX -> was_restored_out
// Phase-4 review (disassembly appended below): the gate is `status & 2` where status is the
//   DWORD GetStatus (vtable +0x24) wrote into a local, i.e. DSBSTATUS_BUFFERLOST; the draft
//   invented a should_wait parameter for it. Returns 0 after restoring, 1 when the buffer was
//   not lost, the GetStatus HRESULT when that failed, 0x800401f0 for a NULL buffer.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "sound.h"



// blam-cc: ESI -> buffer, EBX -> was_restored_out
// Restores a DirectSound buffer whose memory was lost, retrying (Sleep(0)) while Restore keeps
// answering DSERR_BUFFERLOST.
int32_t sound_channel_restore_buffer(void *buffer, uint8_t *was_restored_out)
{
    void **vtable;
    uint32_t status;
    int32_t hr;

    if (buffer == (void *)0) {
        return (int32_t)0x800401f0;
    }
    if (was_restored_out != (uint8_t *)0) {
        *was_restored_out = 0;
    }

    vtable = *(void ***)buffer;
    hr = ((directsound_buffer_get_status_proc)vtable[0x24 / 4])(buffer, &status);
    if (hr < 0) {
        return hr;
    }
    if ((status & 2) == 0) {
        return 1;
    }

    do {
        if (((directsound_buffer_restore_proc)vtable[0x50 / 4])(buffer) == (int32_t)0x88780096) {
            Sleep(0);
        }
    } while (((directsound_buffer_restore_proc)vtable[0x50 / 4])(buffer) == (int32_t)0x88780096);

    if (was_restored_out != (uint8_t *)0) {
        *was_restored_out = 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x547c10):

int FUN_00547c10(void)

{
  int iVar1;
  undefined1 *unaff_EBX;
  int *unaff_ESI;

  if (unaff_ESI == (int *)0x0) {
    return -0x7ffbfe10;
  }
  if (unaff_EBX != (undefined1 *)0x0) {
    *unaff_EBX = 0;
  }
  iVar1 = (**(code **)(*unaff_ESI + 0x24))();
  if (-1 < iVar1) {
    if (((uint)unaff_ESI & 2) != 0) {
      do {
        iVar1 = (**(code **)(*unaff_ESI + 0x50))();
        if (iVar1 == -0x7787ff6a) {
          Sleep(0);
        }
        iVar1 = (**(code **)(*unaff_ESI + 0x50))();
      } while (iVar1 == -0x7787ff6a);
      if (unaff_EBX != (undefined1 *)0x0) {
        *unaff_EBX = 1;
      }
      return 0;
    }
    iVar1 = 1;
  }
  return iVar1;
}

Disassembly (0x547c10..0x547c71, capstone; phase-4 review):

0x547c10: push ecx
0x547c11: test esi, esi
0x547c13: jne 0x547c1c
0x547c15: mov eax, 0x800401f0
0x547c1a: pop ecx
0x547c1b: ret 
0x547c1c: test ebx, ebx
0x547c1e: je 0x547c23
0x547c20: mov byte ptr [ebx], 0
0x547c23: mov eax, dword ptr [esi]
0x547c25: lea ecx, [esp]
0x547c28: push ecx
0x547c29: push esi
0x547c2a: call dword ptr [eax + 0x24]
0x547c2d: test eax, eax
0x547c2f: jl 0x547c6f
0x547c31: test byte ptr [esp], 2
0x547c35: je 0x547c6a
0x547c37: push edi
0x547c38: mov edi, dword ptr [0x63a29c]
0x547c3e: mov edi, edi
0x547c40: mov edx, dword ptr [esi]
0x547c42: push esi
0x547c43: call dword ptr [edx + 0x50]
0x547c46: cmp eax, 0x88780096
0x547c4b: jne 0x547c51
0x547c4d: push 0
0x547c4f: call edi
0x547c51: mov eax, dword ptr [esi]
0x547c53: push esi
0x547c54: call dword ptr [eax + 0x50]
0x547c57: cmp eax, 0x88780096
0x547c5c: je 0x547c40
0x547c5e: test ebx, ebx
0x547c60: pop edi
0x547c61: je 0x547c66
0x547c63: mov byte ptr [ebx], 1
0x547c66: xor eax, eax
0x547c68: pop ecx
0x547c69: ret 
0x547c6a: mov eax, 1
0x547c6f: pop ecx
0x547c70: ret 
#endif
