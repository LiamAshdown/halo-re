// hwreq_parser_evaluate_condition  (Ghidra: gamespy_get_client_key_hash; renamed per the module
// notes: "this is the condition evaluator... It has nothing to do with GameSpy.")
// address 0x579690, size 1261 bytes
// name confidence: 0.1 (Ghidra) / 0.55 (this name)   rewrite confidence: 0.75
// evidence: parses a two-character or one-character comparison operator (verified against
// objdump: the packed 16-bit compares at 0x5796a9..0x5796df spell out "==","!=","<>","=>","=<",
// "<=",">=" byte for byte, matching hwreq_operator's own comments exactly), then evaluates it
// against a GUID (kind 1), a driver version (kind 2), or a plain value/OS enum (kind 0 / 3).
// register convention: this is the second half of the one physical routine split at this address
// (see hwreq_d3dcaps_field_resolve's header); parser, kind and value are what that function held
// in EDI/EBX/ESI when execution fell through to here, now passed as explicit parameters.
// Review: every operator, jump table (0x579b90 driver, 0x579ba8 value / os) and error string was
//   re-checked against objdump 0x579690..0x579b8c.
// UNSURE: the GUID byte-array comparison (bVar13/bVar14 in Ghidra) only ever feeds an == 0 / != 0
// test here, so it is rewritten as a plain 16-byte equality loop rather than reproducing the
// lexicographic less-than tracking, which is never observed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t hwreq_token_parse_number(hwreq_parser *parser); // 0x00578b20
extern uint32_t hwreq_token_parse_hex_id(hwreq_parser *parser); // 0x00578ef0
extern int32_t hwreq_token_parse_hex_id_byteswap(hwreq_parser *parser); // 0x00578f80
extern uint32_t hwreq_token_match_keyword(const char *keyword, hwreq_parser *parser); // 0x00578fa0

// Parses one comparison-operator term of a requirements-script condition (against a driver GUID,
// driver version number, or OS name / plain value) and evaluates it, reporting a parser error
// for malformed operators or operands.
const char *hwreq_parser_evaluate_condition(hwreq_parser *parser, int32_t kind, uint32_t value)
{
    uint16_t op_word;
    int32_t op;
    char c;

    // Review fix: the routine starts at 0x579690 with a space / tab skip before the operator
    // (0x579690..0x5796a1); the first rewrite began at the operator compare and so rejected
    // "cpuspeed >= 700".
    while (*(char *)parser->cursor == ' ' || *(char *)parser->cursor == '\t') {
        parser->cursor++;
    }

    op_word = *(uint16_t *)parser->cursor;
    switch (op_word) {
    case 0x3d3d: /* "==" */ parser->cursor += 2; op = k_hwreq_operator_equal; break;
    case 0x3d21: /* "!=" */ parser->cursor += 2; op = k_hwreq_operator_not_equal; break;
    case 0x3e3c: /* "<>" */ parser->cursor += 2; op = k_hwreq_operator_not_equal; break;
    case 0x3e3d: /* "=>" */ parser->cursor += 2; op = k_hwreq_operator_greater_equal; break;
    case 0x3c3d: /* "=<" */ parser->cursor += 2; op = k_hwreq_operator_less_equal; break;
    case 0x3d3c: /* "<=" */ parser->cursor += 2; op = k_hwreq_operator_less_equal; break;
    case 0x3d3e: /* ">=" */ parser->cursor += 2; op = k_hwreq_operator_greater_equal; break;
    default:
        c = *(char *)parser->cursor;
        if (c == '=') { parser->cursor += 1; op = k_hwreq_operator_equal; }
        else if (c == '>') { parser->cursor += 1; op = k_hwreq_operator_greater; }
        else if (c == '<') { parser->cursor += 1; op = k_hwreq_operator_less; }
        else if (c == '&') { parser->cursor += 1; op = k_hwreq_operator_and; }
        else return "Unknown operator";
        break;
    }

    while (*(char *)parser->cursor == ' ' || *(char *)parser->cursor == '\t') {
        parser->cursor++;
    }

    if (kind == k_hwreq_condition_guid) {
        uint32_t parsed_guid[4];
        int32_t a, b;
        const uint8_t *lhs;
        const uint8_t *rhs;
        int32_t i;
        int32_t equal;

        if (op > k_hwreq_operator_not_equal) {
            return "Only == or != allowed";
        }

        a = hwreq_token_parse_hex_id(parser);
        if (a == -1) return "Invalid GUID";
        b = hwreq_token_parse_hex_id(parser);
        if (b == -1) return "Invalid GUID";
        parsed_guid[0] = (uint32_t)(a * 0x10000 + b);
        c = *(char *)parser->cursor; parser->cursor++;
        if (c != '-') return "Invalid GUID";

        a = hwreq_token_parse_hex_id(parser);
        if (a == -1) return "Invalid GUID";
        c = *(char *)parser->cursor; parser->cursor++;
        if (c != '-') return "Invalid GUID";
        b = hwreq_token_parse_hex_id(parser);
        if (b == -1) return "Invalid GUID";
        parsed_guid[1] = (uint32_t)(b * 0x10000 + a);
        c = *(char *)parser->cursor; parser->cursor++;
        if (c != '-') return "Invalid GUID";

        a = hwreq_token_parse_hex_id_byteswap(parser);
        if (a == -1) return "Invalid GUID";
        c = *(char *)parser->cursor; parser->cursor++;
        if (c != '-') return "Invalid GUID";
        b = hwreq_token_parse_hex_id_byteswap(parser);
        if (b == -1) return "Invalid GUID";
        parsed_guid[2] = (uint32_t)(b * 0x10000 + a);

        a = hwreq_token_parse_hex_id_byteswap(parser);
        if (a == -1) return "Invalid GUID";
        b = hwreq_token_parse_hex_id_byteswap(parser);
        if (b == -1) return "Invalid GUID";
        parsed_guid[3] = (uint32_t)(b * 0x10000 + a);

        lhs = (const uint8_t *)parsed_guid;
        rhs = (const uint8_t *)parser->adapter.device_identifier;
        equal = 1;
        for (i = 0; i < 16; i++) {
            if (lhs[i] != rhs[i]) { equal = 0; break; }
        }

        return (op == k_hwreq_operator_equal) ? (const char *)(uint32_t)equal
                                               : (const char *)(uint32_t)(!equal);
    }

    if (kind == k_hwreq_condition_driver) {
        int32_t n0, n1, n2, n3;
        int32_t parsed_high, parsed_low;
        uint32_t actual_low;
        int32_t actual_high;

        n0 = hwreq_token_parse_number(parser);
        c = *(char *)parser->cursor; parser->cursor++;
        if (n0 == -1 || c != '.') return "Invalid driver number";

        n1 = hwreq_token_parse_number(parser);
        c = *(char *)parser->cursor; parser->cursor++;
        if (c != '.') return "Invalid driver number";
        parsed_high = n1 + n0 * 0x10000;

        n2 = hwreq_token_parse_number(parser);
        c = *(char *)parser->cursor; parser->cursor++;
        if (n2 == -1 || c != '.') return "Invalid driver number";

        n3 = hwreq_token_parse_number(parser);
        parsed_low = n3 + n2 * 0x10000;

        actual_low = parser->adapter.driver_version.parts.low_part;
        actual_high = parser->adapter.driver_version.parts.high_part;

        switch (op) {
        case k_hwreq_operator_equal:
            if (actual_low == (uint32_t)parsed_low && actual_high == parsed_high) return (const char *)1;
            break;
        case k_hwreq_operator_not_equal:
            if (actual_low != (uint32_t)parsed_low || actual_high != parsed_high) return (const char *)1;
            break;
        case k_hwreq_operator_greater:
            if (parsed_high <= actual_high && (parsed_high < actual_high || (uint32_t)parsed_low < actual_low)) return (const char *)1;
            break;
        case k_hwreq_operator_less:
            if (actual_high <= parsed_high && (actual_high < parsed_high || actual_low < (uint32_t)parsed_low)) return (const char *)1;
            break;
        case k_hwreq_operator_greater_equal:
            if (parsed_high <= actual_high && (parsed_high < actual_high || (uint32_t)parsed_low <= actual_low)) return (const char *)1;
            break;
        case k_hwreq_operator_less_equal:
            if (actual_high <= parsed_high) {
                if (actual_high < parsed_high) return (const char *)1;
                if (actual_low <= (uint32_t)parsed_low) return (const char *)1;
            }
            break;
        default:
            return "Invalid";
        }
        return (const char *)0;
    }

    {
        // kind == k_hwreq_condition_value (plain field) or k_hwreq_condition_os: both
        // compare `value` (already resolved by the caller) against a parsed right-hand side.
        uint32_t rhs;

        if (kind == k_hwreq_condition_os) {
            if (hwreq_token_match_keyword("win95", parser)) { rhs = k_hwreq_os_win95; parser->cursor += 5; }
            else if (hwreq_token_match_keyword("win98", parser)) { rhs = k_hwreq_os_win98; parser->cursor += 5; }
            else if (hwreq_token_match_keyword("win98se", parser)) { rhs = k_hwreq_os_win98se; parser->cursor += 7; }
            else if (hwreq_token_match_keyword("winme", parser)) { rhs = k_hwreq_os_winme; parser->cursor += 5; }
            else if (hwreq_token_match_keyword("win2k", parser)) { rhs = k_hwreq_os_win2k; parser->cursor += 5; }
            else if (hwreq_token_match_keyword("winxp", parser)) { rhs = k_hwreq_os_winxp; parser->cursor += 5; }
            else return "Unknown OS";
        } else {
            int32_t n = hwreq_token_parse_number(parser);
            if (n == -1) return "Number expected";
            rhs = (uint32_t)n;
        }

        switch (op) {
        case k_hwreq_operator_equal:         return (const char *)(uint32_t)(value == rhs);
        case k_hwreq_operator_not_equal:      return (const char *)(uint32_t)(value != rhs);
        case k_hwreq_operator_greater:        return (const char *)(uint32_t)(rhs < value);
        case k_hwreq_operator_less:           return (const char *)(uint32_t)(value < rhs);
        case k_hwreq_operator_greater_equal:  return (const char *)(uint32_t)(rhs <= value);
        case k_hwreq_operator_less_equal:     return (const char *)(uint32_t)(value <= rhs);
        case k_hwreq_operator_and:            return (const char *)(uint32_t)((rhs & value) != 0);
        default:                              return "Invalid";
        }
    }
}

#if 0
Original Ghidra decompilation (0x579690):


char * gamespy_get_client_key_hash(void)

{
  short sVar1;
  int iVar2;
  char cVar3;
  short *psVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  uint uVar9;
  int unaff_EBX;
  uint uVar10;
  uint unaff_ESI;
  byte *pbVar11;
  int unaff_EDI;
  byte *pbVar12;
  bool bVar13;
  bool bVar14;
  int in_stack_00000010;
  int in_stack_00000014;
  int in_stack_00000018;
  int in_stack_0000001c;
  
  while( true ) {
    cVar3 = **(char **)(unaff_EDI + 8);
    if ((cVar3 != ' ') && (cVar3 != '\t')) break;
    *(char **)(unaff_EDI + 8) = *(char **)(unaff_EDI + 8) + 1;
  }
  psVar4 = *(short **)(unaff_EDI + 8);
  sVar1 = *psVar4;
  if (sVar1 == 0x3d3d) {
    psVar4 = psVar4 + 1;
    uVar10 = 0;
  }
  else if (sVar1 == 0x3d21) {
    psVar4 = psVar4 + 1;
    uVar10 = 1;
  }
  else if (sVar1 == 0x3e3c) {
    psVar4 = psVar4 + 1;
    uVar10 = 1;
  }
  else if (sVar1 == 0x3e3d) {
    psVar4 = psVar4 + 1;
    uVar10 = 4;
  }
  else if (sVar1 == 0x3c3d) {
    psVar4 = psVar4 + 1;
    uVar10 = 5;
  }
  else if (sVar1 == 0x3d3c) {
    psVar4 = psVar4 + 1;
    uVar10 = 5;
  }
  else if (sVar1 == 0x3d3e) {
    psVar4 = psVar4 + 1;
    uVar10 = 4;
  }
  else {
    cVar3 = (char)*psVar4;
    if (cVar3 == '=') {
      psVar4 = (short *)((int)psVar4 + 1);
      uVar10 = 0;
    }
    else {
      if (cVar3 == '>') {
        uVar10 = 2;
      }
      else if (cVar3 == '<') {
        uVar10 = 3;
      }
      else {
        if (cVar3 != '&') {
          return "Unknown operator";
        }
        uVar10 = 6;
      }
      psVar4 = (short *)((int)psVar4 + 1);
    }
  }
  *(short **)(unaff_EDI + 8) = psVar4;
  while( true ) {
    cVar3 = **(char **)(unaff_EDI + 8);
    if ((cVar3 != ' ') && (cVar3 != '\t')) break;
    *(char **)(unaff_EDI + 8) = *(char **)(unaff_EDI + 8) + 1;
  }
  if (unaff_EBX == 1) {
    if (1 < uVar10) {
      return "Only == or != allowed";
    }
    iVar5 = hwreq_token_parse_hex_id();
    if ((iVar5 != -1) && (in_stack_00000010 = hwreq_token_parse_hex_id(), in_stack_00000010 != -1))
    {
      in_stack_00000010 = iVar5 * 0x10000 + in_stack_00000010;
      cVar3 = **(char **)(unaff_EDI + 8);
      *(char **)(unaff_EDI + 8) = *(char **)(unaff_EDI + 8) + 1;
      if ((cVar3 == '-') &&
         (((iVar5 = hwreq_token_parse_hex_id(), iVar5 != -1 &&
           (cVar3 = **(char **)(unaff_EDI + 8),
           *(char **)(unaff_EDI + 8) = *(char **)(unaff_EDI + 8) + 1, cVar3 == '-')) &&
          (iVar6 = hwreq_token_parse_hex_id(), iVar6 != -1)))) {
        in_stack_00000014 = iVar6 * 0x10000 + iVar5;
        cVar3 = **(char **)(unaff_EDI + 8);
        *(char **)(unaff_EDI + 8) = *(char **)(unaff_EDI + 8) + 1;
        if ((((cVar3 == '-') && (iVar5 = hwreq_token_parse_hex_id_byteswap(), iVar5 != -1)) &&
            (cVar3 = **(char **)(unaff_EDI + 8),
            *(char **)(unaff_EDI + 8) = *(char **)(unaff_EDI + 8) + 1, cVar3 == '-')) &&
           (iVar6 = hwreq_token_parse_hex_id_byteswap(), iVar6 != -1)) {
          in_stack_00000018 = iVar6 * 0x10000 + iVar5;
          iVar5 = hwreq_token_parse_hex_id_byteswap();
          if ((iVar5 != -1) && (iVar6 = hwreq_token_parse_hex_id_byteswap(), iVar6 != -1)) {
            in_stack_0000001c = iVar6 * 0x10000 + iVar5;
            iVar5 = 0x10;
            if (uVar10 != 0) {
              bVar13 = false;
              iVar6 = 0;
              bVar14 = true;
              pbVar11 = (byte *)&stack0x00000010;
              pbVar12 = (byte *)(unaff_EDI + 0x4f4);
              do {
                if (iVar5 == 0) break;
                iVar5 = iVar5 + -1;
                bVar13 = *pbVar11 < *pbVar12;
                bVar14 = *pbVar11 == *pbVar12;
                pbVar11 = pbVar11 + 1;
                pbVar12 = pbVar12 + 1;
              } while (bVar14);
              if (!bVar14) {
                iVar6 = (1 - (uint)bVar13) - (uint)(bVar13 != 0);
              }
              return (char *)(uint)(iVar6 != 0);
            }
            bVar13 = false;
            iVar6 = 0;
            bVar14 = true;
            pbVar11 = (byte *)&stack0x00000010;
            pbVar12 = (byte *)(unaff_EDI + 0x4f4);
            do {
              if (iVar5 == 0) break;
              iVar5 = iVar5 + -1;
              bVar13 = *pbVar11 < *pbVar12;
              bVar14 = *pbVar11 == *pbVar12;
              pbVar11 = pbVar11 + 1;
              pbVar12 = pbVar12 + 1;
            } while (bVar14);
            if (!bVar14) {
              iVar6 = (1 - (uint)bVar13) - (uint)(bVar13 != 0);
            }
            return (char *)(uint)(iVar6 == 0);
          }
        }
      }
    }
    return "Invalid GUID";
  }
  if (unaff_EBX == 2) {
    iVar5 = hwreq_token_parse_number();
    if ((iVar5 == -1) ||
       (cVar3 = **(char **)(unaff_EDI + 8),
       *(char **)(unaff_EDI + 8) = *(char **)(unaff_EDI + 8) + 1, cVar3 != '.')) {
LAB_005799f7:
      return "Invalid driver number";
    }
    iVar6 = hwreq_token_parse_number();
    cVar3 = **(char **)(unaff_EDI + 8);
    iVar6 = iVar6 + iVar5 * 0x10000;
    *(char **)(unaff_EDI + 8) = *(char **)(unaff_EDI + 8) + 1;
    if ((cVar3 != '.') ||
       ((iVar5 = hwreq_token_parse_number(), iVar5 == -1 ||
        (cVar3 = **(char **)(unaff_EDI + 8),
        *(char **)(unaff_EDI + 8) = *(char **)(unaff_EDI + 8) + 1, cVar3 != '.'))))
    goto LAB_005799f7;
    iVar7 = hwreq_token_parse_number();
    uVar9 = *(uint *)(unaff_EDI + 0x4dc);
    iVar2 = *(int *)(unaff_EDI + 0x4e0);
    uVar8 = iVar7 + iVar5 * 0x10000;
    switch(uVar10) {
    case 0:
      if ((uVar9 == uVar8) && (iVar2 == iVar6)) {
        return (char *)0x1;
      }
      break;
    case 1:
      if ((uVar9 != uVar8) || (iVar2 != iVar6)) {
        return (char *)0x1;
      }
      break;
    case 2:
      if ((iVar6 <= iVar2) && ((iVar6 < iVar2 || (uVar8 < uVar9)))) {
        return (char *)0x1;
      }
      break;
    case 3:
      if ((iVar2 <= iVar6) && ((iVar2 < iVar6 || (uVar9 < uVar8)))) {
        return (char *)0x1;
      }
      break;
    case 4:
      if ((iVar6 <= iVar2) && ((iVar6 < iVar2 || (uVar8 <= uVar9)))) {
        return (char *)0x1;
      }
      break;
    case 5:
      if (iVar2 <= iVar6) {
        if (iVar2 < iVar6) {
          return (char *)0x1;
        }
        if (uVar9 <= uVar8) {
          return (char *)0x1;
        }
      }
      break;
    default:
      goto switchD_00579953_default;
    }
    return (char *)0x0;
  }
  if (unaff_EBX == 3) {
    cVar3 = hwreq_token_match_keyword();
    if (cVar3 == '\0') {
      cVar3 = hwreq_token_match_keyword();
      if (cVar3 != '\0') {
        uVar9 = 1;
        goto LAB_00579a20;
      }
      cVar3 = hwreq_token_match_keyword();
      if (cVar3 == '\0') {
        cVar3 = hwreq_token_match_keyword();
        if (cVar3 == '\0') {
          cVar3 = hwreq_token_match_keyword();
          if (cVar3 == '\0') {
            cVar3 = hwreq_token_match_keyword();
            if (cVar3 == '\0') {
              return "Unknown OS";
            }
            uVar9 = 5;
            iVar5 = *(int *)(unaff_EDI + 8) + 5;
            goto LAB_00579a26;
          }
          uVar9 = 4;
        }
        else {
          uVar9 = 3;
        }
        goto LAB_00579a20;
      }
      uVar9 = 2;
      iVar5 = *(int *)(unaff_EDI + 8) + 7;
    }
    else {
      uVar9 = 0;
LAB_00579a20:
      iVar5 = *(int *)(unaff_EDI + 8) + 5;
    }
LAB_00579a26:
    *(int *)(unaff_EDI + 8) = iVar5;
  }
  else {
    uVar9 = hwreq_token_parse_number();
    if (uVar9 == 0xffffffff) {
      return "Number expected";
    }
  }
  switch(uVar10) {
  case 0:
    return (char *)(uint)(unaff_ESI == uVar9);
  case 1:
    return (char *)(uint)(unaff_ESI != uVar9);
  case 2:
    return (char *)(uint)(uVar9 < unaff_ESI);
  case 3:
    return (char *)(uint)(unaff_ESI < uVar9);
  case 4:
    return (char *)(uint)(uVar9 <= unaff_ESI);
  case 5:
    return (char *)(uint)(unaff_ESI <= uVar9);
  case 6:
    return (char *)(uint)((uVar9 & unaff_ESI) != 0);
  }
switchD_00579953_default:
  return "Invalid";
}
#endif
