// object_dump_write  (Ghidra: object_dump_write, already named)
// address 0x4fa490, size 111 bytes
// name confidence: 0.8 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Writes one formatted line of the object memory-dump report
//   (counts, garbage/dead/outside/at-rest tallies, and a definition/type name) to a file")
// rewrite confidence: 0.65
// evidence: types/objects.h object_memory_dump_record (definition_tag 0x00, type 0x04,
//   maximum_size 0x06, total_size 0x08, count 0x0c, active_count 0x0e, garbage_count 0x10,
//   dead_count 0x12, outside_map_count 0x14, at_rest_count 0x16), object_type_definition
//   (name 0x00); global 0x0069bfdc object_type_definitions (12 read-only pointers), 0x0087bc14
//   tag_instances (path at +0x10); callee fprintf (established elsewhere in this codebase).
// register convention: the record in EAX, the output FILE* as the sole stack parameter.
//   Confirmed against objdump -d -M intel bin/halo.exe: 0x4fa490 mov edx,[eax] at entry.
//   // blam-cc: EAX -> record, stack -> file

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern tag_instance *tag_instances; // 0x0087bc14


void object_dump_write(object_memory_dump_record *record, void *file) // blam-cc: EAX -> record, stack -> file
{
    const char *name = "unknown";

    if (record->definition_tag == k_datum_index_none) {
        if (record->type != -1) {
            name = object_type_definitions[record->type]->name;
        }
    } else {
        name = tag_instances[record->definition_tag & 0xffff].path;
    }

    fprintf(file, "% 6d (% 6d) [% 7d/% 7d/% 7d/% 7d] % 7d % 7d %s\r\n",
        record->count, record->active_count, record->garbage_count, record->dead_count,
        record->outside_map_count, record->at_rest_count, record->maximum_size,
        record->total_size, name);
}

#if 0
Original Ghidra decompilation (0x4fa490):

void object_dump_write(FILE *param_1)

{
  int *in_EAX;
  char *pcVar1;

  pcVar1 = "unknown";
  if (*in_EAX == -1) {
    if ((short)in_EAX[1] != -1) {
      pcVar1 = *(char **)(&PTR_PTR_0069bfdc)[(short)in_EAX[1]];
    }
  }
  else {
    pcVar1 = *(char **)((short)*in_EAX * 0x20 + 0x10 + DAT_0087bc14);
  }
  _fprintf(param_1,"% 6d (% 6d) [% 7d/% 7d/% 7d/% 7d] % 7d % 7d %s\r\n",(int)(short)in_EAX[3],
           (int)*(short *)((int)in_EAX + 0xe),(int)(short)in_EAX[4],
           (int)*(short *)((int)in_EAX + 0x12),(int)(short)in_EAX[5],
           (int)*(short *)((int)in_EAX + 0x16),(int)*(short *)((int)in_EAX + 6),in_EAX[2],pcVar1);
  return;
}
#endif
