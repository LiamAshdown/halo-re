// hash_table_set_or_remove  (orphan pass 4: FUN_004f0530, no Ghidra name)
// address 0x4f0530, size 173 bytes
// name confidence: 0.75 (matches the generic open-hash-table family declared in objects.h --
//   hash_table_set_or_remove, hash_table_get, hash_table_grow_freelist -- and its behaviour,
//   insert-or-update when value != -1 and remove when value == -1, matches the name exactly)
// rewrite confidence: 0.7 (register convention and control flow confirmed against objdump;
//   the struct layout is the existing types/objects.h hash_table, already used by callers in
//   this module such as object_delete_unparented.c and object_queue_pickup_denied_event.c)
// evidence: types/objects.h hash_table (initialized 0x00, bucket_count 0x04, buckets 0x08,
//   entry_count 0x0c, freelist 0x10, blocks 0x14), hash_bucket {count, first} 0x08,
//   hash_node {key, value, next} 0x0c; callee hash_table_grow_freelist (0x4f0620, same file).
//   This pass: src/objects/README.md "hash_table - generic engine code this module happens to
//   own" and out/phase4/objects_types_notes.md "hash_table_set_or_remove 0x4f0530,
//   hash_table_get 0x4f05e0 and hash_table_grow_freelist 0x4f0620 ... it is declared in
//   objects.h because no other recovered module owns it."
// register convention: table pointer in EAX, key in EBX, value (or -1 to remove) is a stack
//   argument -- objdump confirms `mov ebp, [esp+8]` before any push, i.e. the caller's first
//   stack slot, then EAX is moved into ESI and used as the table pointer for the rest of the
//   function.
// blam-cc: hash_table_set_or_remove(hash_table *table /*EAX*/, int32_t key /*EBX*/, int32_t value /*stack*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_objects.h"


void hash_table_set_or_remove(hash_table *table, int32_t key, int32_t value)
{
    hash_bucket *bucket;
    hash_node *prev;
    hash_node *node;
    int32_t unsigned_key;

    if (table->initialized != 1) {
        return;
    }

    unsigned_key = (key < 0) ? -key : key;
    bucket = table->buckets + (unsigned_key % table->bucket_count);

    prev = 0;
    for (node = bucket->first; node != 0; node = node->next) {
        if (node->key == key) {
            if (value == -1) {
                if (prev == 0) {
                    bucket->first = node->next;
                } else {
                    prev->next = node->next;
                }
                node->key = -1;
                node->value = -1;
                node->next = table->freelist;
                table->freelist = node;
                bucket->count--;
                table->entry_count--;
                return;
            }
            node->value = value;
            return;
        }
        prev = node;
    }

    if (table->freelist == 0) {
        hash_table_grow_freelist(table);
    }
    node = table->freelist;
    table->freelist = node->next;
    node->key = key;
    node->value = value;
    node->next = bucket->first;
    bucket->first = node;
    bucket->count++;
    table->entry_count++;
}

#if 0
Original Ghidra decompilation (0x4f0530):

void hash_table_set_or_remove(int param_1)

{
  int *piVar1;
  int *piVar2;
  int *piVar3;
  char *in_EAX;
  int iVar4;
  int unaff_EBX;

  if (*in_EAX == '\x01') {
    iVar4 = unaff_EBX;
    if (unaff_EBX < 0) {
      iVar4 = -unaff_EBX;
    }
    piVar1 = (int *)(*(int *)(in_EAX + 8) + (iVar4 % *(int *)(in_EAX + 4)) * 8);
    piVar2 = (int *)0x0;
    for (piVar3 = (int *)piVar1[1]; piVar3 != (int *)0x0; piVar3 = (int *)piVar3[2]) {
      if (*piVar3 == unaff_EBX) {
        if (param_1 == -1) {
          if (piVar2 == (int *)0x0) {
            piVar1[1] = piVar3[2];
          }
          else {
            piVar2[2] = piVar3[2];
          }
          *piVar3 = -1;
          piVar3[1] = -1;
          piVar3[2] = *(int *)(in_EAX + 0x10);
          *(int **)(in_EAX + 0x10) = piVar3;
          *piVar1 = *piVar1 + -1;
          *(int *)(in_EAX + 0xc) = *(int *)(in_EAX + 0xc) + -1;
          return;
        }
        piVar3[1] = param_1;
        return;
      }
      piVar2 = piVar3;
    }
    if (*(int *)(in_EAX + 0x10) == 0) {
      hash_table_grow_freelist();
    }
    piVar2 = *(int **)(in_EAX + 0x10);
    *(int *)(in_EAX + 0x10) = piVar2[2];
    *piVar2 = unaff_EBX;
    piVar2[1] = param_1;
    piVar2[2] = piVar1[1];
    piVar1[1] = (int)piVar2;
    *piVar1 = *piVar1 + 1;
    *(int *)(in_EAX + 0xc) = *(int *)(in_EAX + 0xc) + 1;
  }
  return;
}
#endif
