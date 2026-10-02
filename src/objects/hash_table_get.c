// hash_table_get  (orphan pass 4: FUN_004f05e0, no Ghidra name)
// address 0x4f05e0, size 58 bytes
// name confidence: 0.8 (35 recorded callers already invoke it under this name, e.g.
//   src/objects/object_delete_unparented.c and object_queue_pickup_denied_event.c, which
//   declare `extern int32_t hash_table_get(hash_table *table, uint32_t key);`)
// rewrite confidence: 0.75 (control flow and register convention confirmed against objdump;
//   struct layout is the existing types/objects.h hash_table)
// evidence: types/objects.h hash_table, hash_bucket, hash_node (see hash_table_set_or_remove.c
//   in this same file for the full layout note). out/phase4/objects_types_notes.md groups this
//   with hash_table_set_or_remove 0x4f0530 and hash_table_grow_freelist 0x4f0620 as "generic
//   engine code that happens to live in the objects module".
// register convention: table pointer in ESI, key in ECX, return value in EAX (-1 when absent
//   or when the table is uninitialized or the key is -1) -- confirmed by objdump.
// blam-cc: hash_table_get(hash_table *table /*ESI*/, int32_t key /*ECX*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t hash_table_get(hash_table *table, int32_t key)
{
    hash_bucket *bucket;
    hash_node *node;
    int32_t unsigned_key;

    if (table->initialized != 1 || key == -1) {
        return -1;
    }

    unsigned_key = (key < 0) ? -key : key;
    bucket = table->buckets + (unsigned_key % table->bucket_count);

    for (node = bucket->first; node != 0; node = node->next) {
        if (node->key == key) {
            return node->value;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4f05e0):

int hash_table_get(void)

{
  int *piVar1;
  int iVar2;
  int in_ECX;
  char *unaff_ESI;

  if ((*unaff_ESI == '\x01') && (in_ECX != -1)) {
    iVar2 = in_ECX;
    if (in_ECX < 0) {
      iVar2 = -in_ECX;
    }
    for (piVar1 = *(int **)(*(int *)(unaff_ESI + 8) + 4 + (iVar2 % *(int *)(unaff_ESI + 4)) * 8);
        piVar1 != (int *)0x0; piVar1 = (int *)piVar1[2]) {
      if (*piVar1 == in_ECX) {
        return piVar1[1];
      }
    }
  }
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
