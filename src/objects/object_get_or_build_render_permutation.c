// object_get_or_build_render_permutation  (Ghidra: FUN_004f9b70; renamed, Blam-style, not
// previously named)
// address 0x4f9b70, size 234 bytes
// name confidence: 0.3 (matches functions.md's summary: "Returns a cached render permutation
//   index for a tag/object combination, building and caching a new one on first use"; also the
//   callee object_refresh_local_player_render_cache.c, this batch, already forwards this
//   exact address a {tag_index, ...} pair pointer under this same guessed name)
// rewrite confidence: 0.2 (foreign callees object_placement_data_initialize/euler_angles_to_basis_vectors/object_new/object_type_definitions_notify_two_args_0x2c
//   and the per-tag table at param_1+4 are not otherwise established in this module; preserved
//   close to the raw pointer arithmetic rather than fully re-derived)
// evidence: types/objects.h object_globals (unknown_00 0x00); global 0x006b8cbc
//   object_globals_pointer, 0x006b8cb8 object_name_list (the same 0x200-entry table
//   object_reserve_render_cache_slot/object_release_render_cache_slot use, this batch); callee
//   object_reserve_render_cache_slot (0x4f9ac0, this batch).
// register convention: a {tag_index int16, name_index int16, flags} pointer in EDI, a
//   per-tag-table owner pointer as the sole stack parameter. Confirmed against objdump
//   -d -M intel bin/halo.exe: 0x4f9b70 mov dx,[edi] at entry, 0x4f9bc9 mov edx,[esp+0x94] for
//   the stack argument.
//   // blam-cc: EDI -> pair, stack -> table_owner

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern object_globals *object_globals_pointer; // 0x006b8cbc
extern datum_index *object_name_list; // 0x006b8cb8

extern void object_placement_data_initialize(void *out, int32_t entry, uint32_t flag); // 0x4f53a0, UNSURE: object_placement_data_initialize-shaped, see file header
extern void euler_angles_to_basis_vectors(void); // 0x4cdde0, foreign module, UNSURE
extern int32_t object_new(void); // 0x4f5460, UNSURE
extern void object_type_definitions_notify_two_args_0x2c(int32_t handle); // 0x4f3f20, outside this batch (object_type_definitions_notify_two_args_0x2c-shaped), UNSURE
extern void object_reserve_render_cache_slot(uint32_t object_index, int16_t slot); // 0x4f9ac0, this batch, UNSURE: args guessed

int32_t object_get_or_build_render_permutation(int16_t *pair, uint8_t *table_owner) // blam-cc: EDI -> pair, stack -> table_owner
{
    int32_t result = -1;

    if (pair[0] != -1) {
        if ((object_globals_pointer->unknown_00 == 0) || ((*(uint8_t *)(pair + 2) & 1) == 0)) {
            int16_t name_index = pair[1];
            uint8_t name_ok = (name_index == -1) ||
                (name_index < 0) || (name_index >= 0x200) ||
                (object_name_list[name_index] == k_datum_index_none);

            if (name_ok) {
                uint8_t *table = *(uint8_t **)(table_owner + 4);
                int32_t entry = *(int32_t *)(table + pair[0] * 0x30 + 0xc);
                if (entry != -1) {
                    uint8_t scratch[16]; // UNSURE: the real out-buffer object_placement_data_initialize expects
                    object_placement_data_initialize(scratch, entry, 0xffffffff);
                    euler_angles_to_basis_vectors();
                    result = object_new();
                    if (result != -1) {
                        object_type_definitions_notify_two_args_0x2c(result);
                        if (name_index != -1) {
                            object_reserve_render_cache_slot((uint32_t)result, name_index);
                        }
                    }
                }
            }
        }
    }

    return result;
}

#if 0
Original Ghidra decompilation (0x4f9b70):

int FUN_004f9b70(int param_1)

{
  short sVar1;
  int iVar2;
  int iVar3;
  short *unaff_EDI;

  iVar3 = -1;
  if ((((*unaff_EDI != -1) && ((*DAT_006b8cbc == '\0' || ((*(byte *)(unaff_EDI + 2) & 1) == 0)))) &&
      ((sVar1 = unaff_EDI[1], sVar1 == -1 ||
       (((sVar1 < 0 || (0x1ff < sVar1)) || (*(int *)(DAT_006b8cb8 + sVar1 * 4) == -1)))))) &&
     (iVar2 = *(int *)(*unaff_EDI * 0x30 + *(int *)(param_1 + 4) + 0xc), iVar2 != -1)) {
    FUN_004f53a0(iVar2,0xffffffff);
    FUN_004cdde0();
    iVar3 = FUN_004f5460();
    if ((iVar3 != -1) && (FUN_004f3f20(iVar3), unaff_EDI[1] != -1)) {
      FUN_004f9ac0();
    }
  }
  return iVar3;
}
#endif
