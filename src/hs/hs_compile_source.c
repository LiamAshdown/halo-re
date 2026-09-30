// hs_compile_source  (Ghidra: hs_compile_source, already named)
// address 0x484090, size 279 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: CEA-PDB string match on "scripts successfully compiled."; calls hs_compile once
// per Scenario::source_files entry (offset 0x4c0/0x4c4, stride 0x34, confirmed in
// out/phase4/hs_types_notes.md) and truncates each error message at its first newline before
// reporting overall success/failure.
// register convention: __cdecl, no parameters.
// UNSURE: hs_compile's error_offset out-parameter is declared int32_t but is actually used
// here as a raw char* into the compiled source (hs_compile computes it as
// hs_compile_error_offset + source_text, a pointer). Preserved via a cast rather than changing
// hs_compile's own declared type.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_memory.h"
#include <string.h>


extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, AL clear_first

extern Scenario *global_scenario;              // 0x00746f8c
extern uint8_t hs_compiling;                   // 0x006b14b8
extern char *hs_compiled_source;               // 0x006b14c0
extern int32_t hs_compiled_source_length;      // 0x006b14bc
extern uint8_t hs_compile_release_source;      // 0x006b15dd
extern uint8_t hs_syntax_data_dirty;           // 0x006b14d0
extern char *hs_compile_error;                 // 0x006b14d4
extern data_array *hs_syntax_data;             // 0x0087a474
extern uint8_t hs_compiled_source_owned;       // 0x006b15dc

// Recompiles every registered HS script source file in turn, truncating each error message at
// its first newline, and prints a success message if every one compiled cleanly. Frees the
// compiled-source buffer according to hs_compile_release_source / hs_compiled_source_owned.
char hs_compile_source(void)
{
    Scenario *scenario;
    char all_ok;
    int16_t i;
    int32_t count;
    ScenarioSourceFile *source_files;
    char *error_message;
    int32_t error_offset;
    char *newline;

    scenario = global_scenario;
    all_ok = 1;
    hs_compiling = 1;
    hs_compiled_source = 0;
    hs_compiled_source_length = 0;
    hs_compile_release_source = 1;
    hs_syntax_data_dirty = 0;
    hs_compile_error = 0;
    data_delete_all(hs_syntax_data);
    i = 0;
    count = (int32_t)scenario->source_files.count;
    if (0 < count) {
        source_files = (ScenarioSourceFile *)scenario->source_files.pointer;
        do {
            hs_compile((int32_t)source_files[i].source.size, (char *)source_files[i].source.pointer,
                       &error_message, &error_offset);
            if (error_message != 0) {
                if (error_offset != 0) {
                    newline = strchr((char *)error_offset, '\n');
                    if (newline != 0) {
                        *newline = '\0';
                    }
                }
                all_ok = 0;
            }
            i = i + 1;
        } while (i < count);
        if (all_ok == 0) {
            goto cleanup;
        }
    }
    console_print_error_va(0, "scripts successfully compiled.");
cleanup:
    if (hs_compile_release_source != 0) {
        if (hs_syntax_data_dirty != 0) {
            data_delete_all(hs_syntax_data);
        }
        if (hs_compiled_source != 0) {
            GlobalFree(hs_compiled_source);
        }
    }
    if (hs_compiled_source_owned != 0) {
        GlobalFree(hs_compiled_source);
        hs_compiled_source = 0;
        hs_compiled_source_owned = 0;
    }
    hs_compiling = 0;
    return all_ok;
}

#if 0
Original Ghidra decompilation (0x484090):

char __cdecl hs_compile_source(void)

{
  int iVar1;
  int iVar2;
  undefined1 *puVar3;
  short sVar4;
  char local_9;
  int local_8;
  int local_4;

  iVar1 = DAT_00746f8c;
  local_9 = '\x01';
  DAT_006b14b8 = 1;
  DAT_006b14c0 = (HGLOBAL)0x0;
  DAT_006b14bc = 0;
  DAT_006b15dd = '\x01';
  DAT_006b14d0 = '\0';
  DAT_006b14d4 = 0;
  data_delete_all();
  sVar4 = 0;
  if (0 < *(int *)(iVar1 + 0x4c0)) {
    iVar2 = 0;
    do {
      iVar2 = iVar2 * 0x34 + *(int *)(iVar1 + 0x4c4);
      hs_compile(*(int *)(iVar2 + 0x20),*(int *)(iVar2 + 0x2c),&local_8,&local_4);
      if (local_8 != 0) {
        if (local_4 != 0) {
          puVar3 = (undefined1 *)FUN_006257e0(local_4,10);
          if (puVar3 != (undefined1 *)0x0) {
            *puVar3 = 0;
          }
        }
        local_9 = '\0';
      }
      sVar4 = sVar4 + 1;
      iVar2 = (int)sVar4;
    } while (iVar2 < *(int *)(iVar1 + 0x4c0));
    if (local_9 == '\0') goto LAB_0048414d;
  }
  console_print_error_va("scripts successfully compiled.");
LAB_0048414d:
  if (DAT_006b15dd != '\0') {
    if (DAT_006b14d0 != '\0') {
      data_delete_all();
    }
    if (DAT_006b14c0 != (HGLOBAL)0x0) {
      GlobalFree(DAT_006b14c0);
    }
  }
  if (DAT_006b15dc != '\0') {
    GlobalFree(DAT_006b14c0);
    DAT_006b14c0 = (HGLOBAL)0x0;
    DAT_006b15dc = '\0';
  }
  DAT_006b14b8 = 0;
  return local_9;
}
#endif
