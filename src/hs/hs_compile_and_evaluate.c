// hs_compile_and_evaluate  (Ghidra: chimera__execute_script; renamed per
// out/phase4/hs_types_notes.md -- "The real hs_compile_and_evaluate is 0x484400", identified
// by the CEA-PDB hint on this exact address and the "(set %s)" string it carries. The
// "chimera__" prefix belongs to a different function entirely (0x487030, out of this batch's
// range) that was misattributed the compile_and_evaluate name.)
// address 0x484400, size 502 bytes
// name confidence: 0.85   rewrite confidence: 0.55
// evidence: compiles and immediately evaluates a single console/script command line: strips a
// trailing ';' comment, and if the line doesn't already start with '(', auto-wraps it as either
// "(command)" (a bare function/script call, when the first word isn't a known global) or
// "(set command)" (when the first word is a known global followed by more text, i.e. an
// assignment); triggers a full script reload via hs_rebuild_source/hs_compile_source/
// hs_scripts_free/hs_scripts_reload when hs_reload_pending is set.
// register convention: __cdecl, command is the recognized single stack parameter.
// UNSURE: the do-while loop that walks `command` to its NUL terminator right before the
// hs_compile_expression call mutates `command` itself but that mutated value is never read
// again (the actual call uses the separately-saved start pointer) -- dropped as dead code here,
// its only visible effect (advancing a pointer nothing reads) being unobservable.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

extern hs_global_reference hs_find_global_by_name(char *name); // 0x00483480, this batch
extern datum_index hs_compile_expression(char *text, uint32_t length, char **error_message, char **error_offset); // 0x00485540, this batch
extern void hs_evaluate_expression(datum_index node_index); // 0x0048a250, outside this batch's assigned range
extern void data_delete_all(data_array *array); // 0x004d0580
extern void *GlobalFree(void *memory); // EXTERNAL kernel32
extern char hs_rebuild_source(void); // 0x00483e20, this batch
extern char hs_compile_source(void); // 0x00484090, this batch
extern void hs_scripts_free(void); // 0x004832b0, this batch
extern void hs_scripts_reload(void); // 0x00483250, this batch

extern uint8_t hs_compiling;               // 0x006b14b8
extern char *hs_compiled_source;           // 0x006b14c0
extern int32_t hs_compiled_source_length;  // 0x006b14bc
extern uint8_t hs_compile_release_source;  // 0x006b15dd
extern uint8_t hs_syntax_data_dirty;       // 0x006b14d0
extern char *hs_compile_error;             // 0x006b14d4
extern data_array *hs_syntax_data;         // 0x0087a474
extern uint8_t hs_compiled_source_owned;   // 0x006b15dc
extern uint8_t hs_reload_pending;          // 0x006b14a8

// Compiles and immediately executes a single console/script command line, auto-wrapping bare
// global-set or bare-call syntax, and triggers a full script reload when requested.
char hs_compile_and_evaluate(char *command)
{
    char result;
    char buffer[1024];
    char formatted[1024];
    char *semicolon;
    char *p;
    char *space;
    int16_t mode;
    char *format;
    hs_global_reference global;
    datum_index expr;
    char *error_message;
    char *error_offset;
    char *newline;

    result = 0;
    strncpy(buffer, command, 0x400);
    buffer[1023] = 0;
    semicolon = strchr(buffer, ';');
    if (semicolon != 0) {
        buffer[0] = 0;
    }
    p = buffer;
    if (buffer[0] != 0) {
        while (isspace((unsigned char)*p)) {
            p = p + 1;
            if (*p == 0) {
                goto reload_check;
            }
        }
        hs_compiling = 1;
        hs_compiled_source = 0;
        hs_compiled_source_length = 0;
        hs_compile_release_source = 0;
        hs_syntax_data_dirty = 0;
        hs_compile_error = 0;
        mode = 0;
        if (buffer[0] != '(') {
            space = strchr(buffer, ' ');
            if (space != 0) {
                *space = 0;
            }
            global = hs_find_global_by_name(buffer);
            if (global == k_hs_global_reference_none) {
                mode = 1;
            } else {
                mode = 0;
                if (space != 0) {
                    mode = 2;
                }
            }
            if (space != 0) {
                *space = ' ';
            }
        }
        if (mode == 1) {
            format = "(%s)";
            sprintf(formatted, format, buffer);
            command = formatted;
        } else if (mode == 2) {
            format = "(set %s)";
            sprintf(formatted, format, buffer);
            command = formatted;
        }
        expr = hs_compile_expression(command, (uint32_t)strlen(command), &error_message, &error_offset);
        if (expr == k_datum_index_none) {
            if ((error_message != 0) && (error_offset != 0)) {
                newline = strchr(error_offset, '\n');
                if (newline != 0) {
                    *newline = '\0';
                }
            }
        } else {
            result = 1;
            hs_evaluate_expression(expr);
        }
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
    }
reload_check:
    if (hs_reload_pending != 0) {
        if (hs_rebuild_source() != 0) {
            hs_compile_source();
            hs_scripts_free();
            hs_scripts_reload();
        }
        hs_reload_pending = 0;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x484400):

char __cdecl chimera__execute_script(char *command)

{
  byte *pbVar1;
  char cVar2;
  short sVar3;
  int iVar4;
  undefined1 *puVar5;
  byte *pbVar6;
  char *pcVar7;
  char local_809;
  int local_808;
  int local_804;
  byte local_800 [1023];
  undefined1 local_401;
  char local_400 [1024];

  local_809 = '\0';
  _strncpy((char *)local_800,command,0x400);
  local_401 = 0;
  iVar4 = FUN_006257e0(local_800,0x3b);
  if (iVar4 != 0) {
    local_800[0] = 0;
  }
  pbVar6 = local_800;
  if (local_800[0] != 0) {
    while (iVar4 = _isspace((uint)*pbVar6), iVar4 != 0) {
      pbVar1 = pbVar6 + 1;
      pbVar6 = pbVar6 + 1;
      if (*pbVar1 == 0) goto LAB_004845c2;
    }
    DAT_006b14b8 = 1;
    DAT_006b14c0 = (HGLOBAL)0x0;
    DAT_006b14bc = 0;
    DAT_006b15dd = '\0';
    DAT_006b14d0 = '\0';
    DAT_006b14d4 = 0;
    sVar3 = 0;
    if (local_800[0] != 0x28) {
      puVar5 = (undefined1 *)FUN_006257e0(local_800,0x20);
      if (puVar5 != (undefined1 *)0x0) {
        *puVar5 = 0;
      }
      sVar3 = chimera__get_global_index();
      if (sVar3 == -1) {
        sVar3 = 1;
      }
      else {
        sVar3 = 0;
        if (puVar5 == (undefined1 *)0x0) goto LAB_004844de;
        sVar3 = 2;
      }
      if (puVar5 != (undefined1 *)0x0) {
        *puVar5 = 0x20;
      }
    }
LAB_004844de:
    if (sVar3 == 1) {
      pcVar7 = "(%s)";
    }
    else {
      pcVar7 = command;
      if (sVar3 != 2) goto LAB_00484521;
      pcVar7 = "(set %s)";
    }
    _sprintf(local_400,pcVar7,local_800);
    command = local_400;
    pcVar7 = command;
LAB_00484521:
    do {
      cVar2 = *command;
      command = command + 1;
    } while (cVar2 != '\0');
    iVar4 = hs_compile_expression(pcVar7,&local_804,&local_808);
    if (iVar4 == -1) {
      if (((local_804 != 0) && (local_808 != 0)) &&
         (puVar5 = (undefined1 *)FUN_006257e0(local_808,10), puVar5 != (undefined1 *)0x0)) {
        *puVar5 = 0;
      }
    }
    else {
      local_809 = '\x01';
      FUN_0048a250(iVar4);
    }
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
  }
LAB_004845c2:
  if (DAT_006b14a8 != '\0') {
    cVar2 = hs_rebuild_source();
    if (cVar2 != '\0') {
      hs_compile_source();
      hs_scripts_free();
      hs_scripts_reload();
    }
    DAT_006b14a8 = '\0';
  }
  return local_809;
}
#endif
