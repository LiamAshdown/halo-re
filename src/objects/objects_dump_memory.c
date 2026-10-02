// objects_dump_memory  (Ghidra: objects_dump_memory, already named; a CEA/PDB symbol match)
// address 0x4fa500, size 725 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Builds and writes object_memory.txt, a full debug report of
//   object counts and memory usage broken down by object type and by object definition")
// rewrite confidence: 0.25 (zero recorded callers -- almost certainly reached only through a
//   dynamic/console-command binding outside this module's static call graph. The overflow
//   counter (local_613c) and the objects_get_statistics output struct
//   (local_613c/local_6134, this batch's own out-struct shape) alias the SAME stack slots in
//   the original, so the final "overflowed MAXIMUM_DUMPS" check actually reads back whatever
//   objects_get_statistics most recently wrote there, not the loop's own overflow count. This
//   is preserved literally via a shared union rather than "fixed", since it may be a genuine,
//   if confusing, property of the compiled code. The header line's object/active counts are
//   printed as literal -1 (0xffffffff) in the original rather than pulled from the stats
//   struct; also preserved literally.)
// evidence: types/objects.h object_memory_dump_record, k_maximum_object_types (12),
//   k_maximum_dumps-shaped 0x400 limit (matches the warning string); global 0x008603b0
//   object_data; callees object_iterator_next (0x4f6f20, this batch), objects_get_statistics
//   (0x4f7950, this batch), object_dump_accumulate_stats / object_dump_write /
//   object_dump_compare_by_total_size (this batch), qsort/fprintf/fclose (established
//   elsewhere in this codebase).
// register convention: no parameters (matches functions.md: a one-shot debug dump).
// UNSURE: FUN_00624186 (the file-open helper, called with a path and a second, unexamined
//   argument) is foreign/unexamined.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

// FIXED 2026-09-28 (retail-independence loop): the second fopen argument is the mode "a+b" (0x00660144, pushed at
//   0x4fa66d); the earlier 0 would crash in the CRT, and fopen_00624186 bound to nothing (a direct trap).

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, this batch
extern void objects_get_statistics(void *out); // 0x4f7950, this batch (object_statistics)
extern void object_dump_accumulate_stats(uint32_t object_index, object_memory_dump_record *record); // 0x4fa3d0, this batch
extern void object_dump_write(object_memory_dump_record *record, void *file); // 0x4fa490, this batch
extern int object_dump_compare_by_total_size(const object_memory_dump_record *a, const object_memory_dump_record *b); // 0x4fa3a0, this batch

void objects_dump_memory(void)
{
    object_memory_dump_record by_type[k_maximum_object_types];
    object_memory_dump_record by_definition[0x400];
    int16_t definition_count = 0;
    int16_t overflow_count = 0;
    uint8_t stats_buffer[8]; // shares its first two bytes with overflow_count in the original; see file header
    int16_t i;
    object_iterator iterator;
    object *obj;

    for (i = 0; i < k_maximum_object_types; i++) {
        by_type[i].definition_tag = k_datum_index_none;
        by_type[i].type = i;
        by_type[i].maximum_size = 0;
        by_type[i].total_size = 0;
        by_type[i].count = 0;
        by_type[i].active_count = 0;
        by_type[i].garbage_count = 0;
        by_type[i].dead_count = 0;
        by_type[i].outside_map_count = 0;
        by_type[i].at_rest_count = 0;
    }

    iterator.type_mask = 0xffffffff;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        int16_t slot = -1;
        int16_t j;

        for (j = 0; j < definition_count; j++) {
            if (by_definition[j].definition_tag == obj->definition_tag) {
                slot = j;
                break;
            }
        }

        if (slot == -1) {
            if (definition_count < 0x400) {
                by_definition[definition_count].type = -1;
                by_definition[definition_count].definition_tag = obj->definition_tag;
                by_definition[definition_count].maximum_size = 0;
                by_definition[definition_count].total_size = 0;
                by_definition[definition_count].count = 0;
                by_definition[definition_count].active_count = 0;
                by_definition[definition_count].garbage_count = 0;
                by_definition[definition_count].dead_count = 0;
                by_definition[definition_count].outside_map_count = 0;
                by_definition[definition_count].at_rest_count = 0;
                slot = definition_count;
                definition_count++;
            } else {
                overflow_count++;
            }
        }

        if (slot != -1) {
            object_dump_accumulate_stats(iterator.handle, &by_definition[slot]);
        }
        object_dump_accumulate_stats(iterator.handle, &by_type[obj->type]);

        obj = object_iterator_next(&iterator);
    }

    qsort(by_definition, definition_count, sizeof(object_memory_dump_record), (int (*)(const void *, const void *))object_dump_compare_by_total_size);
    qsort(by_type, k_maximum_object_types, sizeof(object_memory_dump_record), (int (*)(const void *, const void *))object_dump_compare_by_total_size);

    {
        void *file = fopen("object_memory.txt", "a+b"); // 0x0066e850, 0x00660144
        if (file != 0) {
            float fraction;

            objects_get_statistics(stats_buffer);
            fraction = *(float *)(stats_buffer + 4);
            overflow_count = *(int16_t *)stats_buffer; // UNSURE: the alias described in the file header

            fprintf((FILE *)file, "#%d objects (#%d active) using %3.2f%% of available memory\n\n", -1, -1, (double)(fraction * 100.0f));
            fprintf((FILE *)file, "OBJECTS BY TYPE\n");
            fprintf((FILE *)file, "number (active) [garbage/   dead/outside/at-rest] maxsize totsize\n");
            for (i = 0; i < k_maximum_object_types; i++) {
                object_dump_write(&by_type[i], file);
            }
            fprintf((FILE *)file, "\n");
            fprintf((FILE *)file, "OBJECTS BY DEFINITION\n");
            fprintf((FILE *)file, "number (active) [garbage/   dead/outside/at-rest] maxsize totsize\n");
            for (i = 0; i < definition_count; i++) {
                fprintf((FILE *)file, "% 6d (% 6d) [% 7d/% 7d/% 7d/% 7d] % 7d % 7d %s\r\n",
                    by_definition[i].count, by_definition[i].active_count, by_definition[i].garbage_count,
                    by_definition[i].dead_count, by_definition[i].outside_map_count, by_definition[i].at_rest_count,
                    by_definition[i].maximum_size, by_definition[i].total_size); // UNSURE: the %s name argument
                    // is dropped here exactly as the original's own fprintf call is (one fewer
                    // argument than the by-type path's object_dump_write, which does supply it)
            }
            fprintf((FILE *)file, "\n");
            if (overflow_count > 0) {
                fprintf((FILE *)file, "WARNING: overflowed MAXIMUM_DUMPS (%d), this dump does not include %d objects that would not fit!\n", 0x400);
            }
            fprintf((FILE *)file, "\n");
            fclose((FILE *)file);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4fa500):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Type propagation algorithm not settling */

void objects_dump_memory(void)

{
  ushort uVar1;
  short sVar2;
  int *piVar3;
  FILE *_File;
  int iVar4;
  undefined4 *puVar5;
  ushort uVar6;
  uint uVar7;
  ushort uVar8;
  int aiStackY_c6008 [196515];
  short local_613c;
  float local_6134;
  undefined4 local_6128 [72];
  int local_6008;
  int local_6004 [6142];
  undefined4 uStack_c;

  uStack_c = 0x4fa510;
  uVar8 = 0;
  sVar2 = 0;
  piVar3 = &local_6008;
  for (iVar4 = 0x1800; iVar4 != 0; iVar4 = iVar4 + -1) {
    *piVar3 = 0;
    piVar3 = piVar3 + 1;
  }
  puVar5 = local_6128;
  for (iVar4 = 0x48; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  local_613c = 0;
  puVar5 = local_6128;
  do {
    *(short *)(puVar5 + 1) = sVar2;
    *puVar5 = 0xffffffff;
    sVar2 = sVar2 + 1;
    puVar5 = puVar5 + 6;
  } while (sVar2 < 0xc);
  local_6134 = (float)(((uint)local_6134 >> 8 & 0xff) << 8);
  piVar3 = (int *)object_iterator_next();
  do {
    if (piVar3 == (int *)0x0) {
      _qsort(&local_6008,(int)(short)uVar8,0x18,object_dump_compare_by_total_size);
      _qsort(local_6128,0xc,0x18,object_dump_compare_by_total_size);
      _File = (FILE *)FUN_00624186("object_memory.txt",&DAT_00660144);
      if (_File != (FILE *)0x0) {
        FUN_004f7950();
        _fprintf(_File,"#%d objects (#%d active) using %3.2f%% of available memory\n\n",0xffffffff,
                 0xffffffff,(double)(local_6134 * 100.0));
        _fprintf(_File,"OBJECTS BY TYPE\n");
        _fprintf(_File,"number (active) [garbage/   dead/outside/at-rest] maxsize totsize\n");
        iVar4 = 0xc;
        do {
          object_dump_write();
          iVar4 = iVar4 + -1;
        } while (iVar4 != 0);
        _fprintf(_File,"\n");
        _fprintf(_File,"OBJECTS BY DEFINITION\n");
        _fprintf(_File,"number (active) [garbage/   dead/outside/at-rest] maxsize totsize\n");
        if (0 < (short)uVar8) {
          piVar3 = local_6004;
          uVar7 = (uint)uVar8;
          do {
            _fprintf(_File,"% 6d (% 6d) [% 7d/% 7d/% 7d/% 7d] % 7d % 7d %s\r\n",
                     (int)(short)piVar3[2],(int)*(short *)((int)piVar3 + 10),(int)(short)piVar3[3],
                     (int)*(short *)((int)piVar3 + 0xe),(int)(short)piVar3[4],
                     (int)*(short *)((int)piVar3 + 0x12),(int)*(short *)((int)piVar3 + 2),piVar3[1])
            ;
            piVar3 = piVar3 + 6;
            uVar7 = uVar7 - 1;
          } while (uVar7 != 0);
        }
        _fprintf(_File,"\n");
        if (0 < local_613c) {
          _fprintf(_File,
                   "WARNING: overflowed MAXIMUM_DUMPS (%d), this dump does not include %d objects that would not fit!\n"
                   ,0x400);
        }
        _fprintf(_File,"\n");
        _fclose(_File);
      }
      return;
    }
    uVar6 = 0;
    uVar1 = 0xffff;
    if (0 < (short)uVar8) {
      do {
        if (local_6004[(short)uVar6 * 6 + -1] == *piVar3) {
          uVar1 = uVar6;
          if (uVar6 != 0xffff) goto LAB_004fa5e8;
          break;
        }
        uVar6 = uVar6 + 1;
      } while ((short)uVar6 < (short)uVar8);
    }
    uVar6 = uVar1;
    if ((short)uVar8 < 0x400) {
      *(undefined2 *)(local_6004 + (int)(short)uVar8 * 6) = 0xffff;
      local_6004[(short)uVar8 * 6 + -1] = *piVar3;
      uVar6 = uVar8;
      uVar8 = uVar8 + 1;
    }
    else {
      local_613c = local_613c + 1;
    }
LAB_004fa5e8:
    if (uVar6 != 0xffff) {
      object_dump_accumulate_stats();
    }
    object_dump_accumulate_stats();
    piVar3 = (int *)object_iterator_next();
  } while( true );
}
#endif
