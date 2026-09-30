// hwreq_parser_parse_flag_assignment  (Ghidra: hwreq_parser_parse_flag_assignment, already named)
// address 0x578cf0, size 498 bytes
// name confidence: 0.65   rewrite confidence: 0.7
// evidence: lower-cases the flag name into a k_hwreq_flag_name_length buffer, parses either a
// quoted or bare value into a second buffer, upserts (name, value) into the given property set
// (FUN_00578410, this module) and, for "OverallGraphicDetail", also records the set under
// graphic_detail_sets (vtable/+0x6ac) keyed by that value.
// Review: re-checked against objdump 0x578cf0..0x578ee5. The quoted-value behaviour is real: the
//   opening quote is stored (0x578db1 stores cl == '"' on the first pass), the closing quote is
//   stored (0x578ddd) and the cursor is left on the closing quote. The OverallGraphicDetail tail
//   builds a local std::string from the value (0x57bc90), stores the property set through
//   map::operator[] 0x57b6e0 (key in EDI, map on the stack) and frees the string's heap buffer
//   (0x578eb5..0x578ec2); the first rewrite used a private string helper and leaked it.
// register convention: parser in ECX (mov esi,ecx at 0x578d14), property_set on the stack, ret 4.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"


extern void msvc_string_assign_n(msvc_std_string *dest, const char *source, uint32_t length); // 0x57bc90, blam-cc: dest in ECX, source/length on the stack; library code, not in the function list


// Parses one 'flag = value' assignment line from the requirements script, storing it in the
// flag table and, for the special OverallGraphicDetail flag, also recording its value for the
// caller. Returns NULL on success, or a static error message string.
const char *hwreq_parser_parse_flag_assignment(hwreq_parser *parser, hwreq_property_set *property_set)
{
    char name[256];
    char value[256];
    char *name_out = name;
    char *value_out;
    char *src;
    char c;
    char *p2;

    c = *(char *)parser->cursor;
    while (c != ' ') {
        c = *(char *)parser->cursor;
        if (c == '=' || c == '\r') break;
        parser->cursor++;
        if (c > '@' && c < '[') c += ' '; // lowercase
        *name_out = c;
        name_out++;
        if (name_out == name + 254) {
            return "Flag too long";
        }
        c = *(char *)parser->cursor;
    }
    *name_out = '\0';

    while (*(char *)parser->cursor == ' ' || *(char *)parser->cursor == '\t') {
        parser->cursor++;
    }

    src = (char *)parser->cursor;
    value_out = value;

    if (*src != '\r') {
        if (*src != '=') {
            return "flag = xxx expected";
        }
        do {
            do {
                src++;
                parser->cursor = (uint32_t)src;
                c = *src;
            } while (c == ' ');
        } while (c == '\t');

        if (c == '"') {
            c = '"';
            for (;;) {
                src++;
                *value_out = c;
                p2 = value_out + 1;
                parser->cursor = (uint32_t)src;
                if (p2 == value + 0xfe) {
                    return "Flag too long";
                }
                c = *src;
                if (c == '"') {
                    p2[0] = '"';
                    value_out += 2;
                    if (value_out != value + 0xfe) {
                        goto have_value;
                    }
                    return "Flag too long";
                }
                value_out = p2;
                if (c == '\r') {
                    return "Missing Quote";
                }
            }
        }

        while (c != ' ' && (c = *src, c != '\r')) {
            src++;
            parser->cursor = (uint32_t)src;
            if (c > '@' && c < '[') c += ' ';
            *value_out = c;
            value_out++;
            if (value_out == value + 0xfe) {
                return "Flag too long";
            }
            c = *(char *)parser->cursor;
        }
    }

have_value:
    *value_out = '\0';

    hwreq_property_set_upsert(property_set, name, value);

    if (_stricmp("OverallGraphicDetail", name) == 0) {
        msvc_std_string value_string;

        value_string.size = 0;
        value_string.buffer.inline_buffer[0] = 0;
        value_string.capacity = k_msvc_string_inline_capacity;
        msvc_string_assign_n(&value_string, value, (uint32_t)(value_out - value));
        *hwreq_property_set_map_index(&value_string, &parser->graphic_detail_sets) = property_set;
        if (value_string.capacity >= 0x10) {
            free((void *)value_string.buffer.heap_buffer);
        }
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x578cf0):


/* WARNING: Removing unreachable block (ram,0x00578ebd) */

char * hwreq_parser_parse_flag_assignment(undefined4 param_1)

{
  void *pvVar1;
  char cVar2;
  char *pcVar3;
  int iVar4;
  undefined4 *puVar5;
  int in_ECX;
  char *pcVar6;
  char *pcVar7;
  char local_20c [256];
  char local_10c [254];
  char local_e [2];
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  pvVar1 = ExceptionList;
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0063951b;
  local_c = ExceptionList;
  pcVar6 = local_10c;
  cVar2 = **(char **)(in_ECX + 8);
  ExceptionList = &local_c;
  while (cVar2 != ' ') {
    cVar2 = **(char **)(in_ECX + 8);
    if ((cVar2 == '=') || (cVar2 == '\r')) break;
    *(char **)(in_ECX + 8) = *(char **)(in_ECX + 8) + 1;
    if (('@' < cVar2) && (cVar2 < '[')) {
      cVar2 = cVar2 + ' ';
    }
    *pcVar6 = cVar2;
    pcVar6 = pcVar6 + 1;
    if (pcVar6 == local_e) goto LAB_00578dec;
    cVar2 = **(char **)(in_ECX + 8);
  }
  *pcVar6 = '\0';
  while( true ) {
    cVar2 = **(char **)(in_ECX + 8);
    if ((cVar2 != ' ') && (cVar2 != '\t')) break;
    *(char **)(in_ECX + 8) = *(char **)(in_ECX + 8) + 1;
  }
  pcVar3 = *(char **)(in_ECX + 8);
  pcVar6 = local_20c;
  if (*pcVar3 != '\r') {
    if (*pcVar3 != '=') {
      ExceptionList = pvVar1;
      return "flag = xxx expected";
    }
    do {
      do {
        pcVar3 = pcVar3 + 1;
        *(char **)(in_ECX + 8) = pcVar3;
        cVar2 = *pcVar3;
      } while (cVar2 == ' ');
    } while (cVar2 == '\t');
    if (cVar2 == '\"') {
      cVar2 = '\"';
      while( true ) {
        pcVar3 = pcVar3 + 1;
        *pcVar6 = cVar2;
        pcVar7 = pcVar6 + 1;
        *(char **)(in_ECX + 8) = pcVar3;
        if (pcVar7 == local_20c + 0xfe) break;
        cVar2 = *pcVar3;
        if (cVar2 == '\"') {
          *pcVar7 = '\"';
          pcVar6 = pcVar6 + 2;
          if (pcVar6 != local_20c + 0xfe) goto LAB_00578e2e;
          break;
        }
        pcVar6 = pcVar7;
        if (cVar2 == '\r') {
          ExceptionList = pvVar1;
          return "Missing Quote";
        }
      }
LAB_00578dec:
      ExceptionList = pvVar1;
      return "Flag too long";
    }
    while ((cVar2 != ' ' && (cVar2 = *pcVar3, cVar2 != '\r'))) {
      pcVar3 = pcVar3 + 1;
      *(char **)(in_ECX + 8) = pcVar3;
      if (('@' < cVar2) && (cVar2 < '[')) {
        cVar2 = cVar2 + ' ';
      }
      *pcVar6 = cVar2;
      pcVar6 = pcVar6 + 1;
      if (pcVar6 == local_20c + 0xfe) goto LAB_00578dec;
      cVar2 = **(char **)(in_ECX + 8);
    }
  }
LAB_00578e2e:
  *pcVar6 = '\0';
  FUN_00578410(param_1,local_10c,local_20c);
  iVar4 = __stricmp("OverallGraphicDetail",local_10c);
  if (iVar4 == 0) {
    pcVar6 = local_20c;
    do {
      cVar2 = *pcVar6;
      pcVar6 = pcVar6 + 1;
    } while (cVar2 != '\0');
    FUN_0057bc90(local_20c,(int)pcVar6 - (int)(local_20c + 1));
    local_4 = 0;
    puVar5 = (undefined4 *)FUN_0057b6e0(in_ECX + 0x6ac);
    *puVar5 = param_1;
  }
  ExceptionList = local_c;
  return (char *)0x0;
}
#endif
