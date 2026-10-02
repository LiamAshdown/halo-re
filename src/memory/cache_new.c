// cache_new
// address 0x4d1750, size 152 bytes
// name confidence: 0.8 (module summary; every field write matches the cache/data_array layout in
// types/memory.h byte-for-byte, including the two literal-byte signature stamps 'd@t@'/'weee')
// rewrite confidence: 0.75
// evidence: types/memory.h cache/cache_entry/data_array; out/phase4/memory_types_notes.md's
// "cache_new @0x4d1750 writes the whole container..." paragraph and its two call-site examples
// (sound_cache_new, texture_cache_new) confirming param order and that +0x2c is a block *shift*.
// The notes label the first call-site argument "pool", but this function's own body proves that
// argument is the cache object's own memory (every field, down to the two magic-byte signatures,
// is written directly through it) -- it is treated here as the cache to initialize, however the
// caller obtained that memory (plausibly carved from a pool by the caller, per the notes).
// register convention: name string in EBX (unaff_EBX); stack: cache object to initialize
// (param_1), block_count (param_2), block_shift (param_3), maximum_count (param_4, int16),
// release_procedure (param_5), in_use_procedure (param_6).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// strncpy (0x00623a90 strncpy) comes from <string.h>.
extern void data_delete_all(data_array *array); // 0x4d0580, below this batch's assigned range

void cache_new(char *name, cache *self, int32_t block_count, int32_t block_shift,
    int16_t maximum_count, void *release_procedure, void *in_use_procedure)
{
    data_array *entries = &self->entry_data;

    memset(entries, 0, sizeof(*entries));
    strncpy(entries->name, name, 0x1f);
    entries->maximum_count = maximum_count;
    entries->size = sizeof(cache_entry); // 0x1c
    entries->signature = k_data_array_signature; // 'd@t@'
    entries->data = (uint8_t *)self + 0x7c;
    entries->valid = 0;
    entries->valid = 1; // Ghidra writes 0 then immediately 1; preserved verbatim.
    data_delete_all(entries);

    memset(self, 0, 0x44);
    strncpy(self->name, name, 0x1f);
    self->release_procedure = release_procedure;
    self->in_use_procedure = in_use_procedure;
    self->entries = entries;
    self->block_count = block_count;
    self->block_shift = block_shift;
    self->signature = k_cache_signature; // 'weee'
    self->first = (datum_index)0xffffffff; // k_datum_index_none
    self->last = (datum_index)0xffffffff;
    self->age = 1;
}

#if 0
Original Ghidra decompilation (0x4d1750):

void cache_new(char *param_1,undefined4 param_2,undefined4 param_3,undefined2 param_4,
              undefined4 param_5,undefined4 param_6)

{
  char *_Dest;
  int iVar1;
  char *unaff_EBX;
  char *pcVar2;

  _Dest = param_1 + 0x44;
  pcVar2 = _Dest;
  for (iVar1 = 0xe; iVar1 != 0; iVar1 = iVar1 + -1) {
    pcVar2[0] = '\0';
    pcVar2[1] = '\0';
    pcVar2[2] = '\0';
    pcVar2[3] = '\0';
    pcVar2 = pcVar2 + 4;
  }
  _strncpy(_Dest,unaff_EBX,0x1f);
  *(undefined2 *)(param_1 + 100) = param_4;
  param_1[0x66] = '\x1c';
  param_1[0x67] = '\0';
  param_1[0x6c] = '@';
  param_1[0x6d] = 't';
  param_1[0x6e] = '@';
  param_1[0x6f] = 'd';
  *(char **)(param_1 + 0x78) = param_1 + 0x7c;
  param_1[0x68] = '\0';
  param_1[0x68] = '\x01';
  data_delete_all();
  pcVar2 = param_1;
  for (iVar1 = 0x11; iVar1 != 0; iVar1 = iVar1 + -1) {
    pcVar2[0] = '\0';
    pcVar2[1] = '\0';
    pcVar2[2] = '\0';
    pcVar2[3] = '\0';
    pcVar2 = pcVar2 + 4;
  }
  _strncpy(param_1,unaff_EBX,0x1f);
  *(undefined4 *)(param_1 + 0x20) = param_5;
  *(undefined4 *)(param_1 + 0x24) = param_6;
  *(char **)(param_1 + 0x3c) = _Dest;
  *(undefined4 *)(param_1 + 0x28) = param_2;
  *(undefined4 *)(param_1 + 0x2c) = param_3;
  param_1[0x40] = 'e';
  param_1[0x41] = 'e';
  param_1[0x42] = 'e';
  param_1[0x43] = 'w';
  param_1[0x34] = -1;
  param_1[0x35] = -1;
  param_1[0x36] = -1;
  param_1[0x37] = -1;
  param_1[0x38] = -1;
  param_1[0x39] = -1;
  param_1[0x3a] = -1;
  param_1[0x3b] = -1;
  param_1[0x30] = '\x01';
  param_1[0x31] = '\0';
  param_1[0x32] = '\0';
  param_1[0x33] = '\0';
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
