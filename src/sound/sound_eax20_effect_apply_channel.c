// sound_eax20_effect_apply_channel  (Ghidra: missed_54f720; 0 callers in this module -- reached
// only through the EAX2 vtable's apply_channel slot, 0x00671d04+0x14)
// address 0x54f720, size 860 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/sound_types_notes.md "slot 2 is called per channel by 0x551480" (the EAX2
//   vtable's apply_channel slot, confirmed by reading .rdata at 0x00671d04+0x14 == 0x0054f720);
//   directsound_channel field offsets (spatialized 0x06, underwater 0x07, eax_value 0x60,
//   obstruction 0x44, occlusion 0x48, types/sound.h) match by offset; identical structure and
//   the same two helpers (sound_gain_to_directsound_volume 0x54ee70, sound_gain_to_millibels
//   0x54eec0, both this module) as sound_eax30_effect_apply_channel.c (0x550890, this module),
//   which folds its near-identical 9 x 2 Set() blocks the same way; sound_eax20_underwater_direct_gain
//   (0x0069ff24, 0.25f) and sound_eax20_buffer_property_guid (0x0064e300, this module)
//   confirmed against the disassembly appended below.
// register convention: __thiscall (ECX -> this_object), stack -> channel_index.
// blam-cc: ECX -> this_object, stack -> channel_index
// Cleanup-pass review: the underwater gain global was 0x0069ff28 in the first draft; objdump
//   0x54f786 reads 0x0069ff24 (same value, separate global). Fixed.
// Phase-4 review: checked instruction by instruction against the disassembly appended below.
// Three of the nine buffer properties (ids 6, 8, 10) are never actually computed from the
// channel: id 6 and id 8 stay at their preset 0, and id 10 stays at its preset ~0.2f
// (0x3e4ccccd) -- confirmed by disassembly (no store to those stack slots outside the initial
// zeroing), matching the same pattern EAX3's apply_channel uses for ids 0x14 and 0xc.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern const uint8_t sound_eax20_buffer_property_guid[16]; // 0x0064e300
extern float sound_eax20_underwater_direct_gain; // 0x0069ff24, 0.25f (not 0x0069ff28, which EAX3 reads)
extern uint8_t directsound_deferred_dirty; // 0x00746132

extern int32_t sound_gain_to_directsound_volume(float gain, int32_t maximum); // this module, 0x54ee70
extern int __cdecl sound_gain_to_millibels(float gain); // this module, 0x54eec0
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

void __thiscall sound_eax20_effect_apply_channel(sound_eax_effect_object *this_object, int32_t channel_index)
{
    directsound_channel *channel_state;
    void *property_set;
    sound_property_set_fn set;
    uint32_t supported;
    uint8_t deferred;
    int32_t underwater_direct, underwater_room;
    int32_t obstruction_at_1000, obstruction_at_0;
    int32_t occlusion_millibels, self_obstruction_millibels;
    int32_t unused_id6 = 0, unused_id8 = 0;
    int32_t id10_value = 0x3e4ccccd; // ~0.2 float bit pattern

    underwater_direct = 0;
    underwater_room = 0;
    obstruction_at_1000 = k_sound_minimum_volume;
    obstruction_at_0 = k_sound_minimum_volume;
    occlusion_millibels = 0;
    self_obstruction_millibels = 0;

    channel_state = &directsound_channels[channel_index];

    if (channel_state->spatialized) {
        float eax_value = channel_state->eax_value;

        if (channel_state->underwater) {
            underwater_direct = sound_gain_to_directsound_volume(sound_eax20_underwater_direct_gain, 1000);
            underwater_room = sound_gain_to_directsound_volume(sound_eax20_underwater_direct_gain, 0);
        }
        obstruction_at_1000 = sound_gain_to_directsound_volume(1.0f - eax_value, 1000);
        obstruction_at_0 = sound_gain_to_directsound_volume(1.0f - eax_value, 0);
        occlusion_millibels = sound_gain_to_millibels(channel_state->occlusion); // id 7
        self_obstruction_millibels = sound_gain_to_millibels(channel_state->obstruction); // id 9
    }

    supported = this_object->base.supported_properties;
    deferred = (supported & 1) != 0;
    property_set = this_object->channel_property_sets[channel_index];
    set = (sound_property_set_fn)(*(void ***)property_set)[4];

    {
        struct { uint32_t bit; uint32_t id; int32_t *value; } fields[9] = {
            {0x00000004, 2,  &underwater_direct},
            {0x00000008, 3,  &underwater_room},
            {0x00000010, 4,  &obstruction_at_1000},
            {0x00000020, 5,  &obstruction_at_0},
            {0x00000040, 6,  &unused_id6},
            {0x00000080, 7,  &occlusion_millibels},
            {0x00000100, 8,  &unused_id8},
            {0x00000200, 9,  &self_obstruction_millibels},
            {0x00000400, 10, &id10_value},
        };
        int32_t i;

        for (i = 0; i < 9; i++) {
            if (supported & fields[i].bit) {
                uint32_t id = deferred ? (fields[i].id | 0x80000000u) : fields[i].id;
                set(property_set, sound_eax20_buffer_property_guid, id, 0, 0, fields[i].value, 4);
            }
        }
    }

    directsound_deferred_dirty = 1;
}

#if 0
Original Ghidra decompilation (0x54f720):

/* WARNING: Type propagation algorithm not settling */

void missed_54f720(int param_1)

{
  float fVar1;
  uint uVar2;
  int *piVar3;
  undefined4 uVar4;
  int in_ECX;
  int iVar5;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  int local_14;
  int local_10 [4];

  uVar4 = DAT_0069ff24;
  iVar5 = param_1 * 0x678;
  local_1c = 0xffffd8f0;
  local_18 = 0xffffd8f0;
  local_14 = 0;
  local_10[0] = 0;
  local_24 = 0;
  local_20 = 0;
  local_10[1] = 0;
  local_10[2] = 0;
  local_10[3] = 0x3e4ccccd;
  if ((&DAT_00725436)[iVar5] != '\0') {
    fVar1 = *(float *)(&DAT_00725490 + iVar5);
    if ((&DAT_00725437)[iVar5] != '\0') {
      local_20 = sound_gain_to_directsound_volume(DAT_0069ff24,0);
      local_24 = sound_gain_to_directsound_volume(uVar4,1000);
    }
    local_1c = sound_gain_to_directsound_volume(1.0 - fVar1,1000);
    local_18 = sound_gain_to_directsound_volume(1.0 - fVar1,0);
    local_14 = sound_gain_to_millibels(*(float *)(&DAT_00725478 + iVar5));
    local_10[0] = sound_gain_to_millibels(*(float *)(&DAT_00725474 + iVar5));
  }
  uVar2 = *(uint *)(in_ECX + 8);
  if ((uVar2 & 1) == 0) {
    if ((uVar2 & 4) != 0) { ...set(...,2,...,&local_24,4); }
    if ((*(byte *)(in_ECX + 8) & 8) != 0) { ...set(...,3,...,&local_20,4); }
    if ((*(byte *)(in_ECX + 8) & 0x10) != 0) { ...set(...,4,...,&local_1c,4); }
    if ((*(byte *)(in_ECX + 8) & 0x20) != 0) { ...set(...,5,...,&local_18,4); }
    if ((*(byte *)(in_ECX + 8) & 0x40) != 0) { ...set(...,6,...,local_10 + 1,4); }
    if (*(char *)(in_ECX + 8) < '\0') { ...set(...,7,...,&local_14,4); }
    if ((*(uint *)(in_ECX + 8) & 0x100) != 0) { ...set(...,8,...,local_10 + 2,4); }
    if ((*(uint *)(in_ECX + 8) & 0x200) != 0) { ...set(...,9,...,local_10,4); }
    if ((*(uint *)(in_ECX + 8) & 0x400) != 0) { ...set(...,10,...,local_10 + 3,4); }
  }
  else {
    /* same nine tests, with id | 0x80000000 */
  }
  DAT_00746132 = 1;
  return;
}

Disassembly (0x54f720..0x54fa7c, condensed; phase-4 review): every Set() call has the shape
  mov eax, [esi+edi*4+0x1c]   ; this->channel_property_sets[channel_index]
  mov ecx/edx, [eax]          ; vtable
  push 4 ; lea T,[esp+N] ; push T ; push ebx ; push ebx ; push id(|0x80000000 if deferred)
  push 0x64e300 ; push eax ; call [ecx/edx + 0x10]
repeated for ids 2,3,4,5,6,7 (tested via `test al/ah,bit` or `test al,al;jns` for the id-7 sign
bit),8,9,10 in the non-deferred block at 0x54f7e6..0x54f935 (guarded by `test al,1; je 0x54f946`),
then again for the deferred block (ids | 0x80000000) at 0x54f946..0x54fa6b; both paths converge on
0x54fa6e: mov byte ptr [0x746132], 1 ; ret 4. The gain/millibel setup (0x54f720..0x54f7e6) matches
the decompiled block above exactly, including the three fields (local_10[1], local_10[2],
local_10[3]) that keep their preset defaults because nothing in that block stores to them.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
