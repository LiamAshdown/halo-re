// hwreq_parser_parse_propertyset_directive  (Ghidra: hwreq_parser_parse_propertyset_directive,
//   already named)
// address 0x57a3e0, size 665 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Parses a
//   'propertyset = \"name\"' directive from the requirements script, allocating a property-set
//   record, parsing its nested block, and registering it in the parser's property-set list."
//   Loops scanning for "propertyset" / "vendor" / "applytoall" lines exactly like
//   hwreq_parser_scan_for_applytoall_or_vendor 0x57a220 and hwreq_parser_scan_for_applytoall
//   0x57a320; a "propertyset" match parses "= \"name\"", allocates a property set, parses its
//   { ... } block into it, and inserts it into this->property_sets keyed by name, then keeps
//   scanning; a "vendor" or "applytoall" line stops the scan (leaving the cursor there) and
//   returns true.
// register convention: this in ECX (objdump: "mov esi,ecx" at entry -- the only true __thiscall
//   seen so far in this module). hwreq_token_parse_quoted_string 0x578c60 takes the parser in
//   EAX. The two library helpers take their string argument by register: FUN_0057bc90 (declared
//   here as msvc_string_assign_n) is a thiscall, ECX = destination string, with source pointer
//   and length on the stack; FUN_0057b6e0 (declared here as hwreq_property_set_map_index) takes
//   the key string in EDI and the map address on the stack.
// blam-cc: this in ECX (only recognized parameter; the whole body uses it via ESI once copied).
// UNSURE: Ghidra's decompile (which flags two unreachable-block removals) drops the local
//   std::string temporary's destructor on the "parse_block failed" exit path entirely; objdump
//   (0x57a629..0x57a642) shows it runs there exactly like on the success path, so it is restored
//   here -- otherwise a failed propertyset block would leak the name string's heap buffer.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hwreq_parser_report_error(hwreq_parser *self, const char *message); // 0x578a20, blam-cc: this in ESI (live-in), message on the stack; below this module's rewrite range
extern char *hwreq_token_parse_quoted_string(hwreq_parser *self); // 0x578c60, blam-cc: this in EAX; below this module's rewrite range; NULL on error
extern void msvc_string_assign_n(msvc_std_string *dest, const char *source, uint32_t length); // 0x57bc90, blam-cc: dest in ECX, source/length on the stack; library code, not in the function list
extern hwreq_property_set **hwreq_property_set_map_index(msvc_std_string *key, msvc_std_map *map); // 0x57b6e0, blam-cc: key in EDI, map on the stack; library code, not in the function list; map::operator[], returns the (possibly freshly inserted) value slot
extern void hwreq_property_set_flags_destruct(hwreq_property_set *set); // 0x57b990, blam-cc: set in EBX; library code, not in the function list
extern uint8_t hwreq_parser_parse_block(hwreq_parser *self, hwreq_property_set *target); // 0x57af10

// Scans forward from the parser's cursor for "propertyset" directives, parsing each one's
// "= \"name\" { ... }" form into a freshly allocated property set that is registered in
// this->property_sets under that name, and keeps scanning past it; stops (leaving the cursor
// unconsumed, returning true) at the first line starting with "vendor" or "applytoall", or once
// the end of the file is reached. Returns false on a malformed directive or a failed nested
// block parse.
uint8_t hwreq_parser_parse_propertyset_directive(hwreq_parser *self)
{
    char *cursor;
    char *line;
    char c;
    msvc_std_string name;
    char *quoted;
    uint32_t length;
    hwreq_property_set *set;
    uint8_t result;
    hwreq_property_set **slot;

    for (;;) {
        if (_strnicmp((char *)self->cursor, "propertyset", 11) == 0) {
            cursor = (char *)self->cursor + 11;
            c = *cursor;
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                while (*cursor == ' ' || *cursor == '\t') {
                    cursor++;
                }
                self->cursor = (uint32_t)cursor;
                if (*cursor != '=') {
                    hwreq_parser_report_error(self, "Missing =");
                    return 0;
                }
                self->cursor = self->cursor + 1;

                quoted = hwreq_token_parse_quoted_string(self);
                if (quoted == 0) {
                    return 0;
                }

                length = 0;
                while (quoted[length] != 0) {
                    length++;
                }
                name.capacity = k_msvc_string_inline_capacity;
                name.size = 0;
                name.buffer.inline_buffer[0] = 0;
                msvc_string_assign_n(&name, quoted, length);

                // consume the rest of the "propertyset = ..." line
                do {
                    line = (char *)self->cursor;
                    self->cursor = (uint32_t)(line + 1);
                    if (*line == '\r') break;
                } while ((char *)self->cursor < (char *)self->end);
                if ((char *)self->cursor < (char *)self->end && *(char *)self->cursor == '\n') {
                    self->cursor = (uint32_t)(line + 2);
                }
                self->line_start = self->cursor;
                self->line_number = self->line_number + 1;

                set = (hwreq_property_set *)malloc(k_hwreq_property_set_size);
                if (set != 0) {
                    set->flags.first = 0;
                    set->flags.last = 0;
                    set->flags.end = 0;
                    set->owner = (uint32_t)self;
                }

                result = hwreq_parser_parse_block(self, set);
                if (result == 0) {
                    if (set != 0) {
                        hwreq_property_set_flags_destruct(set);
                        free(set);
                    }
                    if (name.capacity > k_msvc_string_inline_capacity) {
                        free((void *)name.buffer.heap_buffer);
                    }
                    return 0;
                }

                slot = hwreq_property_set_map_index(&name, &self->property_sets);
                *slot = set;

                if (name.capacity > k_msvc_string_inline_capacity) {
                    free((void *)name.buffer.heap_buffer);
                }
                name.capacity = k_msvc_string_inline_capacity;
                name.size = 0;
                name.buffer.inline_buffer[0] = 0;

                goto skip_line_and_continue;
            }
        } else if (_strnicmp((char *)self->cursor, "vendor", 6) == 0) {
            c = ((char *)self->cursor)[6];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                break;
            }
        }

        if (_strnicmp((char *)self->cursor, "applytoall", 10) == 0) {
            c = ((char *)self->cursor)[10];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                break;
            }
        }

    skip_line_and_continue:
        do {
            line = (char *)self->cursor;
            self->cursor = (uint32_t)(line + 1);
            if (*line == '\r') break;
        } while ((char *)self->cursor < (char *)self->end);
        if ((char *)self->cursor < (char *)self->end && *(char *)self->cursor == '\n') {
            self->cursor = (uint32_t)(line + 2);
        }
        line = (char *)self->cursor;
        self->line_start = self->cursor;
        self->line_number = self->line_number + 1;
        if ((char *)self->end <= line) {
            return 1;
        }
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x57a3e0):

/* WARNING: Removing unreachable block (ram,0x0057a523) */
/* WARNING: Removing unreachable block (ram,0x0057a638) */

uint hwreq_parser_parse_propertyset_directive(void)

{
  char cVar1;
  int iVar2;
  void *_Memory;
  uint uVar3;
  undefined4 *puVar4;
  uint extraout_EAX;
  int in_ECX;
  char *pcVar5;
  char *pcVar6;
  void *local_c;
  undefined1 *puStack_8;
  uint local_4;

  local_4 = 0xffffffff;
  puStack_8 = &LAB_00639563;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  do {
    iVar2 = __strnicmp(*(char **)(in_ECX + 8),"propertyset",0xb);
    if (iVar2 == 0) {
      pcVar5 = (char *)(*(int *)(in_ECX + 8) + 0xb);
      cVar1 = *pcVar5;
      if (((((cVar1 != '>') && (cVar1 != '<')) && (cVar1 != '!')) &&
          ((cVar1 != '=' && (cVar1 != ' ')))) && ((cVar1 != '\r' && (cVar1 != '\t'))))
      goto LAB_0057a545;
      while( true ) {
        *(char **)(in_ECX + 8) = pcVar5;
        if ((*pcVar5 != ' ') && (*pcVar5 != '\t')) break;
        pcVar5 = pcVar5 + 1;
      }
      if (**(char **)(in_ECX + 8) != '=') {
        uVar3 = hwreq_parser_report_error("Missing =");
LAB_0057a664:
        ExceptionList = local_c;
        return uVar3 & 0xffffff00;
      }
      *(char **)(in_ECX + 8) = *(char **)(in_ECX + 8) + 1;
      pcVar5 = (char *)hwreq_token_parse_quoted_string();
      uVar3 = 0;
      if (pcVar5 == (char *)0x0) goto LAB_0057a664;
      pcVar6 = pcVar5;
      do {
        cVar1 = *pcVar6;
        pcVar6 = pcVar6 + 1;
      } while (cVar1 != '\0');
      FUN_0057bc90(pcVar5,(int)pcVar6 - (int)(pcVar5 + 1));
      local_4 = 0;
      do {
        pcVar5 = *(char **)(in_ECX + 8);
        pcVar6 = pcVar5 + 1;
        *(char **)(in_ECX + 8) = pcVar6;
        if (*pcVar5 == '\r') break;
      } while (pcVar6 < *(char **)(in_ECX + 0xc));
      if ((pcVar6 < *(char **)(in_ECX + 0xc)) && (*pcVar6 == '\n')) {
        *(char **)(in_ECX + 8) = pcVar5 + 2;
      }
      *(undefined4 *)(in_ECX + 0x10) = *(undefined4 *)(in_ECX + 8);
      *(int *)(in_ECX + 0x14) = *(int *)(in_ECX + 0x14) + 1;
      _Memory = operator_new(0x14);
      if (_Memory == (void *)0x0) {
        _Memory = (void *)0x0;
      }
      else {
        *(undefined4 *)((int)_Memory + 4) = 0;
        *(undefined4 *)((int)_Memory + 8) = 0;
        *(undefined4 *)((int)_Memory + 0xc) = 0;
        *(int *)((int)_Memory + 0x10) = in_ECX;
      }
      local_4 = local_4 & 0xffffff00;
      uVar3 = hwreq_parser_parse_block(in_ECX,_Memory);
      if ((char)uVar3 == '\0') {
        if (_Memory != (void *)0x0) {
          FUN_0057b990();
          _free(_Memory);
          uVar3 = extraout_EAX;
        }
        ExceptionList = local_c;
        return uVar3 & 0xffffff00;
      }
      puVar4 = (undefined4 *)FUN_0057b6e0(in_ECX + 0x6a0);
      *puVar4 = _Memory;
      local_4 = 0xffffffff;
    }
    else {
LAB_0057a545:
      iVar2 = __strnicmp(*(char **)(in_ECX + 8),"vendor",6);
      if (iVar2 == 0) {
        cVar1 = *(char *)(*(int *)(in_ECX + 8) + 6);
        pcVar5 = (char *)0x0;
        if ((((cVar1 == '>') || (cVar1 == '<')) ||
            ((cVar1 == '!' || (((cVar1 == '=' || (cVar1 == ' ')) || (cVar1 == '\r')))))) ||
           (cVar1 == '\t')) break;
      }
      iVar2 = __strnicmp(*(char **)(in_ECX + 8),"applytoall",10);
      if (iVar2 == 0) {
        cVar1 = *(char *)(*(int *)(in_ECX + 8) + 10);
        pcVar5 = (char *)0x0;
        if ((((cVar1 == '>') || (cVar1 == '<')) ||
            ((cVar1 == '!' || ((cVar1 == '=' || (cVar1 == ' ')))))) ||
           ((cVar1 == '\r' || (cVar1 == '\t')))) break;
      }
    }
    do {
      pcVar5 = *(char **)(in_ECX + 8);
      pcVar6 = pcVar5 + 1;
      *(char **)(in_ECX + 8) = pcVar6;
      if (*pcVar5 == '\r') break;
    } while (pcVar6 < *(char **)(in_ECX + 0xc));
    if ((pcVar6 < *(char **)(in_ECX + 0xc)) && (*pcVar6 == '\n')) {
      *(char **)(in_ECX + 8) = pcVar5 + 2;
    }
    pcVar5 = *(char **)(in_ECX + 8);
    *(char **)(in_ECX + 0x10) = pcVar5;
    *(int *)(in_ECX + 0x14) = *(int *)(in_ECX + 0x14) + 1;
  } while (pcVar5 < *(char **)(in_ECX + 0xc));
  ExceptionList = local_c;
  return CONCAT31((int3)((uint)pcVar5 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
