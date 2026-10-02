// hwreq_parser_parse_block  (Ghidra: hwreq_parser_parse_block, already named)
// address 0x57af10, size 1372 bytes
// name confidence: 0.55  rewrite confidence: 0.75
// evidence: matches its own name and out/phase4/shell_functions.md summary: "The core parser
//   routine for one nested block of the hardware-requirements script: evaluates conditions,
//   processes flag assignments and graphic-detail directives, and handles if/endif/break control
//   flow." Every caller (hwreq_parser_scan_for_applytoall_or_vendor, hwreq_parser_scan_for_
//   applytoall, hwreq_parser_parse_propertyset_directive, hwreq_parser_parse_vendor_block,
//   hwreq_parser_parse_audiovendor_block, hwreq_parser_find_requirements_section) hands it a
//   target property set (this->flags, this->requirements, or a freshly allocated one) and reads
//   this function's boolean result.
// register convention: both parameters (this, target) already recognized correctly by Ghidra as
//   ordinary stack arguments -- objdump confirms every use of param_1/param_2 matches. The
//   keyword scan uses hwreq_token_match_keyword 0x578fa0 (keyword in EDX, this in EDI, does NOT
//   advance the cursor past a match -- every caller here that consumes a matched keyword does so
//   itself); hwreq_d3dcaps_field_resolve 0x578ff0 and hwreq_token_parse_number/_quoted_string
//   take this in EAX; hwreq_token_skip_whitespace takes this in EDX; hwreq_parser_parse_flag_
//   assignment 0x578cf0 is a thiscall (this in ECX, target on the stack);
//   hwreq_device_override_list_find 0x578630 takes the property set to search in EAX and the key
//   C string in EBX, with the output buffer/capacity on the stack; the small MSVC map helpers
//   (FUN_0057b470/0x57b520/0x57b560/0x57b7a0, all library code, none in the function list) are
//   documented individually below, next to their extern declarations.
// blam-cc: this, target (both stack parameters, left as Ghidra found them).
// UNSURE: hwreq_d3dcaps_field_resolve's failure return is reused directly as a report_error
//   message pointer (objdump 0x57b287..0x57b289: any EAX other than 0 or 1 is passed straight to
//   hwreq_parser_report_error), and hwreq_parser_parse_flag_assignment's nonzero return is used
//   the same way (0x578cf0's return, tested at "iVar3 != 0", is passed to report_error); both are
//   below this module's rewrite range so the exact meaning of a nonzero-non-error-pointer isn't
//   pinned down further here, but the observed dispatch is preserved exactly.
// Reconciled against the sibling files already committed for this module's <0x579690 range:
// hwreq_d3dcaps_field_resolve and hwreq_parser_parse_flag_assignment are declared there as
// returning `const char *` directly ((const char*)0/(const char*)1 for false/true, a real string
// otherwise), not `undefined4`/int cast to a pointer as Ghidra's own decompile of this function
// implied; the externs and comparisons here were updated to match that real signature.
// Review fix: a "propertyset = \"name\"" reference merges the named set into the caller's
//   target, not this->flags. objdump 0x57b34c loads [esp+0x490], which with four registers pushed
//   under the 0x46c byte frame is the second stack argument (target), before calling 0x57b470.
//   Only MaxOverallGraphicDetail merges into this->flags (0x57b223 loads [ebp+0x18]). The first
//   rewrite had applied the 0x57b223 reading to both.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void hwreq_parser_report_error(hwreq_parser *self, const char *message); // 0x578a20, blam-cc: this in ESI (live-in), message on the stack; below this module's rewrite range
extern int32_t hwreq_token_parse_number(hwreq_parser *self); // 0x578b20, blam-cc: this in EAX; below this module's rewrite range; -1 on error
extern char *hwreq_token_parse_quoted_string(hwreq_parser *self); // 0x578c60, blam-cc: this in EAX; below this module's rewrite range; NULL on error
extern void hwreq_token_skip_whitespace(hwreq_parser *self); // 0x578a00, blam-cc: this in EDX; below this module's rewrite range
extern uint32_t hwreq_token_match_keyword(const char *keyword, hwreq_parser *self); // 0x578fa0, blam-cc: keyword in EDX, this in EDI; below this module's rewrite range; does not advance the cursor
extern const char *hwreq_d3dcaps_field_resolve(hwreq_parser *self); // 0x578ff0, blam-cc: this in EAX; below this module's rewrite range; (const char*)0 false, (const char*)1 true, else a real error message string (see UNSURE above)
extern const char *hwreq_parser_parse_flag_assignment(hwreq_parser *self, hwreq_property_set *target); // 0x578cf0, blam-cc: this in ECX, target on the stack; below this module's rewrite range; 0 success, else an error message string (see UNSURE above)
extern uint32_t hwreq_device_override_list_find(hwreq_property_set *set, const char *key, char *dest_buffer, uint32_t capacity); // 0x578630, blam-cc: set in EAX, key in EBX, dest_buffer/capacity on the stack; below this module's rewrite range
extern void hwreq_key_string_construct_cstr(msvc_std_string *dest, const char *source); // 0x57b520, blam-cc: dest in ECX, source on the stack; library code (map neighbour), not in the function list; constructs dest fresh from source
extern void hwreq_key_string_destruct(msvc_std_string *key); // 0x57b560, blam-cc: key in ECX (thiscall); library code, not in the function list
extern hwreq_map_node *hwreq_map_find(msvc_std_map *map, msvc_std_string *key); // 0x57b7a0, blam-cc: map in EDI, key in ESI; library code (std::map<string,T*>::find), not in the function list. It also takes an EBX scratch output slot that every caller here only reads back through the returned pointer, so it is folded into an ordinary return value here (see the file header note above); returns the found node, or the map's own head sentinel if the key is absent
extern void hwreq_property_set_apply(hwreq_property_set *source, hwreq_property_set *target); // 0x57b470, blam-cc: source in EAX, target on the stack; library code, not in the function list; merges every (name,value) pair of source->flags into target via hwreq_property_set_upsert (0x578410, below this module's rewrite range)

// Parses one nested block of the hardware-requirements script line by line, starting at the
// parser's current cursor: "if <condition>" / "endif" toggle which lines are active; "break"
// ends the block early; "MaxOverallGraphicDetail = N" merges a graphic-detail-level property set
// into this->flags when N undercuts any existing "OverallGraphicDetail" override;
// "propertyset = \"name\"" merges an already-registered property set into target; every
// other active line is parsed as a "flag = value" assignment into target. A line beginning
// "vendor", "audiovendor" or "Requirements" ends the block immediately (cursor left there,
// returns true) regardless of unclosed ifs; reaching the end of the file or a "break" keyword
// ends it too, but only succeeds if every "if" was matched by an "endif" first.
uint8_t hwreq_parser_parse_block(hwreq_parser *self, hwreq_property_set *target)
{
    int32_t if_state;        // 0 active, 1 active (inside a true if), 2 skipping
    int32_t if_stack[17];    // Ghidra sized this aiStack_410[257]; only indices 0..16 are ever addressed (max nesting 16)
    int32_t if_depth;
    uint8_t is_first_line;
    char *line;
    char *cursor;
    char c;
    char *name;
    const char *resolve_result;
    int32_t parsed_number;
    char number_text[40];
    char override_text[16];
    msvc_std_string key;
    hwreq_map_node *node;
    hwreq_property_set *source_set;
    const char *flag_result;
    int i;

    if_state = 0;
    if_depth = 0;
    is_first_line = 1;

    for (;;) {
        if (is_first_line) {
            is_first_line = 0;
        } else {
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
        }

        if (_strnicmp((char *)self->cursor, "vendor", 6) == 0) {
            c = ((char *)self->cursor)[6];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                return 1;
            }
        }
        if (_strnicmp((char *)self->cursor, "audiovendor", 11) == 0) {
            c = ((char *)self->cursor)[11];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                return 1;
            }
        }
        if (_strnicmp((char *)self->cursor, "Requirements", 12) == 0) {
            c = ((char *)self->cursor)[12];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                return 1;
            }
        }

        cursor = (char *)self->cursor;
        while (*cursor == ' ' || *cursor == '\t') {
            cursor++;
        }
        self->cursor = (uint32_t)cursor;

        c = *cursor;
        if (c == '\r' || *(uint16_t *)cursor == 0x2f2f || (c >= '0' && c <= '9') ||
            (_strnicmp(cursor, "unknown", 7) == 0 &&
             (c = cursor[7], c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t'))) {
            // blank line, "//" comment, a bare number, or "unknown": nothing to do this line
        } else if (hwreq_token_match_keyword("break", self)) {
            break;
        } else if (hwreq_token_match_keyword("MaxOverallGraphicDetail", self)) {
            self->cursor = self->cursor + 23;
            cursor = (char *)self->cursor;
            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
            }
            self->cursor = (uint32_t)cursor;
            if (*cursor != '=') {
                hwreq_parser_report_error(self, "Expecting '=', didn't get it");
                return 0;
            }
            do {
                cursor++;
                self->cursor = (uint32_t)cursor;
            } while (*cursor == ' ' || *cursor == '\t');

            parsed_number = hwreq_token_parse_number(self);
            if (parsed_number == -1) {
                hwreq_parser_report_error(self, "MaxOverallGraphicDetail did not specify a number!");
                return 0;
            }

            override_text[0] = '0';
            override_text[1] = 0;
            for (i = 2; i < 14; i++) {
                override_text[i] = 0;
            }
            sprintf(number_text, "%d", parsed_number);

            if (hwreq_device_override_list_find((hwreq_property_set *)self->flags, "OverallGraphicDetail",
                                                 override_text, 0x10) != 0 &&
                (uint32_t)parsed_number < (uint32_t)atol(override_text)) {
                key.capacity = k_msvc_string_inline_capacity;
                key.size = 0;
                key.buffer.inline_buffer[0] = 0;
                hwreq_key_string_construct_cstr(&key, number_text);

                node = hwreq_map_find(&self->graphic_detail_sets, &key);
                if (node == (hwreq_map_node *)self->graphic_detail_sets.head) {
                    hwreq_parser_report_error(self, "Unrecognized graphic detail");
                    hwreq_key_string_destruct(&key);
                    return 0;
                }
                source_set = (hwreq_property_set *)node->value;
                hwreq_property_set_apply(source_set, (hwreq_property_set *)self->flags);
                hwreq_key_string_destruct(&key);
            }
        } else if (hwreq_token_match_keyword("if", self)) {
            self->cursor = self->cursor + 2;
            resolve_result = hwreq_d3dcaps_field_resolve(self);

            if_stack[if_depth + 1] = if_state;
            if_depth = if_depth + 1;
            if (if_depth == 0x10) {
                hwreq_parser_report_error(self, "IF's nested too deep");
                return 0;
            }

            if (resolve_result == (const char *)1) {
                if (if_state != 2) {
                    if_state = 1;
                }
            } else {
                if (resolve_result != 0) {
                    hwreq_parser_report_error(self, resolve_result);
                    return 0;
                }
                if_state = 2;
            }
        } else if (hwreq_token_match_keyword("endif", self)) {
            if (if_depth == 0) {
                hwreq_parser_report_error(self, "Unexpected ENDIF");
                return 0;
            }
            if_state = if_stack[if_depth];
            if_depth = if_depth - 1;
        } else if (if_state == 0 || if_state == 1) {
            if (hwreq_token_match_keyword("propertyset", self)) {
                self->cursor = self->cursor + 11;
                hwreq_token_skip_whitespace(self);
                if (*(char *)self->cursor != '=') {
                    hwreq_parser_report_error(self, "Missing =");
                    return 0;
                }
                self->cursor = self->cursor + 1;

                name = hwreq_token_parse_quoted_string(self);
                if (name == 0) {
                    return 0;
                }

                key.capacity = k_msvc_string_inline_capacity;
                key.size = 0;
                key.buffer.inline_buffer[0] = 0;
                hwreq_key_string_construct_cstr(&key, name);

                node = hwreq_map_find(&self->property_sets, &key);
                if (node == (hwreq_map_node *)self->property_sets.head) {
                    hwreq_parser_report_error(self, "Unrecognized property set");
                    hwreq_key_string_destruct(&key);
                    return 0;
                }
                source_set = (hwreq_property_set *)node->value;
                hwreq_property_set_apply(source_set, target);
                hwreq_key_string_destruct(&key);
            } else {
                flag_result = hwreq_parser_parse_flag_assignment(self, target);
                if (flag_result != 0) {
                    hwreq_parser_report_error(self, flag_result);
                    return 0;
                }
            }
        }
        // if_state == 2 (skipping a false "if"): nothing to do for this line

        if (!((char *)self->cursor < (char *)self->end)) {
            break;
        }
    }

    if (if_state == 0 && if_depth == 0) {
        return 1;
    }
    hwreq_parser_report_error(self, "Bad IF/ENDIF");
    return 0;
}

#if 0
Original Ghidra decompilation (0x57af10):

uint hwreq_parser_parse_block(int param_1,undefined4 param_2)

{
  short *_Str1;
  bool bVar1;
  char cVar2;
  int iVar3;
  undefined3 uVar8;
  char *pcVar4;
  uint uVar5;
  uint uVar6;
  int *piVar7;
  char *pcVar9;
  int iVar10;
  int local_474;
  int local_470;
  char local_46c [52];
  char local_438 [40];
  int aiStack_410 [257];
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;

  local_4 = 0xffffffff;
  puStack_8 = &LAB_00639546;
  local_c = ExceptionList;
  iVar10 = 0;
  local_470 = 0;
  local_474 = 0;
  bVar1 = true;
  ExceptionList = &local_c;
  do {
    if (bVar1) {
      bVar1 = false;
    }
    else {
      do {
        pcVar4 = *(char **)(param_1 + 8);
        pcVar9 = pcVar4 + 1;
        *(char **)(param_1 + 8) = pcVar9;
        if (*pcVar4 == '\r') break;
      } while (pcVar9 < *(char **)(param_1 + 0xc));
      if ((pcVar9 < *(char **)(param_1 + 0xc)) && (*pcVar9 == '\n')) {
        *(char **)(param_1 + 8) = pcVar4 + 2;
      }
      *(undefined4 *)(param_1 + 0x10) = *(undefined4 *)(param_1 + 8);
      *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    }
    iVar3 = __strnicmp(*(char **)(param_1 + 8),"vendor",6);
    if (iVar3 == 0) {
      cVar2 = *(char *)(*(int *)(param_1 + 8) + 6);
      uVar8 = 0;
      if ((((((cVar2 == '>') || (cVar2 == '<')) || (cVar2 == '!')) ||
           ((cVar2 == '=' || (cVar2 == ' ')))) || (cVar2 == '\r')) || (cVar2 == '\t'))
      goto LAB_0057b44f;
    }
    iVar3 = __strnicmp(*(char **)(param_1 + 8),"audiovendor",0xb);
    if (iVar3 == 0) {
      cVar2 = *(char *)(*(int *)(param_1 + 8) + 0xb);
      uVar8 = 0;
      if ((((cVar2 == '>') || (cVar2 == '<')) ||
          ((cVar2 == '!' || (((cVar2 == '=' || (cVar2 == ' ')) || (cVar2 == '\r')))))) ||
         (cVar2 == '\t')) goto LAB_0057b44f;
    }
    iVar3 = __strnicmp(*(char **)(param_1 + 8),"Requirements",0xc);
    if (iVar3 == 0) {
      cVar2 = *(char *)(*(int *)(param_1 + 8) + 0xc);
      uVar8 = (undefined3)((uint)*(int *)(param_1 + 8) >> 8);
      if (((cVar2 == '>') || (cVar2 == '<')) ||
         ((((cVar2 == '!' || ((cVar2 == '=' || (cVar2 == ' ')))) || (cVar2 == '\r')) ||
          (cVar2 == '\t')))) goto LAB_0057b44f;
    }
    while( true ) {
      cVar2 = **(char **)(param_1 + 8);
      if ((cVar2 != ' ') && (cVar2 != '\t')) break;
      *(char **)(param_1 + 8) = *(char **)(param_1 + 8) + 1;
    }
    _Str1 = *(short **)(param_1 + 8);
    cVar2 = (char)*_Str1;
    if (((cVar2 != '\r') && (*_Str1 != 0x2f2f)) &&
       (((cVar2 < '0' || ('9' < cVar2)) &&
        ((iVar3 = __strnicmp((char *)_Str1,"unknown",7), iVar3 != 0 ||
         ((((cVar2 = *(char *)(*(int *)(param_1 + 8) + 7), cVar2 != '>' && (cVar2 != '<')) &&
           (cVar2 != '!')) &&
          (((cVar2 != '=' && (cVar2 != ' ')) && ((cVar2 != '\r' && (cVar2 != '\t')))))))))))) {
      cVar2 = hwreq_token_match_keyword();
      if (cVar2 != '\0') break;
      cVar2 = hwreq_token_match_keyword();
      if (cVar2 == '\0') {
        cVar2 = hwreq_token_match_keyword();
        if (cVar2 == '\0') {
          cVar2 = hwreq_token_match_keyword();
          if (cVar2 == '\0') {
            if ((iVar10 == 0) || (iVar10 == 1)) {
              cVar2 = hwreq_token_match_keyword();
              if (cVar2 == '\0') {
                iVar3 = hwreq_parser_parse_flag_assignment(param_2);
                if (iVar3 != 0) goto LAB_0057b443;
              }
              else {
                *(int *)(param_1 + 8) = *(int *)(param_1 + 8) + 0xb;
                hwreq_token_skip_whitespace();
                if (**(char **)(param_1 + 8) != '=') {
                  uVar5 = hwreq_parser_report_error("Missing =");
                  ExceptionList = local_c;
                  return uVar5 & 0xffffff00;
                }
                *(char **)(param_1 + 8) = *(char **)(param_1 + 8) + 1;
                iVar10 = hwreq_token_parse_quoted_string();
                uVar5 = 0;
                if (iVar10 == 0) goto LAB_0057b3b5;
                FUN_0057b520(iVar10);
                local_4 = 1;
                piVar7 = (int *)FUN_0057b7a0();
                if (*piVar7 == *(int *)(param_1 + 0x6a4)) {
                  hwreq_parser_report_error("Unrecognized property set");
                  local_4 = 0xffffffff;
                  uVar5 = FUN_0057b560();
                  ExceptionList = local_c;
                  return uVar5 & 0xffffff00;
                }
                FUN_0057b470(param_2);
                local_4 = 0xffffffff;
                FUN_0057b560();
                iVar10 = local_474;
              }
            }
          }
          else {
            if (local_470 == 0) {
              uVar5 = hwreq_parser_report_error("Unexpected ENDIF");
              ExceptionList = local_c;
              return uVar5 & 0xffffff00;
            }
            local_474 = aiStack_410[local_470];
            local_470 = local_470 + -1;
            iVar10 = local_474;
          }
        }
        else {
          *(int *)(param_1 + 8) = *(int *)(param_1 + 8) + 2;
          iVar3 = hwreq_d3dcaps_field_resolve();
          aiStack_410[local_470 + 1] = iVar10;
          local_470 = local_470 + 1;
          if (local_470 == 0x10) {
            uVar5 = hwreq_parser_report_error("IF\'s nested too deep");
            ExceptionList = local_c;
            return uVar5 & 0xffffff00;
          }
          if (iVar3 == 1) {
            if (iVar10 != 2) {
              local_474 = 1;
              iVar10 = local_474;
            }
          }
          else {
            if (iVar3 != 0) {
LAB_0057b443:
              uVar5 = hwreq_parser_report_error(iVar3);
              ExceptionList = local_c;
              return uVar5 & 0xffffff00;
            }
            local_474 = 2;
            iVar10 = 2;
          }
        }
      }
      else {
        *(int *)(param_1 + 8) = *(int *)(param_1 + 8) + 0x17;
        while( true ) {
          cVar2 = **(char **)(param_1 + 8);
          if ((cVar2 != ' ') && (cVar2 != '\t')) break;
          *(char **)(param_1 + 8) = *(char **)(param_1 + 8) + 1;
        }
        pcVar4 = *(char **)(param_1 + 8);
        if (*pcVar4 != '=') {
          uVar5 = hwreq_parser_report_error("Expecting \'=\', didn\'t get it");
          ExceptionList = local_c;
          return uVar5 & 0xffffff00;
        }
        do {
          do {
            pcVar4 = pcVar4 + 1;
            *(char **)(param_1 + 8) = pcVar4;
          } while (*pcVar4 == ' ');
        } while (*pcVar4 == '\t');
        uVar5 = hwreq_token_parse_number();
        if (uVar5 == 0xffffffff) {
          uVar5 = hwreq_parser_report_error("MaxOverallGraphicDetail did not specify a number!");
          ExceptionList = local_c;
          return uVar5 & 0xffffff00;
        }
        local_46c[2] = '\0';
        local_46c[3] = '\0';
        local_46c[4] = '\0';
        local_46c[5] = '\0';
        local_46c[6] = '\0';
        local_46c[7] = '\0';
        local_46c[8] = '\0';
        local_46c[9] = '\0';
        local_46c[10] = '\0';
        local_46c[0xb] = '\0';
        local_46c[0xc] = '\0';
        local_46c[0xd] = '\0';
        local_46c[0] = '0';
        local_46c[1] = '\0';
        local_46c[0xe] = '\0';
        local_46c[0xf] = '\0';
        _sprintf(local_438,"%d",uVar5);
        cVar2 = hwreq_device_override_list_find(local_46c,0x10);
        if ((cVar2 != '\0') && (uVar6 = _atol(local_46c), uVar5 < uVar6)) {
          FUN_0057b520(local_438);
          local_4 = 0;
          piVar7 = (int *)FUN_0057b7a0();
          iVar10 = *piVar7;
          local_4 = 0xffffffff;
          FUN_0057b560();
          if (iVar10 == *(int *)(param_1 + 0x6b0)) {
            uVar5 = hwreq_parser_report_error("Unrecognized graphic detail");
            ExceptionList = local_c;
            return uVar5 & 0xffffff00;
          }
          FUN_0057b470(*(undefined4 *)(param_1 + 0x18));
          iVar10 = local_474;
        }
      }
    }
  } while (*(uint *)(param_1 + 8) < *(uint *)(param_1 + 0xc));
  if ((iVar10 == 0) && (uVar8 = 0, local_470 == 0)) {
LAB_0057b44f:
    uVar5 = CONCAT31(uVar8,1);
  }
  else {
    uVar5 = hwreq_parser_report_error("Bad IF/ENDIF");
LAB_0057b3b5:
    uVar5 = uVar5 & 0xffffff00;
  }
  ExceptionList = local_c;
  return uVar5;
}
#endif
