// sound_channel_refresh_cursor  (Ghidra: FUN_00547890)
// address 0x547890, size 46 bytes
// name confidence: 0.4   rewrite confidence: 0.95
// evidence: out/phase4/sound_functions.md summary "Refreshes a channel's play/write cursor via
//   the DirectSoundBuffer interface and returns that interface pointer." (it returns the play
//   cursor, see below); directsound_channel.
//   buffer (0x670, types/sound.h); IDirectSoundBuffer vtable slot 0x10 is GetCurrentPosition.
// register convention: channel index in AX (in_AX).
// blam-cc: AX -> channel_index

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

// blam-cc: AX -> channel_index
// Returns the channel's current hardware play cursor (IDirectSoundBuffer::GetCurrentPosition,
// vtable +0x10; the write cursor output is dropped).
// Phase-4 review (disassembly 0x547890..0x5478bd): the function returns the play cursor it just
// read ([esp] after the call), not the buffer interface pointer as the draft had it.
uint32_t sound_channel_refresh_cursor(int16_t channel_index)
{
    void *buffer = directsound_channels[channel_index].buffer;
    void **vtable = *(void ***)buffer;
    int32_t (__stdcall *get_current_position)(void *, uint32_t *, uint32_t *) =
        (int32_t (__stdcall *)(void *, uint32_t *, uint32_t *))vtable[0x10 / 4];
    uint32_t play_cursor;
    uint32_t write_cursor;

    get_current_position(buffer, &play_cursor, &write_cursor);
    return play_cursor;
}

#if 0
Original Ghidra decompilation (0x547890):

int * FUN_00547890(void)

{
  short in_AX;
  int *piVar1;
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  piVar1 = (int *)(&DAT_00725aa0)[in_AX * 0x19e];
  (**(code **)(*piVar1 + 0x10))(piVar1,local_8,local_4);
  return piVar1;
}

Disassembly (0x547890..0x5478be, capstone; phase-4 review):

0x547890: movsx eax, ax
0x547893: imul eax, eax, 0x678
0x547899: sub esp, 8
0x54789c: lea edx, [esp + 4]
0x5478a0: push edx
0x5478a1: add eax, 0x725430
0x5478a6: mov eax, dword ptr [eax + 0x670]
0x5478ac: mov ecx, dword ptr [eax]
0x5478ae: lea edx, [esp + 4]
0x5478b2: push edx
0x5478b3: push eax
0x5478b4: call dword ptr [ecx + 0x10]
0x5478b7: mov eax, dword ptr [esp]
0x5478ba: add esp, 8
0x5478bd: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
