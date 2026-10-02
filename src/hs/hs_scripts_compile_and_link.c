// hs_scripts_compile_and_link  (Ghidra: hs_scripts_compile_and_link, already named)
// address 0x483190, size 179 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: allocates a syntax-node table, points it at the scenario's own script_syntax_data
// storage (data = table + 0x38, i.e. the inline layout data_new normally allocates), then either
// relinks the already-compiled tree via hs_compile_postprocess or, if there are no scripts yet
// but there are source files (or relinking failed), recompiles everything from source with
// hs_compile_source and postprocesses again. restore_previous puts the caller's previous
// hs_syntax_data back before returning, used when this is a scratch compile.
// register convention: __cdecl, restore_previous is the recognized single stack parameter.
// UNSURE: local_4/local_8 (here error_message/error_offset) are discarded by every caller in
// this batch; they exist only because hs_compile_postprocess always wants an error-out pair.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_allocate_script_node_table(void); // 0x00483100, this batch
extern char hs_compile_source(void); // 0x00484090, this batch
extern char hs_compile_postprocess(char **error_message, int32_t *error_offset); // 0x004858c0, this batch
extern void data_delete_all(data_array *array); // 0x004d0580

extern Scenario *global_scenario;      // 0x00746f8c
extern data_array *hs_syntax_data;     // 0x0087a474

// (Re)compiles and links every script in the loaded scenario into a fresh syntax-node table
// backed by the scenario's own script_syntax_data storage. If restore_previous is set, the
// caller's previous hs_syntax_data is restored before returning either way.
char hs_scripts_compile_and_link(char restore_previous)
{
    data_array *saved_syntax_data;
    Scenario *scenario;
    char no_scripts_but_source_files;
    char success;
    char *error_message;
    int32_t error_offset;

    saved_syntax_data = hs_syntax_data;
    scenario = global_scenario;
    success = 1;
    hs_allocate_script_node_table();
    no_scripts_but_source_files = (scenario->scripts.count == 0) && (0 < scenario->source_files.count);
    hs_syntax_data = (data_array *)scenario->script_syntax_data.pointer;
    hs_syntax_data->data = (uint8_t *)hs_syntax_data + 0x38;
    if (no_scripts_but_source_files || (hs_compile_postprocess(&error_message, &error_offset) == 0)) {
        success = hs_compile_source();
        if ((success != 0) && (hs_compile_postprocess(&error_message, &error_offset) != 0)) {
            success = 1;
            goto restore;
        }
        data_delete_all(hs_syntax_data);
    } else if (0x3ff < (int32_t)scenario->script_string_data.size) {
        goto restore;
    }
    success = 0;
restore:
    if (restore_previous != 0) {
        hs_syntax_data = saved_syntax_data;
    }
    return success;
}

#if 0
Original Ghidra decompilation (0x483190):

char __cdecl hs_scripts_compile_and_link(char restore_previous)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  char cVar4;
  char cVar5;
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  iVar3 = DAT_0087a474;
  iVar2 = DAT_00746f8c;
  cVar5 = '\x01';
  hs_allocate_script_node_table();
  if ((*(int *)(iVar2 + 0x49c) == 0) && (0 < *(int *)(iVar2 + 0x4c0))) {
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  DAT_0087a474 = *(int *)(iVar2 + 0x480);
  *(int *)(DAT_0087a474 + 0x34) = DAT_0087a474 + 0x38;
  if ((bVar1) || (cVar4 = hs_compile_postprocess(local_4,local_8), cVar4 == '\0')) {
    cVar5 = hs_compile_source();
    if ((cVar5 != '\0') && (cVar5 = hs_compile_postprocess(local_4,local_8), cVar5 != '\0')) {
      cVar5 = '\x01';
      goto LAB_0048322c;
    }
    data_delete_all();
  }
  else if (0x3ff < *(int *)(iVar2 + 0x488)) goto LAB_0048322c;
  cVar5 = '\0';
LAB_0048322c:
  if (restore_previous != '\0') {
    DAT_0087a474 = iVar3;
  }
  return cVar5;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
