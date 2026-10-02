// game_looping_sound_update  (Ghidra: FUN_00544330)
// address 0x544330, size 655 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: out/phase4/sound_functions.md summary "Runs the full per-tick update for one
//   looping-sound datum: audibility/zone check, world-position computation, and start, update, or
//   delete of its playback channel."; field offsets match game_looping_sound (last_update 0x14,
//   flags 0x04, function_index 0x18, object_index 0x10, node_index 0x1a, position 0x1c,
//   forward 0x28, state 0x02, scale 0x08) and object.function_out_values/function_valid_flags
//   (0x134/0x123, types/objects.h). The stack block at [esp+0x24] is a sound_location.
// Phase-4 review: re-derived from the disassembly below. Fixes against the earlier draft: the
//   computed gain value is stored into location.scale (the draft dropped it); the node matrix
//   transforms the datum's own node-space position/forward (0x1c/0x28), not the global
//   origin/forward vectors; 0x4f6aa0 is object_get_root_object_velocities writing the root
//   object's velocity into location+0x24; sound_looping_set_state receives the handle in EAX
//   and the definition in ECX.
// register convention: stack -> (looping_sound_index, root_location), where root_location is the
//   {leaf, cluster} pair game_sound_update got from object_get_root_location.
// obstruction/occlusion (location 0x38/0x3c) are never written here (stack garbage in the
//   binary; sound_play_new-created sounds overwrite them). Zeroed for determinism.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *game_looping_sound_data;        // 0x007461a0
extern tag_instance *tag_instances;                 // 0x0087bc14
extern game_sound_globals *game_sound_globals_ptr;  // 0x007461a4
extern data_array *object_data;                     // 0x008603b0

extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0, blam-cc: EAX out, EDX point
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cbec0, blam-cc: EAX out, EDX normal
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity); // 0x4f6aa0, blam-cc: EAX -> object_index, ESI -> out_velocity, EDI -> out_angular_velocity
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array, EDX -> handle
extern uint8_t sound_looping_set_state(int32_t owner, datum_index definition_index,
    sound_location *location, int16_t state, uint8_t alternate, float fade_duration); // 0x549fa0, blam-cc: EAX, ECX, stack

// blam-cc: stack -> (looping_sound_index, root_location)
// Per-tick update of one game_looping_sound datum: reads its gain (object function value or
// script scale) and liveness, places it on its object's node, and starts, continues or stops
// the underlying looping_sound through sound_looping_set_state. Script sounds are deleted once
// their looping_sound reports it is gone.
void game_looping_sound_update(datum_index looping_sound_index, int32_t *root_location)
{
    game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[looping_sound_index & 0xffff];
    SoundLooping *definition = (SoundLooping *)tag_instances[self->definition_index & 0xffff].data;
    uint8_t stale = self->last_update == -1 || self->last_update == game_sound_globals_ptr->update_count - 1;
    uint32_t flags = self->flags;
    uint8_t alternate = (uint8_t)((flags >> 3) & 1);
    sound_location location;
    uint8_t live;

    if ((flags & _game_looping_sound_script_gain_bit) == 0) {
        object *obj = ((object_header *)object_data->data)[self->object_index & 0xffff].data;

        if (self->function_index == -1) {
            location.scale = 1.0f;
            live = 1;
        } else {
            location.scale = obj->function_out_values[self->function_index];
            live = (obj->function_valid_flags & (uint8_t)(1 << (self->function_index & 0x1f))) != 0;
        }
    } else {
        location.scale = self->scale;
        live = (uint8_t)(~(flags >> 1) & 1);
    }

    if (!live) {
        if (self->state == _game_looping_sound_stopped) {
            goto done;
        }
        if (!stale) {
            self->state = _game_looping_sound_stopped;
            goto done;
        }
    }

    location.obstruction = 0.0f;
    location.occlusion = 0.0f;
    if (self->object_index != k_datum_index_none) {
        object *obj = ((object_header *)object_data->data)[self->object_index & 0xffff].data;
        real_matrix4x3 *node_matrix = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset + self->node_index * 0x34);

        matrix4x3_transform_point((real_point3d *)&location.position, (real_point3d *)&self->position, node_matrix);
        matrix4x3_transform_normal((real_vector3d *)&location.forward, (real_vector3d *)&self->forward, node_matrix);
        object_get_root_object_velocities(self->object_index, (real_vector3d *)&location.velocity,
            (real_vector3d *)0); // binary passes a scratch vector for the angular velocity
        location.leaf_index = root_location[0];
        *(int32_t *)&location.cluster_index = root_location[1];
        location.type = _sound_location_absolute;
    } else {
        location.type = _sound_location_none;
    }
    location.gain = 1.0f;

    if (live) {
        int16_t new_state = (self->state == _game_looping_sound_playing || !stale) ? 1 : 0;

        self->state = _game_looping_sound_playing;
        if (sound_looping_set_state((int32_t)looping_sound_index, self->definition_index, &location,
                new_state, alternate, 0.0f) == 0) {
            goto done;
        }
        if ((self->flags & _game_looping_sound_script_gain_bit) == 0) {
            self->state = _game_looping_sound_stopped;
            goto done;
        }
        if (*(uint32_t *)&definition->runtime_scripting_sound == (uint32_t)looping_sound_index) {
            *(uint32_t *)&definition->runtime_scripting_sound = 0xffffffff;
        }
        datum_delete(game_looping_sound_data, looping_sound_index);
        goto done;
    }

    // not live but still inside its one-update grace period: stop it (4 s fade when a music
    // loop pushed it out)
    if (sound_looping_set_state((int32_t)looping_sound_index, self->definition_index, &location,
            2, alternate, (flags & _game_looping_sound_stopped_by_music_bit) != 0 ? 4.0f : 0.0f) == 0) {
        self->state = _game_looping_sound_stopping;
        goto done;
    }
    if ((self->flags & _game_looping_sound_script_gain_bit) == 0) {
        self->state = _game_looping_sound_stopped;
        goto done;
    }
    datum_delete(game_looping_sound_data, looping_sound_index);

done:
    // the binary stamps last_update even into a slot it has just deleted
    self->last_update = game_sound_globals_ptr->update_count;
}

#if 0
Original Ghidra decompilation (0x544330):

void FUN_00544330(uint param_1,undefined4 *param_2)

{
  short sVar1;
  uint uVar2;
  uint uVar3;
  bool bVar4;
  int *piVar5;
  char cVar6;
  undefined4 uVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  bool bVar11;
  undefined4 local_50;
  ushort local_40 [2];
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_10;
  undefined4 local_c;

  iVar8 = (param_1 & 0xffff) * 0x34;
  iVar9 = iVar8 + *(int *)(DAT_007461a0 + 0x34);
  iVar8 = *(int *)((*(uint *)(iVar8 + 0xc + *(int *)(DAT_007461a0 + 0x34)) & 0xffff) * 0x20 + 0x14 +
                  DAT_0087bc14);
  if ((*(int *)(iVar9 + 0x14) == -1) ||
     (bVar4 = false, *(int *)(iVar9 + 0x14) == *DAT_007461a4 + -1)) {
    bVar4 = true;
  }
  uVar2 = *(uint *)(iVar9 + 4);
  if ((uVar2 & 1) == 0) {
    sVar1 = *(short *)(iVar9 + 0x18);
    iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar9 + 0x10) & 0xffff) * 0xc);
    if (sVar1 == -1) {
      local_3c = 0x3f800000;
      bVar11 = true;
    }
    else {
      local_3c = *(undefined4 *)(iVar10 + 0x134 + sVar1 * 4);
      bVar11 = ((byte)(1 << ((byte)sVar1 & 0x1f)) & *(byte *)(iVar10 + 0x123)) != 0;
    }
  }
  else {
    local_3c = *(undefined4 *)(iVar9 + 8);
    bVar11 = (bool)(~(byte)(uVar2 >> 1) & 1);
  }
  if (bVar11 == false) {
    if (*(short *)(iVar9 + 2) == 2) goto LAB_00544417;
    if (bVar4) goto LAB_00544429;
    if (*(short *)(iVar9 + 2) == 2) goto LAB_00544417;
  }
  else {
LAB_00544429:
    uVar3 = *(uint *)(iVar9 + 0x10);
    if (uVar3 != 0xffffffff) {
      iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
      iVar10 = (int)*(short *)(iVar10 + 0x1f2) + *(short *)(iVar9 + 0x1a) * 0x34 + iVar10;
      matrix4x3_transform_point(iVar10);
      matrix4x3_transform_normal(iVar10);
      FUN_004f6aa0();
      local_10 = *param_2;
      local_c = param_2[1];
    }
    local_40[0] = (ushort)(uVar3 != 0xffffffff);
    local_38 = 0x3f800000;
    if (bVar11 == false) {
      if (bVar4) {
        local_50 = 0x40800000;
        if ((uVar2 & 4) == 0) {
          local_50 = 0;
        }
        cVar6 = sound_looping_set_state(local_40,2,uVar2 >> 3 & 0xffffff01,local_50);
        piVar5 = DAT_007461a4;
        if (cVar6 == '\0') {
          *(undefined2 *)(iVar9 + 2) = 1;
          *(int *)(iVar9 + 0x14) = *piVar5;
          return;
        }
      }
      if ((*(byte *)(iVar9 + 4) & 1) != 0) {
LAB_005445a3:
        datum_delete();
        *(int *)(iVar9 + 0x14) = *DAT_007461a4;
        return;
      }
    }
    else {
      if ((*(short *)(iVar9 + 2) == 0) || (!bVar4)) {
        uVar7 = 1;
      }
      else {
        uVar7 = 0;
      }
      *(undefined2 *)(iVar9 + 2) = 0;
      cVar6 = sound_looping_set_state(local_40,uVar7,uVar2 >> 3 & 0xffffff01,0);
      if (cVar6 == '\0') goto LAB_00544417;
      if ((*(byte *)(iVar9 + 4) & 1) != 0) {
        if (*(uint *)(iVar8 + 0x1c) == param_1) {
          *(undefined4 *)(iVar8 + 0x1c) = 0xffffffff;
          datum_delete();
          *(int *)(iVar9 + 0x14) = *DAT_007461a4;
          return;
        }
        goto LAB_005445a3;
      }
    }
  }
  *(undefined2 *)(iVar9 + 2) = 2;
LAB_00544417:
  *(int *)(iVar9 + 0x14) = *DAT_007461a4;
  return;
}

Disassembly (0x544330..0x5445bf, capstone; phase-4 review):

0x544330: sub esp, 0x54
0x544333: mov eax, dword ptr [0x7461a0]
0x544338: mov edx, dword ptr [0x87bc14]
0x54433e: push ebx
0x54433f: push ebp
0x544340: mov ebp, dword ptr [esp + 0x60]
0x544344: and ebp, 0xffff
0x54434a: imul ebp, ebp, 0x34
0x54434d: push esi
0x54434e: push edi
0x54434f: mov edi, dword ptr [eax + 0x34]
0x544352: mov ecx, dword ptr [ebp + edi + 0xc]
0x544356: add ebp, edi
0x544358: and ecx, 0xffff
0x54435e: shl ecx, 5
0x544361: mov eax, dword ptr [ecx + edx + 0x14]
0x544365: mov dword ptr [esp + 0x14], eax
0x544369: mov eax, dword ptr [ebp + 0x14]
0x54436c: cmp eax, -1
0x54436f: je 0x544383
0x544371: mov ecx, dword ptr [0x7461a4]
0x544377: mov edx, dword ptr [ecx]
0x544379: dec edx
0x54437a: cmp eax, edx
0x54437c: mov byte ptr [esp + 0x12], 0
0x544381: jne 0x544388
0x544383: mov byte ptr [esp + 0x12], 1
0x544388: mov ebx, dword ptr [ebp + 4]
0x54438b: test bl, 1
0x54438e: mov edx, dword ptr [0x8603b0]
0x544394: jne 0x5443e2
0x544396: mov eax, dword ptr [ebp + 0x10]
0x544399: mov cx, word ptr [ebp + 0x18]
0x54439d: mov esi, dword ptr [edx + 0x34]
0x5443a0: and eax, 0xffff
0x5443a5: cmp cx, -1
0x5443a9: lea eax, [eax + eax*2]
0x5443ac: mov esi, dword ptr [esi + eax*4 + 8]
0x5443b0: jne 0x5443be
0x5443b2: mov dword ptr [esp + 0x28], 0x3f800000
0x5443ba: mov al, 1
0x5443bc: jmp 0x5443f1
0x5443be: movsx edi, cx
0x5443c1: mov ecx, edi
0x5443c3: mov eax, 1
0x5443c8: fld dword ptr [esi + edi*4 + 0x134]
0x5443cf: shl eax, cl
0x5443d1: mov cl, byte ptr [esi + 0x123]
0x5443d7: fstp dword ptr [esp + 0x28]
0x5443db: test al, cl
0x5443dd: setne al
0x5443e0: jmp 0x5443f1
0x5443e2: mov ecx, dword ptr [ebp + 8]
0x5443e5: mov eax, ebx
0x5443e7: shr eax, 1
0x5443e9: not al
0x5443eb: and al, 1
0x5443ed: mov dword ptr [esp + 0x28], ecx
0x5443f1: test al, al
0x5443f3: mov byte ptr [esp + 0x13], al
0x5443f7: jne 0x544429
0x5443f9: mov ax, word ptr [ebp + 2]
0x5443fd: cmp ax, 2
0x544401: je 0x544417
0x544403: mov cl, byte ptr [esp + 0x12]
0x544407: test cl, cl
0x544409: jne 0x544429
0x54440b: cmp ax, 2
0x54440f: je 0x544417
0x544411: mov word ptr [ebp + 2], 2
0x544417: mov eax, dword ptr [0x7461a4]
0x54441c: mov ecx, dword ptr [eax]
0x54441e: pop edi
0x54441f: pop esi
0x544420: mov dword ptr [ebp + 0x14], ecx
0x544423: pop ebp
0x544424: pop ebx
0x544425: add esp, 0x54
0x544428: ret 
0x544429: mov eax, dword ptr [ebp + 0x10]
0x54442c: cmp eax, -1
0x54442f: je 0x54449b
0x544431: mov ecx, dword ptr [edx + 0x34]
0x544434: movsx edx, word ptr [ebp + 0x1a]
0x544438: imul edx, edx, 0x34
0x54443b: and eax, 0xffff
0x544440: lea eax, [eax + eax*2]
0x544443: mov eax, dword ptr [ecx + eax*4 + 8]
0x544447: movsx esi, word ptr [eax + 0x1f2]
0x54444e: add edx, eax
0x544450: add esi, edx
0x544452: lea edx, [ebp + 0x1c]
0x544455: push esi
0x544456: lea eax, [esp + 0x34]
0x54445a: call 0x4cbde0
0x54445f: lea edx, [ebp + 0x28]
0x544462: push esi
0x544463: lea eax, [esp + 0x44]
0x544467: call 0x4cbec0
0x54446c: mov eax, dword ptr [ebp + 0x10]
0x54446f: add esp, 8
0x544472: lea edi, [esp + 0x18]
0x544476: lea esi, [esp + 0x48]
0x54447a: call 0x4f6aa0
0x54447f: mov eax, dword ptr [esp + 0x6c]
0x544483: mov ecx, dword ptr [eax]
0x544485: mov edx, dword ptr [eax + 4]
0x544488: mov dword ptr [esp + 0x54], ecx
0x54448c: mov dword ptr [esp + 0x58], edx
0x544490: mov word ptr [esp + 0x24], 1
0x544497: xor ecx, ecx
0x544499: jmp 0x5444a2
0x54449b: xor ecx, ecx
0x54449d: mov word ptr [esp + 0x24], cx
0x5444a2: mov al, byte ptr [esp + 0x13]
0x5444a6: test al, al
0x5444a8: mov dword ptr [esp + 0x2c], 0x3f800000
0x5444b0: je 0x544537
0x5444b6: cmp word ptr [ebp + 2], cx
0x5444ba: je 0x5444c8
0x5444bc: mov al, byte ptr [esp + 0x12]
0x5444c0: test al, al
0x5444c2: je 0x5444c8
0x5444c4: xor eax, eax
0x5444c6: jmp 0x5444cd
0x5444c8: mov eax, 1
0x5444cd: push ecx
0x5444ce: shr ebx, 3
0x5444d1: and ebx, 0xffffff01
0x5444d7: push ebx
0x5444d8: push eax
0x5444d9: lea eax, [esp + 0x30]
0x5444dd: mov word ptr [ebp + 2], cx
0x5444e1: mov ecx, dword ptr [ebp + 0xc]
0x5444e4: push eax
0x5444e5: mov eax, dword ptr [esp + 0x78]
0x5444e9: call 0x549fa0
0x5444ee: add esp, 0x10
0x5444f1: test al, al
0x5444f3: je 0x544417
0x5444f9: test byte ptr [ebp + 4], 1
0x5444fd: je 0x544411
0x544503: mov eax, dword ptr [esp + 0x14]
0x544507: mov edx, dword ptr [esp + 0x68]
0x54450b: cmp dword ptr [eax + 0x1c], edx
0x54450e: jne 0x5445a3
0x544514: mov dword ptr [eax + 0x1c], 0xffffffff
0x54451b: mov eax, dword ptr [0x7461a0]
0x544520: call 0x4d0510
0x544525: mov eax, dword ptr [0x7461a4]
0x54452a: mov ecx, dword ptr [eax]
0x54452c: pop edi
0x54452d: pop esi
0x54452e: mov dword ptr [ebp + 0x14], ecx
0x544531: pop ebp
0x544532: pop ebx
0x544533: add esp, 0x54
0x544536: ret 
0x544537: mov al, byte ptr [esp + 0x12]
0x54453b: test al, al
0x54453d: je 0x544595
0x54453f: test bl, 4
0x544542: mov dword ptr [esp + 0x14], 0x40800000
0x54454a: jne 0x544554
0x54454c: mov dword ptr [esp + 0x14], 0
0x544554: mov ecx, dword ptr [esp + 0x14]
0x544558: mov eax, dword ptr [esp + 0x68]
0x54455c: push ecx
0x54455d: mov ecx, dword ptr [ebp + 0xc]
0x544560: shr ebx, 3
0x544563: and ebx, 0xffffff01
0x544569: push ebx
0x54456a: lea edx, [esp + 0x2c]
0x54456e: push 2
0x544570: push edx
0x544571: call 0x549fa0
0x544576: add esp, 0x10
0x544579: test al, al
0x54457b: jne 0x544595
0x54457d: mov eax, dword ptr [0x7461a4]
0x544582: pop edi
0x544583: mov word ptr [ebp + 2], 1
0x544589: mov ecx, dword ptr [eax]
0x54458b: pop esi
0x54458c: mov dword ptr [ebp + 0x14], ecx
0x54458f: pop ebp
0x544590: pop ebx
0x544591: add esp, 0x54
0x544594: ret 
0x544595: test byte ptr [ebp + 4], 1
0x544599: je 0x544411
0x54459f: mov edx, dword ptr [esp + 0x68]
0x5445a3: mov eax, dword ptr [0x7461a0]
0x5445a8: call 0x4d0510
0x5445ad: mov eax, dword ptr [0x7461a4]
0x5445b2: mov ecx, dword ptr [eax]
0x5445b4: pop edi
0x5445b5: pop esi
0x5445b6: mov dword ptr [ebp + 0x14], ecx
0x5445b9: pop ebp
0x5445ba: pop ebx
0x5445bb: add esp, 0x54
0x5445be: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
