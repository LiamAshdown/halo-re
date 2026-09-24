// game_sound_update  (Ghidra: game_sound_update, already named)
// address 0x5445c0, size 759 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/sound_functions.md summary "Per-frame master update for the object-
//   looping-sound system: throttled listener refresh, cluster-audibility rebuild, and per-datum
//   audibility/placement update."; k_game_sound_update_interval_ms (0x21, types/sound.h) gates the
//   full path; the 0x12-dword (0x48-byte) copy destination is DAT_0072525c, exactly
//   sizeof(SoundEnvironment); game_sound_globals fields (types/sound.h: update_count 0x00,
//   background_sound_index 0x04, last_update_time 0x08) match the header's own note
//   "[1] background sound index swapped against FUN_0053f150 output"; object.flags bit 0x800 is
//   _object_needs_cluster_update_bit (types/objects.h).
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// UNSURE: FUN_0053f150 (outside this module, address < 0x543a30) is called with three output
//   pointers whose exact fields are inferred structurally (a {new_background_sound_tag,
//   leaf_number} pair, a SoundEnvironment* out-pointer, and a byte this function never reads) --
//   modeled here as sound_resolve_listener_cluster with a best-effort signature (argument order
//   confirmed by the disassembly: cluster pair, environment pointer, valid byte).
// Phase-4 review (disassembly appended below): object_try_and_get takes the object in ECX
//   (type mask -1), 0x4f6b10 is object_get_root_location(EAX out, ECX object), and
//   sound_looping_start_ambient takes EAX = -1 (no object), EDX = background tag. Objects whose
//   _object_needs_cluster_update_bit is clear are skipped for this pass (not stopped).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "sound.h"

extern data_array *game_looping_sound_data;        // 0x007461a0
extern game_sound_globals *game_sound_globals_ptr;  // 0x007461a4
extern tag_instance *tag_instances;                 // 0x0087bc14
extern data_array *object_data;                     // 0x008603b0
extern SoundEnvironment sound_environment;          // 0x0072525c
extern uint32_t sound_cluster_audible_bitmap[k_sound_cluster_bitmap_words]; // 0x00746160
extern int64_t performance_frequency;               // 0x006ac8f8/0x006ac8fc, see src/sound/sound_update_clock.c

extern int32_t QueryPerformanceCounter(large_integer *counter);
extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern void game_looping_sound_touch_if_valid(datum_index looping_sound_index); // 0x544290
extern void game_looping_sound_update(datum_index looping_sound_index, int32_t *root_location); // 0x544330
extern datum_index sound_looping_start_ambient(datum_index object_index, datum_index definition_index, float scale); // 0x544250, blam-cc: EAX, EDX, stack
extern void sound_build_cluster_range_bitmap(void); // 0x544980

extern void sound_resolve_listener_cluster(int32_t *background_tag_and_leaf, SoundEnvironment **environment,
    uint8_t *valid); // 0x53f150, UNSURE signature (outside this module), see file header
extern void object_get_root_location(int32_t *out, uint32_t object_index); // 0x4f6b10, blam-cc: EAX -> out, ECX -> object_index

static int32_t game_sound_update_now_ms(void) // UNSURE helper name; inlines sound_update_clock.c's own QPC math
{
    large_integer counter;
    QueryPerformanceCounter(&counter);
    return (int32_t)((counter.quad_part * 1000) / performance_frequency);
}

// blam-cc: (no arguments)
// Per-tick object-looping-sound update. Below the 33ms full-update threshold, only refreshes each
// live datum's liveness stamp. At or above it, resolves the listener's current BSP cluster and
// its SoundEnvironment, rebuilds the cluster-audibility bitmap, starts/stops the cluster's
// background ambient loop as needed, and runs the full per-datum audibility/placement update,
// deleting datums whose owning object is gone.
void game_sound_update(void)
{
    int32_t now_ms = game_sound_update_now_ms();

    if ((uint32_t)(now_ms - game_sound_globals_ptr->last_update_time) < k_game_sound_update_interval_ms) {
        datum_index index = datum_next(-1, game_looping_sound_data);
        while (index != k_datum_index_none) {
            game_looping_sound_touch_if_valid(index);
            index = datum_next((int16_t)index, game_looping_sound_data);
        }
        return;
    }

    {
        int32_t cluster_info[2]; // {new_background_sound_tag, leaf_number}
        SoundEnvironment *environment;
        uint8_t valid;
        datum_index background_index;
        datum_index index;

        sound_resolve_listener_cluster(cluster_info, &environment, &valid);
        sound_environment = *environment;

        sound_build_cluster_range_bitmap();

        background_index = *(datum_index *)&game_sound_globals_ptr->background_sound_index;

        if ((uint32_t)cluster_info[0] == 0xffffffff) {
            if (background_index != k_datum_index_none) {
                game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)background_index];
                self->flags |= _game_looping_sound_stop_requested_bit;
                *(uint32_t *)&game_sound_globals_ptr->background_sound_index = 0xffffffff;
            }
        } else if (background_index == k_datum_index_none) {
            background_index = sound_looping_start_ambient(k_datum_index_none, (datum_index)cluster_info[0], 1.0f);
            *(uint32_t *)&game_sound_globals_ptr->background_sound_index = (uint32_t)background_index;
        } else {
            game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)background_index];
            if ((uint32_t)self->definition_index != (uint32_t)cluster_info[0]) {
                self->flags |= _game_looping_sound_stop_requested_bit;
                background_index = sound_looping_start_ambient(k_datum_index_none, (datum_index)cluster_info[0], 1.0f);
                *(uint32_t *)&game_sound_globals_ptr->background_sound_index = (uint32_t)background_index;
            }
        }

        index = datum_next(-1, game_looping_sound_data);
        while (index != k_datum_index_none) {
            game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)index];
            datum_index object_index = self->object_index;

            if (object_index == k_datum_index_none) {
                game_looping_sound_update(index, (int32_t *)0);
            } else if ((self->flags & _game_looping_sound_script_gain_bit) == 0 ||
                       object_try_and_get(object_index, 0xffffffff) != 0) {
                object *obj = (object *)((object_header *)object_data->data)[object_index & 0xffff].data;

                if ((obj->flags & _object_needs_cluster_update_bit) != 0) {
                    int32_t leaf_cluster[2];

                    object_get_root_location(leaf_cluster, object_index);

                    if ((int16_t)leaf_cluster[1] != -1 &&
                        (sound_cluster_audible_bitmap[(int16_t)leaf_cluster[1] >> 5] &
                         (1u << (leaf_cluster[1] & 0x1f))) != 0) {
                        game_looping_sound_update(index, leaf_cluster);
                    }
                }
            } else {
                SoundLooping *definition = (SoundLooping *)tag_instances[self->definition_index & 0xffff].data;
                if (*(uint32_t *)&definition->runtime_scripting_sound == (uint32_t)index) {
                    *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)k_datum_index_none;
                }
                datum_delete(game_looping_sound_data, index);
            }

            index = datum_next((int16_t)index, game_looping_sound_data);
        }

        game_sound_globals_ptr->update_count += 1;
        game_sound_globals_ptr->last_update_time = game_sound_update_now_ms();
    }
}

#if 0
Original Ghidra decompilation (0x5445c0):

void game_sound_update(void)

{
  uint *puVar1;
  uint uVar2;
  int *piVar3;
  int iVar4;
  uint uVar5;
  short *psVar6;
  int iVar7;
  short sVar8;
  int iVar9;
  undefined4 *puVar10;
  undefined8 uVar11;
  LARGE_INTEGER *pLVar12;
  undefined1 local_15;
  undefined4 *local_14;
  LARGE_INTEGER local_10;

  QueryPerformanceCounter(&local_10);
  uVar11 = __allmul(local_10.s.LowPart,local_10.s.HighPart,1000,0);
  iVar4 = __alldiv(uVar11,DAT_006ac8f8,DAT_006ac8fc);
  piVar3 = DAT_007461a4;
  iVar9 = DAT_007461a0;
  if ((uint)(iVar4 - DAT_007461a4[2]) < 0x21) {
    uVar5 = datum_next();
    if (uVar5 != 0xffffffff) {
      do {
        FUN_00544290();
        iVar4 = uVar5 + 1;
        uVar5 = 0xffffffff;
        sVar8 = (short)iVar4;
        if ((-1 < sVar8) && (sVar8 < *(short *)(iVar9 + 0x2e))) {
          psVar6 = (short *)((int)sVar8 * (int)*(short *)(iVar9 + 0x22) + *(int *)(iVar9 + 0x34));
          do {
            if (*psVar6 != 0) {
              uVar5 = (int)*psVar6 << 0x10 | (int)(short)iVar4;
              break;
            }
            iVar4 = iVar4 + 1;
            psVar6 = (short *)((int)psVar6 + (int)*(short *)(iVar9 + 0x22));
          } while ((short)iVar4 < *(short *)(iVar9 + 0x2e));
        }
        if (uVar5 == 0xffffffff) {
          return;
        }
      } while( true );
    }
  }
  else {
    FUN_0053f150(&local_10,&local_14,&local_15);
    puVar10 = &DAT_0072525c;
    for (iVar9 = 0x12; iVar9 != 0; iVar9 = iVar9 + -1) {
      *puVar10 = *local_14;
      local_14 = local_14 + 1;
      puVar10 = puVar10 + 1;
    }
    sound_build_cluster_range_bitmap();
    iVar9 = DAT_007461a0;
    uVar5 = piVar3[1];
    if (local_10.s.LowPart == 0xffffffff) {
      if (uVar5 != 0xffffffff) {
        iVar4 = (uVar5 & 0xffff) * 0x34;
        *(uint *)(iVar4 + *(int *)(DAT_007461a0 + 0x34) + 4) =
             *(uint *)(iVar4 + 4 + *(int *)(DAT_007461a0 + 0x34)) | 2;
        piVar3[1] = -1;
      }
    }
    else if (uVar5 == 0xffffffff) {
      iVar9 = FUN_00544250(0x3f800000);
      DAT_007461a4[1] = iVar9;
      iVar9 = DAT_007461a0;
    }
    else {
      iVar4 = (uVar5 & 0xffff) * 0x34 + *(int *)(DAT_007461a0 + 0x34);
      if (*(DWORD *)(iVar4 + 0xc) != local_10.s.LowPart) {
        puVar1 = (uint *)(iVar4 + 4);
        *puVar1 = *puVar1 | 2;
        iVar9 = FUN_00544250(0x3f800000);
        DAT_007461a4[1] = iVar9;
        iVar9 = DAT_007461a0;
      }
    }
    uVar5 = datum_next();
joined_r0x0054474b:
    if (uVar5 != 0xffffffff) {
      iVar4 = (uVar5 & 0xffff) * 0x34;
      uVar2 = *(uint *)(iVar4 + 0x10 + *(int *)(iVar9 + 0x34));
      iVar4 = iVar4 + *(int *)(iVar9 + 0x34);
      if (uVar2 == 0xffffffff) {
        pLVar12 = (LARGE_INTEGER *)0x0;
LAB_00544807:
        FUN_00544330(uVar5,pLVar12);
        iVar9 = DAT_007461a0;
      }
      else if (((*(byte *)(iVar4 + 4) & 1) == 0) ||
              (iVar7 = object_try_and_get(0xffffffff), iVar7 != 0)) {
        if ((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 0x10)
            & 0x800) != 0) {
          FUN_004f6b10();
          if (((short)local_10.s.HighPart != -1) &&
             (((&DAT_00746160)[(int)(short)local_10.s.HighPart >> 5] &
              1 << ((byte)local_10.s.HighPart & 0x1f)) != 0)) {
            pLVar12 = &local_10;
            goto LAB_00544807;
          }
        }
      }
      else {
        iVar4 = *(int *)((*(uint *)(iVar4 + 0xc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if (*(uint *)(iVar4 + 0x1c) == uVar5) {
          *(undefined4 *)(iVar4 + 0x1c) = 0xffffffff;
        }
        datum_delete();
      }
      iVar4 = uVar5 + 1;
      uVar5 = 0xffffffff;
      sVar8 = (short)iVar4;
      if ((-1 < sVar8) && (sVar8 < *(short *)(iVar9 + 0x2e))) {
        psVar6 = (short *)((int)sVar8 * (int)*(short *)(iVar9 + 0x22) + *(int *)(iVar9 + 0x34));
        do {
          if (*psVar6 != 0) {
            uVar5 = (int)*psVar6 << 0x10 | (int)(short)iVar4;
            break;
          }
          iVar4 = iVar4 + 1;
          psVar6 = (short *)((int)psVar6 + (int)*(short *)(iVar9 + 0x22));
        } while ((short)iVar4 < *(short *)(iVar9 + 0x2e));
      }
      goto joined_r0x0054474b;
    }
    *DAT_007461a4 = *DAT_007461a4 + 1;
    QueryPerformanceCounter(&local_10);
    uVar11 = __allmul(local_10.s.LowPart,local_10.s.HighPart,1000,0);
    iVar9 = __alldiv(uVar11,DAT_006ac8f8,DAT_006ac8fc);
    DAT_007461a4[2] = iVar9;
  }
  return;
}

Disassembly (0x5445c0..0x5448b7, capstone; phase-4 review):

0x5445c0: push ebp
0x5445c1: mov ebp, esp
0x5445c3: and esp, 0xfffffff8
0x5445c6: sub esp, 0x10
0x5445c9: push ebx
0x5445ca: push ebp
0x5445cb: push esi
0x5445cc: push edi
0x5445cd: lea eax, [esp + 0x18]
0x5445d1: push eax
0x5445d2: call dword ptr [0x63a0ac]
0x5445d8: mov ecx, dword ptr [esp + 0x1c]
0x5445dc: mov edx, dword ptr [esp + 0x18]
0x5445e0: push 0
0x5445e2: push 0x3e8
0x5445e7: push ecx
0x5445e8: push edx
0x5445e9: call 0x62de80
0x5445ee: mov ecx, dword ptr [0x6ac8fc]
0x5445f4: push ecx
0x5445f5: mov ecx, dword ptr [0x6ac8f8]
0x5445fb: push ecx
0x5445fc: push edx
0x5445fd: push eax
0x5445fe: call 0x639230
0x544603: mov ebp, dword ptr [0x7461a4]
0x544609: sub eax, dword ptr [ebp + 8]
0x54460c: cmp eax, 0x21
0x54460f: jae 0x544682
0x544611: mov edi, dword ptr [0x7461a0]
0x544617: or edx, 0xffffffff
0x54461a: call 0x4d0630
0x54461f: mov esi, eax
0x544621: cmp esi, -1
0x544624: je 0x5448af
0x54462a: lea ebx, [ebx]
0x544630: call 0x544290
0x544635: lea ecx, [esi + 1]
0x544638: or ebp, 0xffffffff
0x54463b: test cx, cx
0x54463e: jl 0x544673
0x544640: mov si, word ptr [edi + 0x2e]
0x544644: cmp cx, si
0x544647: jge 0x544673
0x544649: movsx edx, word ptr [edi + 0x22]
0x54464d: mov ebx, dword ptr [edi + 0x34]
0x544650: movsx eax, cx
0x544653: imul eax, edx
0x544656: add eax, ebx
0x544658: cmp word ptr [eax], 0
0x54465c: jne 0x544668
0x54465e: inc ecx
0x54465f: add eax, edx
0x544661: cmp cx, si
0x544664: jl 0x544658
0x544666: jmp 0x544673
0x544668: movsx ebp, word ptr [eax]
0x54466b: movsx edx, cx
0x54466e: shl ebp, 0x10
0x544671: or ebp, edx
0x544673: cmp ebp, -1
0x544676: mov esi, ebp
0x544678: jne 0x544630
0x54467a: pop edi
0x54467b: pop esi
0x54467c: pop ebp
0x54467d: pop ebx
0x54467e: mov esp, ebp
0x544680: pop ebp
0x544681: ret 
0x544682: lea eax, [esp + 0x13]
0x544686: push eax
0x544687: lea ecx, [esp + 0x18]
0x54468b: push ecx
0x54468c: lea edx, [esp + 0x20]
0x544690: push edx
0x544691: call 0x53f150
0x544696: mov esi, dword ptr [esp + 0x20]
0x54469a: mov ecx, 0x12
0x54469f: mov edi, 0x72525c
0x5446a4: add esp, 0xc
0x5446a7: rep movsd dword ptr es:[edi], dword ptr [esi]
0x5446a9: call 0x544980
0x5446ae: mov edx, dword ptr [esp + 0x18]
0x5446b2: cmp edx, -1
0x5446b5: mov eax, dword ptr [ebp + 4]
0x5446b8: jne 0x5446e4
0x5446ba: cmp eax, edx
0x5446bc: mov ebx, dword ptr [0x7461a0]
0x5446c2: je 0x54473c
0x5446c4: mov edx, dword ptr [ebx + 0x34]
0x5446c7: and eax, 0xffff
0x5446cc: imul eax, eax, 0x34
0x5446cf: mov ecx, dword ptr [eax + edx + 4]
0x5446d3: add eax, edx
0x5446d5: or ecx, 2
0x5446d8: mov dword ptr [eax + 4], ecx
0x5446db: mov dword ptr [ebp + 4], 0xffffffff
0x5446e2: jmp 0x54473c
0x5446e4: cmp eax, -1
0x5446e7: je 0x54471d
0x5446e9: mov ebx, dword ptr [0x7461a0]
0x5446ef: mov ecx, dword ptr [ebx + 0x34]
0x5446f2: and eax, 0xffff
0x5446f7: imul eax, eax, 0x34
0x5446fa: add eax, ecx
0x5446fc: cmp dword ptr [eax + 0xc], edx
0x5446ff: je 0x54473c
0x544701: or dword ptr [eax + 4], 2
0x544705: push 0x3f800000
0x54470a: or eax, 0xffffffff
0x54470d: call 0x544250
0x544712: mov edx, dword ptr [0x7461a4]
0x544718: mov dword ptr [edx + 4], eax
0x54471b: jmp 0x544733
0x54471d: push 0x3f800000
0x544722: or eax, 0xffffffff
0x544725: call 0x544250
0x54472a: mov ecx, dword ptr [0x7461a4]
0x544730: mov dword ptr [ecx + 4], eax
0x544733: mov ebx, dword ptr [0x7461a0]
0x544739: add esp, 4
0x54473c: or edx, 0xffffffff
0x54473f: mov edi, ebx
0x544741: call 0x4d0630
0x544746: mov ebp, eax
0x544748: cmp ebp, -1
0x54474b: je 0x544866
0x544751: mov ecx, dword ptr [ebx + 0x34]
0x544754: mov esi, ebp
0x544756: and esi, 0xffff
0x54475c: imul esi, esi, 0x34
0x54475f: mov edi, dword ptr [esi + ecx + 0x10]
0x544763: add esi, ecx
0x544765: cmp edi, -1
0x544768: jne 0x544771
0x54476a: push 0
0x54476c: jmp 0x544807
0x544771: test byte ptr [esi + 4], 1
0x544775: je 0x5447b3
0x544777: push -1
0x544779: mov ecx, edi
0x54477b: call 0x4f6ec0
0x544780: add esp, 4
0x544783: test eax, eax
0x544785: jne 0x5447b3
0x544787: mov edx, dword ptr [esi + 0xc]
0x54478a: mov eax, dword ptr [0x87bc14]
0x54478f: and edx, 0xffff
0x544795: shl edx, 5
0x544798: mov eax, dword ptr [edx + eax + 0x14]
0x54479c: cmp dword ptr [eax + 0x1c], ebp
0x54479f: jne 0x5447a8
0x5447a1: mov dword ptr [eax + 0x1c], 0xffffffff
0x5447a8: mov edx, ebp
0x5447aa: mov eax, ebx
0x5447ac: call 0x4d0510
0x5447b1: jmp 0x544816
0x5447b3: mov edx, dword ptr [0x8603b0]
0x5447b9: mov eax, edi
0x5447bb: and eax, 0xffff
0x5447c0: lea ecx, [eax + eax*2]
0x5447c3: mov eax, dword ptr [edx + 0x34]
0x5447c6: mov ecx, dword ptr [eax + ecx*4 + 8]
0x5447ca: mov eax, dword ptr [ecx + 0x10]
0x5447cd: test ah, 8
0x5447d0: je 0x544816
0x5447d2: lea eax, [esp + 0x18]
0x5447d6: mov ecx, edi
0x5447d8: call 0x4f6b10
0x5447dd: mov eax, dword ptr [esp + 0x1c]
0x5447e1: cmp ax, 0xffff
0x5447e5: je 0x544816
0x5447e7: movsx eax, ax
0x5447ea: mov ecx, eax
0x5447ec: and ecx, 0x1f
0x5447ef: mov edx, 1
0x5447f4: shl edx, cl
0x5447f6: sar eax, 5
0x5447f9: test dword ptr [eax*4 + 0x746160], edx
0x544800: je 0x544816
0x544802: lea eax, [esp + 0x18]
0x544806: push eax
0x544807: push ebp
0x544808: call 0x544330
0x54480d: mov ebx, dword ptr [0x7461a0]
0x544813: add esp, 8
0x544816: lea ecx, [ebp + 1]
0x544819: or esi, 0xffffffff
0x54481c: test cx, cx
0x54481f: jl 0x54485b
0x544821: mov di, word ptr [ebx + 0x2e]
0x544825: cmp cx, di
0x544828: jge 0x54485b
0x54482a: movsx edx, word ptr [ebx + 0x22]
0x54482e: mov ebp, dword ptr [ebx + 0x34]
0x544831: movsx eax, cx
0x544834: imul eax, edx
0x544837: add eax, ebp
0x544839: lea esp, [esp]
0x544840: cmp word ptr [eax], 0
0x544844: jne 0x544850
0x544846: inc ecx
0x544847: add eax, edx
0x544849: cmp cx, di
0x54484c: jl 0x544840
0x54484e: jmp 0x54485b
0x544850: movsx esi, word ptr [eax]
0x544853: movsx ecx, cx
0x544856: shl esi, 0x10
0x544859: or esi, ecx
0x54485b: cmp esi, -1
0x54485e: mov ebp, esi
0x544860: jne 0x544751
0x544866: mov eax, dword ptr [0x7461a4]
0x54486b: mov ebx, dword ptr [eax]
0x54486d: lea edx, [esp + 0x18]
0x544871: inc ebx
0x544872: push edx
0x544873: mov dword ptr [eax], ebx
0x544875: call dword ptr [0x63a0ac]
0x54487b: mov eax, dword ptr [esp + 0x1c]
0x54487f: mov ecx, dword ptr [esp + 0x18]
0x544883: push 0
0x544885: push 0x3e8
0x54488a: push eax
0x54488b: push ecx
0x54488c: call 0x62de80
0x544891: mov ecx, dword ptr [0x6ac8fc]
0x544897: push ecx
0x544898: mov ecx, dword ptr [0x6ac8f8]
0x54489e: push ecx
0x54489f: push edx
0x5448a0: push eax
0x5448a1: call 0x639230
0x5448a6: mov edx, dword ptr [0x7461a4]
0x5448ac: mov dword ptr [edx + 8], eax
0x5448af: pop edi
0x5448b0: pop esi
0x5448b1: pop ebp
0x5448b2: pop ebx
0x5448b3: mov esp, ebp
0x5448b5: pop ebp
0x5448b6: ret 
#endif
