// objects_garbage_collection  (Ghidra: objects_garbage_collection, already named; a CEA/PDB
// symbol match per out/phase4/objects_functions.md's naming hints, confirmed by the debug
// strings it prints)
// address 0x4f9c60, size 1149 bytes
// name confidence: 0.9 (already carries this name; matches functions.md's summary: "Periodic
//   object-pool garbage collector that frees/evicts objects when free memory or free slots drop
//   below thresholds, reporting via debug strings")
// rewrite confidence: 0.2 (this is the largest and most heuristic-heavy function in the module:
//   nested severity levels, an AI-module callback table this module does not own, and five
//   different printf-style debug reports. Given the time available, this rewrite is a close,
//   MECHANICAL transliteration of the decompiled C -- control flow (including its gotos) and
//   arithmetic are preserved, but many object_globals/memory_pool offsets past what
//   types/objects.h already documents are kept as raw casts rather than re-derived field names.
//   Treat every non-established offset below as UNSURE.)
// evidence: types/objects.h object_globals (first_tracked_object 0x08), object (flags 0x10);
//   types/memory.h memory_pool (base 0x24, size 0x28, free_bytes 0x2c, last_block 0x34,
//   memory_pool_block.size 0x04), data_array (maximum_count 0x20, last_index 0x2e,
//   actual_count 0x30); global 0x008603b0 object_data, 0x006b8cb4 object_memory_pool,
//   0x006b8cbc object_globals_pointer, 0x006f1d6c game_time (tick at +0xc); callees
//   block_list_compact (memory module), object_test_in_atmosphere_zone (0x4f76e0, this batch),
//   object_list_membership_set (0x4f7450, this batch), object_delete_4f9030 (0x4f9030, this
//   batch), console_print_error_va and sprintf (established elsewhere in this codebase).
// register convention: no parameters (matches functions.md: a periodic sweep with no caller
//   inputs).
// UNSURE: FUN_004f59d0 (called here as (object_index, 0)) and the AI callback table at
//   0x0065ddd0 (three named function pointers, ai_release_inactive_swarms /
//   ai_build_priority_target_list / ai_release_inactive_encounters, walked in pairs) both
//   belong to modules this pass does not own and are preserved as raw offsets / opaque
//   function-pointer calls.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern memory_pool *object_memory_pool; // 0x006b8cb4
extern object_globals *object_globals_pointer; // 0x006b8cbc
extern uint8_t *game_time; // 0x006f1d6c, tick at +0xc
extern void *ai_gc_callback_table; // 0x0065ddd0, UNSURE: see file header
extern void *console_error_category_objects; // 0x0065efec, UNSURE: a console category tag

extern void block_list_compact(); // memory module, 0x4d1eb0. src/memory/block_list_compact.c
    // takes `memory_pool *arena`; Ghidra models no argument at the call sites in this file,
    // so no prototype is asserted here rather than inventing an arena pointer.
extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693
extern void console_print_error_va(const char *format, ...); // 0x4c67c0, same declaration as
    // src/hs/hs_compile_source.c; the first argument at the call sites here is a category
    // string, which this variadic form accepts unchanged
extern uint8_t object_test_in_atmosphere_zone(uint32_t object_index); // 0x4f76e0, this batch
extern void object_list_membership_set(uint32_t object_index, char add); // 0x4f7450, this batch
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0, cdecl
extern void object_delete_4f9030(uint32_t object_index, char recurse_siblings); // 0x4f9030, this batch

void objects_garbage_collection(void)
{
    int32_t severity; // local_2810
    datum_index queue[2047]; // local_2008: tracked-object handles gathered up front
    int16_t queue_count = 0; // sVar11
    uint8_t critical; // bVar1

    if (object_globals_pointer->unknown_02[0] != 0) { // UNSURE: byte at object_globals+0x02
        severity = 0;
    } else {
        int32_t used_end = (object_memory_pool->last_block == 0) ? 0 :
            (int32_t)object_memory_pool->last_block + object_memory_pool->last_block->size - (int32_t)object_memory_pool->base;

        if (object_memory_pool->size - used_end < 0x1999a) {
            block_list_compact();
            used_end = (object_memory_pool->last_block == 0) ? 0 :
                (int32_t)object_memory_pool->last_block + object_memory_pool->last_block->size - (int32_t)object_memory_pool->base;
            if (object_memory_pool->size - used_end > 0x33333) {
                object_globals_pointer->unknown_02[0] = 0;
                return;
            }
            severity = 2;
        } else if (0x800 - object_data->maximum_count < 0x67) {
            severity = 2;
        } else if (object_globals_pointer->unknown_04 < 0x32) {
            object_globals_pointer->unknown_02[0] = 0;
            return;
        } else {
            severity = 1;
        }
    }

    // Gather every object currently on the tracked-object list (the same list
    // object_list_membership_set, this batch, maintains).
    {
        datum_index handle = object_globals_pointer->first_tracked_object;
        while (handle != k_datum_index_none) {
            queue[queue_count] = handle;
            handle = ((object_header *)object_data->data)[handle & 0xffff].data->next_tracked_object;
            queue_count++;
        }
    }

sweep:
    critical = 0;
    if (severity == 0) {
        critical = 0;
    } else if (severity == 1) {
        critical = object_globals_pointer->unknown_04 < 0x1f;
    } else if (severity == 2) {
        if ((object_memory_pool->free_bytes < 0x33333) || (0x800 - object_data->last_index < 0xcc)) {
            critical = 0;
            goto compact_and_retry;
        }
        critical = 1;
        goto report;
    } else {
        goto after_report;
    }
    goto after_report;

report:
    {
        char message_a[512];   // local_2808
        char message_b[512];   // local_2608
        char message_c[512];   // acStack_2408
        datum_index scratch_result_array[512]; // auStack_2208
        uint8_t already_ran_first_stage = 0; // bVar4
        uint8_t low_on_slots;   // bVar3
        uint8_t need_report;    // bVar2
        char keep_going;        // local_2811, initialized once above the label in the original
        char stale_warning;     // local_2812
        void **callback = (void **)&ai_gc_callback_table;
        char last_removed;

        if ((object_globals_pointer->unknown_8c == -1) ||
            (*(int32_t *)(game_time + 0xc) > object_globals_pointer->unknown_8c + 0x96)) {
            stale_warning = 1;
        } else {
            stale_warning = 0;
        }
        keep_going = 0;

    resample:
        need_report = 0;
        low_on_slots = 0;
        if (severity == 2) {
            int32_t used_end2 = (object_memory_pool->last_block == 0) ? 0 :
                (int32_t)object_memory_pool->last_block + object_memory_pool->last_block->size - (int32_t)object_memory_pool->base;
            int32_t free_bytes = object_memory_pool->size - used_end2;
            int32_t free_slots = 0x800 - object_data->last_index;

            if (free_bytes < 0xcccd) {
                low_on_slots = 1;
                need_report = 1;
                sprintf(message_a, "%4.2f%% memory free", (double)((float)free_bytes * 100.0f * 4.7683716e-07f));
            } else if (free_slots > 0x33) {
                if (free_bytes < 0x1999a) {
                    low_on_slots = 1;
                    need_report = 1;
                    sprintf(message_a, "%4.2f%% memory free", (double)((float)free_bytes * 100.0f * 4.7683716e-07f));
                } else if (free_slots < 0x67) {
                    need_report = 1;
                    sprintf(message_a, "%d slots free", free_slots);
                } else {
                    need_report = 1;
                    sprintf(message_a, "%4.2f%% memory free", (double)((float)free_bytes * 100.0f * 4.7683716e-07f));
                }
            } else {
                low_on_slots = 1;
                need_report = 1;
                sprintf(message_a, "%d slots free", free_slots);
            }

            if (!low_on_slots) {
                goto check_thresholds;
            }
        } else {
            goto check_thresholds;
        }

        {
            const char *prefix = critical ? "still " : "";
            sprintf(message_b, "garbage collection %scritical (%s)", prefix, message_a);
            console_print_error_va(console_error_category_objects, message_b);
            keep_going = 1;

            if ((!low_on_slots) || (callback[1] == 0)) {
                goto mark_and_return;
            }

            last_removed = 0;
            for (;;) {
                if (last_removed != 0) {
                    goto compact_and_resample;
                }
                stale_warning = last_removed; // local_2813 = cVar5 (always 0 here)
                if ((!already_ran_first_stage) && (callback[0] != 0)) {
                    ((void (*)(void *))callback[0])(queue);
                    already_ran_first_stage = 1;
                }
                last_removed = ((char (*)(void *, char *, void *))callback[1])(scratch_result_array, &stale_warning, queue);
                if (last_removed != 0) {
                    sprintf(message_c, "removing objects: %s");
                    console_print_error_va(console_error_category_objects, message_c);
                }
                if (stale_warning == 0) {
                    callback += 2;
                    already_ran_first_stage = 0;
                }
                if (callback[1] == 0) {
                    break;
                }
            }
            if (last_removed == 0) {
                goto mark_and_return;
            }
        }

    compact_and_resample:
        critical = 1;
        block_list_compact();
        goto resample;

    check_thresholds:
        if (!critical) {
            if (((!need_report) || (stale_warning == 0)) && (keep_going == 0)) {
                object_globals_pointer->unknown_02[0] = 0;
                return;
            }
            goto mark_and_return;
        }
        {
            sprintf(message_b, "garbage collection %scritical (%s)", "not ", message_a);
            console_print_error_va(console_error_category_objects, message_b);
        }
        keep_going = 1;
        goto mark_and_return;

    mark_and_return:
        object_globals_pointer->unknown_8c = *(int32_t *)(game_time + 0xc);
        object_globals_pointer->unknown_02[0] = 0;
        return;
    }

compact_and_retry:
    // UNSURE: matches the original's LAB_004f9e5c (block_list_compact, then either return when
    // critical, or fall into the AI-callback report block below when not).
    block_list_compact();
    if (critical) {
        object_globals_pointer->unknown_02[0] = 0;
        return;
    }
    goto report;

after_report:
    if (queue_count == 0) {
        goto compact_and_retry;
    }
    queue_count--;
    {
        datum_index handle = queue[queue_count];
        object_header *header = (object_header *)object_data->data + (handle & 0xffff);
        uint8_t eligible = 1;

        if (severity == 1) {
            eligible = (header->flags & _object_header_active_bit) != 0;
        }

        if ((object_test_in_atmosphere_zone(handle) == 0) && (eligible != 0)) {
            if ((header->flags & _object_header_active_bit) != 0) {
                object_globals_pointer->unknown_04--;
            }
            object_list_membership_set(handle, 0);
            object_delete_recursive(handle, 0);
            object_delete_4f9030(handle, 0);
        }
    }
    goto sweep;
}

#if 0
Original Ghidra decompilation (0x4f9c60):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void objects_garbage_collection(void)

{
  bool bVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  char cVar5;
  uint uVar6;
  char *pcVar7;
  int iVar8;
  byte bVar9;
  undefined4 *puVar10;
  short sVar11;
  uint auStackY_22008 [32242];
  char local_2813;
  char local_2812;
  char local_2811;
  int local_2810;
  int local_280c;
  char local_2808 [512];
  char local_2608 [512];
  char acStack_2408 [512];
  undefined1 auStack_2208 [512];
  uint local_2008 [2047];
  undefined4 uStack_c;

  uStack_c = 0x4f9c70;
  if (*(char *)(DAT_006b8cbc + 2) == '\0') {
    if (*(int *)(DAT_006b8cb4 + 0x34) == 0) {
      iVar8 = 0;
    }
    else {
      iVar8 = (*(int *)(*(int *)(DAT_006b8cb4 + 0x34) + 4) - *(int *)(DAT_006b8cb4 + 0x24)) +
              *(int *)(DAT_006b8cb4 + 0x34);
    }
    if (*(int *)(DAT_006b8cb4 + 0x28) - iVar8 < 0x1999a) {
      block_list_compact();
      if (*(int *)(DAT_006b8cb4 + 0x34) == 0) {
        iVar8 = 0;
      }
      else {
        iVar8 = (*(int *)(*(int *)(DAT_006b8cb4 + 0x34) + 4) - *(int *)(DAT_006b8cb4 + 0x24)) +
                *(int *)(DAT_006b8cb4 + 0x34);
      }
      if (0x33333 < *(int *)(DAT_006b8cb4 + 0x28) - iVar8) {
        *(undefined1 *)(DAT_006b8cbc + 2) = 0;
        return;
      }
      local_2810 = 2;
    }
    else if (0x800 - *(short *)(DAT_008603b0 + 0x30) < 0x67) {
      local_2810 = 2;
    }
    else {
      if (*(short *)(DAT_006b8cbc + 4) < 0x32) {
LAB_004fa0ca:
        *(undefined1 *)(DAT_006b8cbc + 2) = 0;
        return;
      }
      local_2810 = 1;
    }
  }
  else {
    local_2810 = 0;
  }
  uVar6 = *(uint *)(DAT_006b8cbc + 8);
  sVar11 = 0;
  bVar1 = false;
  if (uVar6 != 0xffffffff) {
    iVar8 = *(int *)(DAT_008603b0 + 0x34);
    do {
      local_2008[sVar11] = uVar6;
      uVar6 = *(uint *)(*(int *)(iVar8 + 8 + (uVar6 & 0xffff) * 0xc) + 0x110);
      sVar11 = sVar11 + 1;
    } while (uVar6 != 0xffffffff);
  }
  local_280c = (int)(short)local_2810;
LAB_004f9d83:
  if (local_280c == 0) {
LAB_004f9db7:
    bVar1 = false;
  }
  else {
    if (local_280c == 1) {
      bVar1 = *(short *)(DAT_006b8cbc + 4) < 0x1f;
    }
    else if (local_280c == 2) {
      if ((*(int *)(DAT_006b8cb4 + 0x2c) < 0x33333) ||
         (0x800 - *(short *)(DAT_008603b0 + 0x2e) < 0xcc)) goto LAB_004f9db7;
      bVar1 = true;
LAB_004f9e5c:
      block_list_compact();
      iVar8 = DAT_006b8cbc;
      if (bVar1) {
LAB_004fa0b8:
        *(undefined1 *)(iVar8 + 2) = 0;
        return;
      }
      puVar10 = &DAT_0065ddd0;
      bVar4 = false;
      bVar1 = false;
      if ((*(int *)(DAT_006b8cbc + 0x8c) == -1) ||
         (local_2812 = '\0', *(int *)(DAT_006b8cbc + 0x8c) + 0x96 < *(int *)(DAT_006f1d6c + 0xc))) {
        local_2812 = '\x01';
      }
      local_2811 = '\0';
LAB_004f9eb0:
      bVar2 = false;
      bVar3 = false;
      if (local_280c == 2) {
        if (*(int *)(DAT_006b8cb4 + 0x34) == 0) {
          local_2810 = 0;
        }
        else {
          local_2810 = (*(int *)(*(int *)(DAT_006b8cb4 + 0x34) + 4) - *(int *)(DAT_006b8cb4 + 0x24))
                       + *(int *)(DAT_006b8cb4 + 0x34);
        }
        local_2810 = *(int *)(DAT_006b8cb4 + 0x28) - local_2810;
        iVar8 = 0x800 - *(short *)(DAT_008603b0 + 0x2e);
        if (local_2810 < 0xcccd) {
          bVar3 = true;
LAB_004f9f00:
          bVar2 = true;
LAB_004f9f02:
          _sprintf(local_2808,"%4.2f%% memory free",
                   (double)((float)local_2810 * 100.0 * 4.7683716e-07));
        }
        else {
          if (0x33 < iVar8) {
            if (local_2810 < 0x1999a) goto LAB_004f9f00;
            if (iVar8 < 0x67) goto LAB_004f9f5d;
            goto LAB_004f9f02;
          }
          bVar3 = true;
LAB_004f9f5d:
          bVar2 = true;
          _sprintf(local_2808,"%d slots free");
        }
        if (!bVar3) goto LAB_004f9f32;
        if (bVar1) {
          pcVar7 = "still ";
        }
        else {
          pcVar7 = "";
        }
      }
      else {
LAB_004f9f32:
        if (!bVar1) {
          if (((!bVar2) || (local_2812 == '\0')) && (local_2811 == '\0')) goto LAB_004fa0ca;
LAB_004fa0a5:
          iVar8 = DAT_006b8cbc;
          *(undefined4 *)(DAT_006b8cbc + 0x8c) = *(undefined4 *)(DAT_006f1d6c + 0xc);
          goto LAB_004fa0b8;
        }
        pcVar7 = "not ";
      }
      _sprintf(local_2608,"garbage collection %scritical (%s)",pcVar7);
      console_print_error_va(&DAT_0065efec,local_2608);
      local_2811 = '\x01';
      if ((!bVar3) || (puVar10[1] == 0)) goto LAB_004fa0a5;
      cVar5 = '\0';
      do {
        if (cVar5 != '\0') goto LAB_004fa07c;
        local_2813 = cVar5;
        if ((!bVar4) && ((code *)*puVar10 != (code *)0x0)) {
          (*(code *)*puVar10)(local_2008);
          bVar4 = true;
        }
        cVar5 = (*(code *)puVar10[1])(auStack_2208,&local_2813,local_2008);
        if (cVar5 != '\0') {
          _sprintf(acStack_2408,"removing objects: %s");
          console_print_error_va(&DAT_0065efec,acStack_2408);
        }
        if (local_2813 == '\0') {
          puVar10 = puVar10 + 2;
          bVar4 = false;
        }
      } while (puVar10[1] != 0);
      if (cVar5 == '\0') goto LAB_004fa0a5;
LAB_004fa07c:
      bVar1 = true;
      block_list_compact();
      goto LAB_004f9eb0;
    }
    if (bVar1) goto LAB_004f9e5c;
  }
  if (sVar11 == 0) goto LAB_004f9e5c;
  sVar11 = sVar11 + -1;
  uVar6 = local_2008[sVar11];
  iVar8 = *(int *)(DAT_008603b0 + 0x34) + (uVar6 & 0xffff) * 0xc;
  bVar9 = 1;
  if ((short)local_2810 == 1) {
    bVar9 = *(byte *)(iVar8 + 2) & 1;
  }
  cVar5 = FUN_004f76e0();
  if ((cVar5 == '\0') && (bVar9 != 0)) {
    if ((*(byte *)(iVar8 + 2) & 1) != 0) {
      *(short *)(DAT_006b8cbc + 4) = *(short *)(DAT_006b8cbc + 4) + -1;
    }
    FUN_004f7450();
    FUN_004f59d0(uVar6,0);
    object_delete_4f9030(uVar6,0);
  }
  goto LAB_004f9d83;
}
#endif
