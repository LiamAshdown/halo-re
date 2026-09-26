// cluster_partition_new  (Ghidra: cluster_partition_new, already named via cea-pdb string match)
// address 0x551e30, size 197 bytes
// name confidence: 0.85   rewrite confidence: 0.85
// evidence: disassembly (objdump -d -M intel bin/halo.exe, 0x551e30..0x551ef4) resolves both
//   register arguments that Ghidra's decompilation dropped entirely:
//   - ESI: the two callers (objects_initialize 0x4f4ad0, lights_initialize 0x4f0a20 via
//     src/objects/objects_initialize.c) load it with `mov esi,0x8603d0` / `mov esi,0x8603c0`
//     immediately before the call -- the address of one of the two adjacent three-global groups
//     types/objects.h already documents (noncollideable_cluster_first/_object_references/
//     _cluster_partition at 0x008603c0, the collideable equivalent at 0x008603d0). This confirms
//     src/objects/objects_initialize.c's own UNSURE note about where those globals come from.
//   - EDI: the same two call sites load it with `mov edi,0x66e944` ("collideable object") /
//     `mov edi,0x66e92c` ("noncollideable object") -- a category NAME string. Ghidra's
//     decompiled `_sprintf(local_200,"cluster %s")` silently drops this exact register argument;
//     disassembly at 0x551e68 (`push edi` / `push offset "cluster %s"` / `push eax` / call
//     sprintf) shows it is pushed as the vararg. The other call site's rewrite
//     (src/objects/objects_initialize.c) is not touched by this file; its two
//     `cluster_partition_new()` calls stay argument-less there because that file predates this
//     resolution -- a future pass over src/objects could add the two arguments back.
//   - `mov ebx,0xc` immediately before both game_state_new calls: the pool element size,
//     confirming types/game.h's documented "game_state_new takes its stride in EBX" convention.
//     0xc == sizeof(object_cluster_reference) (types/objects.h).
// register convention: ESI -> out (the three-global group), EDI -> name (category string, e.g.
//   "collideable object"). blam-cc order would place EDI before ESI (ECX, EDX, EBX, ESI, EDI);
//   they are listed by the order the source naturally uses them here.
// UNSURE: whether out->cluster_first (the first word, a bump-allocated but otherwise
//   uninitialized 0x800-byte region) is expected to already be zero-filled by the game-state
//   arena allocator, or whether some other function clears it before first use; nothing in this
//   function or its two known callers zeroes it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "structures.h"

extern uint8_t *game_state_base;   // 0x006e2dc8, game.h (saved_games)
extern int32_t game_state_cursor;  // 0x006e2dcc, game.h (saved_games)
extern uint32_t game_state_crc;    // 0x006e2dd4, game.h (saved_games)

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0, memory module
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count
    // 0x5380d0; name and maximum_count on the stack, element_size in EBX
    // (blam-cc: EBX -> element_size)
extern int32_t sprintf(char *buffer, const char *format, ...); // 0x00623693 _sprintf

// Reserves a new cluster-reference partition for one object category (e.g. "collideable" /
// "noncollideable"): an 0x800-byte, CRC-tracked per-cluster head table, and two named,
// CRC-tracked object_cluster_reference pools -- one for the clusters' chains of referencing
// objects, one for the objects' chains of referenced clusters.
void cluster_partition_new(cluster_reference_group *out, char *name)
    // blam-cc: ESI -> out, EDI -> name
{
    char format_buffer[256];
    char pool_name[256];
    int32_t size;
    uint8_t *region;

    region = game_state_base + game_state_cursor;
    game_state_cursor = game_state_cursor + 0x800;
    size = 0x800;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    out->cluster_first = (datum_index *)region;

    sprintf(format_buffer, "cluster %s", name);
    sprintf(pool_name, "%s reference", format_buffer);
    out->cluster_object_references = game_state_new(pool_name, 0x800, sizeof(object_cluster_reference));

    sprintf(format_buffer, "%s cluster", name);
    sprintf(pool_name, "%s reference", format_buffer);
    out->object_cluster_references = game_state_new(pool_name, 0x800, sizeof(object_cluster_reference));
}

#if 0
Original Ghidra decompilation (0x551e30):

void cluster_partition_new(void)

{
  int iVar1;
  int *unaff_ESI;
  undefined4 local_204;
  char local_200 [256];
  char local_100 [256];

  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x800;
  local_204 = 0x800;
  crc32_update(&DAT_006e2dd4,&local_204,4);
  *unaff_ESI = iVar1;
  _sprintf(local_200,"cluster %s");
  _sprintf(local_100,"%s reference",local_200);
  iVar1 = game_state_new(local_100,0x800);
  unaff_ESI[1] = iVar1;
  _sprintf(local_200,"%s cluster");
  _sprintf(local_100,"%s reference",local_200);
  iVar1 = game_state_new(local_100,0x800);
  unaff_ESI[2] = iVar1;
  return;
}

Disassembly (objdump -d -M intel, resolving ESI/EDI and the sprintf vararg Ghidra dropped):

00551e30:
  mov eax,ds:0x6e2dcc ; mov ecx,[0x6e2dc8] ; lea ebx,[eax+ecx] ; add eax,0x800
  mov [0x6e2dcc],eax ; call crc32_update(&0x6e2dd4, &size_local, 4)
  push edi
  lea eax,[esp+0x18] ; push offset "cluster %s" ; push eax
  mov [esi],ebx                 ; out->cluster_first = base
  call sprintf                   ; sprintf(local_200, "cluster %s", edi/name)
  lea ecx,[esp+0x20] ; push ecx ; lea edx,[esp+0x124] ; push offset "%s reference" ; push edx
  call sprintf                   ; sprintf(local_100, "%s reference", local_200)
  lea eax,[esp+0x12c] ; push 0x800 ; push eax ; mov ebx,0xc ; call game_state_new
  push edi
  lea ecx,[esp+0x38] ; push offset "%s cluster" ; push ecx
  mov [esi+4],eax               ; out->cluster_object_references = pool
  call sprintf                   ; sprintf(local_200, "%s cluster", edi/name)
  lea edx,[esp+0x40] ; push edx ; lea eax,[esp+0x144] ; push offset "%s reference" ; push eax
  call sprintf                   ; sprintf(local_100, "%s reference", local_200)
  lea ecx,[esp+0x108] ; push 0x800 ; push ecx ; call game_state_new
  mov [esi+8],eax                ; out->object_cluster_references = pool
  pop ebx ; ret
#endif
