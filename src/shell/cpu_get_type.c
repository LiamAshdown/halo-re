// cpu_get_type  (Ghidra: cpu_get_type, already named)
// address 0x5402a0, size 2353 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: the query values and every switch arm match the cpu_query / cpu_vendor / cpu_model
// enums in shell.h field for field (see the notes there); cpu_query_identification (0x540da0)
// lazily fills the CPUID cache this function reads.
// register convention: __cdecl (mode is a normal stack argument; the two recursive calls and
// the direct call both pass it on the stack).
//
// The SEH frame (ExceptionList save/restore around the whole body) is MSVC /EHsc boilerplate
// with no C-level effect here (nothing in the body can throw) and is dropped from the rewrite.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t _strncmp(const char *a, const char *b, uint32_t count);

extern int32_t cpu_identification_state; // 0x00721e88
extern char cpu_vendor_string[0x10];     // 0x006e35ac
extern char cpu_brand_string[0x30];      // 0x006e357c
extern uint32_t cpu_signature;           // 0x00721e64
extern uint32_t cpu_features;            // 0x00721e5c
extern uint32_t cpu_extended_features;   // 0x00721e60
extern uint32_t cpu_l1_tlb_large;        // 0x00721e68
extern uint32_t cpu_l1_tlb_4k;           // 0x00721e6c
extern uint32_t cpu_l1_data_cache;       // 0x00721e70
extern uint32_t cpu_l1_code_cache;       // 0x00721e74
extern uint32_t cpu_l2_tlb_large;        // 0x00721e78
extern uint32_t cpu_l2_tlb_4k;           // 0x00721e7c
extern uint32_t cpu_l2_cache;            // 0x00721e80

extern int32_t cpu_query_identification(void); // 0x00540da0

// Lazily gathers CPUID data (via cpu_query_identification) then, depending on mode, returns
// either the CPU vendor / model, one of the cached string pointers, a single feature bit, or a
// cache/TLB descriptor field (AMD only; the L1-large and L2 fields are Athlon only).
int32_t cpu_get_type(int32_t mode)
{
    int32_t result;

    if (cpu_identification_state == 0) {
        cpu_identification_state = cpu_query_identification();
    }
    if (cpu_identification_state == -1) {
        return 0;
    }

    switch (mode) {
    case k_cpu_query_vendor:
        if (_strncmp(cpu_vendor_string, "AuthenticAMD", 0xc) == 0) return k_cpu_vendor_amd;
        if (_strncmp(cpu_vendor_string, "GenuineIntel", 0xc) == 0) return k_cpu_vendor_intel;
        if (_strncmp(cpu_vendor_string, "CyrixInstead", 0xc) == 0) return k_cpu_vendor_cyrix;
        if (_strncmp(cpu_vendor_string, "CentaurHauls", 0xc) == 0) return k_cpu_vendor_centaur;
        return k_cpu_vendor_unknown;

    case k_cpu_query_model:
        result = cpu_get_type(k_cpu_query_vendor);
        if (result == k_cpu_vendor_amd) {
            uint32_t family;
            family = (cpu_signature >> 8) & 0xf;
            if (family == 4) return k_cpu_model_amd_family4;
            if (family == 5) {
                switch ((cpu_signature >> 4) & 0xf) {
                case 0: case 1: case 2: case 3: return k_cpu_model_amd_k5;
                case 4: case 5: case 6: case 7: return k_cpu_model_amd_k6;
                case 8: return k_cpu_model_amd_k6_2;
                case 9: case 10: case 11: case 12: case 13: case 14: case 15:
                    return k_cpu_model_amd_k6_3;
                default: return k_cpu_model_unknown;
                }
            }
            if (family == 6) return k_cpu_model_amd_athlon;
            return k_cpu_model_unknown;
        }
        if (result == k_cpu_vendor_intel) {
            uint32_t family = (cpu_signature >> 8) & 0xf;
            if (family == 4) {
                switch ((cpu_signature >> 4) & 0xf) {
                case 0: case 1: return k_cpu_model_intel_486dx;
                case 2: return k_cpu_model_intel_486sx;
                case 3: return k_cpu_model_intel_486dx2;
                case 4: return k_cpu_model_intel_486sl;
                case 5: return k_cpu_model_intel_486sx2;
                default: return k_cpu_model_unknown;
                case 7: return k_cpu_model_intel_486dx2_write_back;
                case 8: return k_cpu_model_intel_486dx4;
                }
            }
            if (family == 5) {
                switch ((cpu_signature >> 4) & 0xf) {
                case 1: case 2: case 3: return k_cpu_model_intel_pentium;
                case 4: return k_cpu_model_intel_pentium_mmx;
                default: return k_cpu_model_unknown;
                }
            }
            if (family == 6) {
                switch ((cpu_signature >> 4) & 0xf) {
                case 1: return k_cpu_model_intel_pentium_pro;
                default: return k_cpu_model_unknown;
                case 3: case 5: return k_cpu_model_intel_pentium_2;
                case 6: return k_cpu_model_intel_celeron;
                case 7: return k_cpu_model_intel_pentium_3;
                }
            }
            return k_cpu_model_unknown;
        }
        return k_cpu_model_unknown;

    case k_cpu_query_vendor_string: return (int32_t)cpu_vendor_string;
    case k_cpu_query_brand_string:  return (int32_t)cpu_brand_string;
    case k_cpu_query_cpuid_available: return 1;

    case k_cpu_query_fpu:    return cpu_features & 1;
    case k_cpu_query_vme:    return (cpu_features >> 1) & 1;
    case k_cpu_query_de:     return (cpu_features >> 2) & 1;
    case k_cpu_query_pse:    return (cpu_features >> 3) & 1;
    case k_cpu_query_tsc:    return (cpu_features >> 4) & 1;
    case k_cpu_query_msr:    return (cpu_features >> 5) & 1;
    case k_cpu_query_pae:    return (cpu_features >> 6) & 1;
    case k_cpu_query_mce:    return (cpu_features >> 7) & 1;
    case k_cpu_query_cx8:    return (cpu_features >> 8) & 1;
    case k_cpu_query_apic:   return (cpu_features >> 9) & 1;
    case k_cpu_query_sep:    return (cpu_features >> 0xb) & 1;
    case k_cpu_query_mtrr:   return (cpu_features >> 0xc) & 1;
    case k_cpu_query_pge:    return (cpu_features >> 0xd) & 1;
    case k_cpu_query_mca:    return (cpu_features >> 0xe) & 1;
    case k_cpu_query_cmov:   return (cpu_features >> 0xf) & 1;
    case k_cpu_query_pat:    return (cpu_features >> 0x10) & 1;
    case k_cpu_query_pse36:  return (cpu_features >> 0x11) & 1;

    // bit 0x19 of cpu_features (leaf 1 EDX bit25, SSE) OR bit 0x16 of cpu_extended_features
    // (0x80000001 EDX bit22, AMD MMX extensions); Ghidra folded this into one shifted-OR.
    case k_cpu_query_mmx_extensions:
        return ((cpu_features >> 3) | cpu_extended_features) >> 0x16 & 1;

    case k_cpu_query_mmx:    return (cpu_features >> 0x17) & 1;
    case k_cpu_query_fxsr:   return (cpu_features >> 0x18) & 1;
    case k_cpu_query_3dnow_extensions: return (cpu_extended_features >> 0x1e) & 1;
    case k_cpu_query_3dnow:             return cpu_extended_features >> 0x1f;
    case k_cpu_query_amd_mmx_extensions: return (cpu_extended_features >> 0x16) & 1;
    case k_cpu_query_sse:    return (cpu_features >> 0x19) & 1;
    case k_cpu_query_sse_usable:
        return cpu_get_type(k_cpu_query_sse) != 0 ? 1 : 0;
    case k_cpu_query_sse2:   return (cpu_features >> 0x1a) & 1;
    case k_cpu_query_sse2_usable:
        return cpu_get_type(k_cpu_query_sse2) != 0 ? 1 : 0;

    default:
        // 0x20..0x3b: cache/TLB descriptor fields, AMD only.
        if (cpu_get_type(k_cpu_query_vendor) != k_cpu_vendor_amd) {
            return 0;
        }

        result = 0;
        switch (mode) {
        case k_cpu_query_l1_tlb_4k_data_associativity: result = cpu_l1_tlb_4k >> 0x18; break;
        case k_cpu_query_l1_tlb_4k_data_entries:       result = (cpu_l1_tlb_4k >> 0x10) & 0xff; break;
        case k_cpu_query_l1_tlb_4k_code_associativity: result = (cpu_l1_tlb_4k >> 8) & 0xff; break;
        case k_cpu_query_l1_tlb_4k_code_entries:       result = cpu_l1_tlb_4k & 0xff; break;
        case k_cpu_query_l1_data_cache_size:           result = cpu_l1_data_cache >> 0x18; break;
        case k_cpu_query_l1_data_cache_associativity:  result = (cpu_l1_data_cache >> 0x10) & 0xff; break;
        case k_cpu_query_l1_data_cache_lines_per_tag:  result = (cpu_l1_data_cache >> 8) & 0xff; break;
        case k_cpu_query_l1_data_cache_line_size:      result = cpu_l1_data_cache & 0xff; break;
        case k_cpu_query_l1_code_cache_size:           result = cpu_l1_code_cache >> 0x18; break;
        case k_cpu_query_l1_code_cache_associativity:  result = (cpu_l1_code_cache >> 0x10) & 0xff; break;
        case k_cpu_query_l1_code_cache_lines_per_tag:  result = (cpu_l1_code_cache >> 8) & 0xff; break;
        case k_cpu_query_l1_code_cache_line_size:      result = cpu_l1_code_cache & 0xff; break;
        case k_cpu_query_l2_cache_size:                result = cpu_l2_cache >> 0x10; break;
        case k_cpu_query_l2_cache_associativity:       result = (cpu_l2_cache >> 0xc) & 0xf; break;
        case k_cpu_query_l2_cache_lines_per_tag:       result = (cpu_l2_cache >> 8) & 0xf; break;
        case k_cpu_query_l2_cache_line_size:           result = cpu_l2_cache & 0xff; break;
        default: break;
        }

        // Athlon (family 6) only: L1-large TLB and L2 TLB fields.
        if (cpu_get_type(k_cpu_query_model) == k_cpu_model_amd_athlon) {
            switch (mode) {
            case k_cpu_query_l1_tlb_large_data_associativity: return cpu_l1_tlb_large >> 0x18;
            case k_cpu_query_l1_tlb_large_data_entries:       return (cpu_l1_tlb_large >> 0x10) & 0xff;
            case k_cpu_query_l1_tlb_large_code_associativity: return (cpu_l1_tlb_large >> 8) & 0xff;
            case k_cpu_query_l1_tlb_large_code_entries:       return cpu_l1_tlb_large & 0xff;
            case k_cpu_query_l2_tlb_large_data_associativity: return cpu_l2_tlb_large >> 0x1c;
            case k_cpu_query_l2_tlb_large_data_entries:       result = (cpu_l2_tlb_large >> 0x10) & 0xfff; break;
            case k_cpu_query_l2_tlb_large_code_associativity: return (cpu_l2_tlb_large >> 0xc) & 0xf;
            case k_cpu_query_l2_tlb_large_code_entries:       result = cpu_l2_tlb_large & 0xfff; break;
            case k_cpu_query_l2_tlb_4k_data_associativity:    return cpu_l2_tlb_4k >> 0x1c;
            case k_cpu_query_l2_tlb_4k_data_entries:          result = (cpu_l2_tlb_4k >> 0x10) & 0xfff; break;
            case k_cpu_query_l2_tlb_4k_code_associativity:    return (cpu_l2_tlb_4k >> 0xc) & 0xf;
            case k_cpu_query_l2_tlb_4k_code_entries:          result = cpu_l2_tlb_4k & 0xfff; break;
            default: break;
            }
        }
        return result;
    }
}

#if 0
Original Ghidra decompilation (0x5402a0):


int __cdecl cpu_get_type(int mode)

{
  int iVar1;
  uint uVar2;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_00672b40;
  puStack_10 = &LAB_00628dfc;
  local_14 = ExceptionList;
  uVar2 = 0;
  ExceptionList = &local_14;
  if (DAT_00721e88 == 0) {
    ExceptionList = &local_14;
    DAT_00721e88 = cpu_query_identification();
  }
  if (DAT_00721e88 == -1) {
    ExceptionList = local_14;
    return 0;
  }
  switch(mode) {
  case 0:
    iVar1 = _strncmp(&DAT_006e35ac,"AuthenticAMD",0xc);
    if (iVar1 == 0) {
      ExceptionList = local_14;
      return 1;
    }
    iVar1 = _strncmp(&DAT_006e35ac,"GenuineIntel",0xc);
    if (iVar1 != 0) {
      iVar1 = _strncmp(&DAT_006e35ac,"CyrixInstead",0xc);
      if (iVar1 == 0) {
        ExceptionList = local_14;
        return 3;
      }
      iVar1 = _strncmp(&DAT_006e35ac,"CentaurHauls",0xc);
      ExceptionList = local_14;
      return (-(uint)(iVar1 != 0) & 0xfffffffc) + 4;
    }
switchD_0054042d_caseD_0:
    ExceptionList = local_14;
    return 2;
  case 1:
    iVar1 = cpu_get_type(0);
    switch(iVar1) {
    case 1:
      goto switchD_005403d7_caseD_1;
    case 2:
      uVar2 = DAT_00721e64 >> 8 & 0xf;
      if (uVar2 == 4) {
        switch(DAT_00721e64 >> 4 & 0xf) {
        case 0:
        case 1:
          ExceptionList = local_14;
          return 7;
        case 2:
          ExceptionList = local_14;
          return 8;
        case 3:
          ExceptionList = local_14;
          return 9;
        case 4:
          ExceptionList = local_14;
          return 10;
        case 5:
          ExceptionList = local_14;
          return 0xb;
        default:
          ExceptionList = local_14;
          return 0;
        case 7:
          ExceptionList = local_14;
          return 0xc;
        case 8:
          ExceptionList = local_14;
          return 0xd;
        }
      }
      if (uVar2 == 5) {
        switch(DAT_00721e64 >> 4 & 0xf) {
        case 1:
        case 2:
        case 3:
          ExceptionList = local_14;
          return 0xe;
        case 4:
          ExceptionList = local_14;
          return 0xf;
        default:
          ExceptionList = local_14;
          return 0;
        }
      }
      if (uVar2 == 6) {
        switch(DAT_00721e64 >> 4 & 0xf) {
        case 1:
          ExceptionList = local_14;
          return 0x10;
        default:
          ExceptionList = local_14;
          return 0;
        case 3:
        case 5:
          ExceptionList = local_14;
          return 0x11;
        case 6:
          ExceptionList = local_14;
          return 0x12;
        case 7:
          ExceptionList = local_14;
          return 0x13;
        }
      }
      ExceptionList = local_14;
      return 0;
    default:
      ExceptionList = local_14;
      return 0;
    }
  case 2:
    ExceptionList = local_14;
    return 0x6e35ac;
  case 3:
    ExceptionList = local_14;
    return 0x6e357c;
  case 4:
    ExceptionList = local_14;
    return 1;
  case 5:
    ExceptionList = local_14;
    return DAT_00721e5c & 1;
  case 6:
    ExceptionList = local_14;
    return DAT_00721e5c >> 1 & 1;
  case 7:
    ExceptionList = local_14;
    return DAT_00721e5c >> 2 & 1;
  case 8:
    ExceptionList = local_14;
    return DAT_00721e5c >> 3 & 1;
  case 9:
    ExceptionList = local_14;
    return DAT_00721e5c >> 4 & 1;
  case 10:
    ExceptionList = local_14;
    return DAT_00721e5c >> 5 & 1;
  case 0xb:
    ExceptionList = local_14;
    return DAT_00721e5c >> 6 & 1;
  case 0xc:
    ExceptionList = local_14;
    return DAT_00721e5c >> 7 & 1;
  case 0xd:
    ExceptionList = local_14;
    return DAT_00721e5c >> 8 & 1;
  case 0xe:
    ExceptionList = local_14;
    return DAT_00721e5c >> 9 & 1;
  case 0xf:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0xb & 1;
  case 0x10:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0xc & 1;
  case 0x11:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0xd & 1;
  case 0x12:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0xe & 1;
  case 0x13:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0xf & 1;
  case 0x14:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0x10 & 1;
  case 0x15:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0x11 & 1;
  case 0x16:
    ExceptionList = local_14;
    return (DAT_00721e5c >> 3 | DAT_00721e60) >> 0x16 & 1;
  case 0x17:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0x17 & 1;
  case 0x18:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0x18 & 1;
  case 0x19:
    ExceptionList = local_14;
    return DAT_00721e60 >> 0x1e & 1;
  case 0x1a:
    ExceptionList = local_14;
    return DAT_00721e60 >> 0x1f;
  case 0x1b:
    ExceptionList = local_14;
    return DAT_00721e60 >> 0x16 & 1;
  case 0x1c:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0x19 & 1;
  case 0x1d:
    iVar1 = cpu_get_type(0x1c);
    if (iVar1 != 0) {
      ExceptionList = local_14;
      return 1;
    }
    ExceptionList = local_14;
    return 0;
  case 0x1e:
    ExceptionList = local_14;
    return DAT_00721e5c >> 0x1a & 1;
  case 0x1f:
    iVar1 = cpu_get_type(0x1e);
    if (iVar1 != 0) {
      ExceptionList = local_14;
      return 1;
    }
    ExceptionList = local_14;
    return 0;
  default:
    iVar1 = cpu_get_type(0);
    if (iVar1 != 1) {
      ExceptionList = local_14;
      return 0;
    }
  }
  switch(mode) {
  case 0x20:
    uVar2 = DAT_00721e6c >> 0x18;
    break;
  case 0x21:
    uVar2 = DAT_00721e6c >> 0x10 & 0xff;
    break;
  case 0x22:
    uVar2 = DAT_00721e6c >> 8 & 0xff;
    break;
  case 0x23:
    uVar2 = DAT_00721e6c;
    goto LAB_00540ab1;
  case 0x28:
    uVar2 = DAT_00721e70 >> 0x18;
    break;
  case 0x29:
    uVar2 = DAT_00721e70 >> 0x10 & 0xff;
    break;
  case 0x2a:
    uVar2 = DAT_00721e70 >> 8 & 0xff;
    break;
  case 0x2b:
    uVar2 = DAT_00721e70;
    goto LAB_00540ab1;
  case 0x2c:
    uVar2 = DAT_00721e74 >> 0x18;
    break;
  case 0x2d:
    uVar2 = DAT_00721e74 >> 0x10 & 0xff;
    break;
  case 0x2e:
    uVar2 = DAT_00721e74 >> 8 & 0xff;
    break;
  case 0x2f:
    uVar2 = DAT_00721e74;
    goto LAB_00540ab1;
  case 0x30:
    uVar2 = DAT_00721e80 >> 0x10;
    break;
  case 0x31:
    uVar2 = DAT_00721e80 >> 0xc & 0xf;
    break;
  case 0x32:
    uVar2 = DAT_00721e80 >> 8 & 0xf;
    break;
  case 0x33:
    uVar2 = DAT_00721e80;
LAB_00540ab1:
    uVar2 = uVar2 & 0xff;
  }
  iVar1 = cpu_get_type(1);
  if (iVar1 == 6) {
    switch(mode) {
    case 0x24:
      ExceptionList = local_14;
      return DAT_00721e68 >> 0x18;
    case 0x25:
      ExceptionList = local_14;
      return DAT_00721e68 >> 0x10 & 0xff;
    case 0x26:
      ExceptionList = local_14;
      return DAT_00721e68 >> 8 & 0xff;
    case 0x27:
      ExceptionList = local_14;
      return DAT_00721e68 & 0xff;
    default:
      goto switchD_00540add_caseD_28;
    case 0x34:
      ExceptionList = local_14;
      return DAT_00721e78 >> 0x1c;
    case 0x35:
      uVar2 = DAT_00721e78 >> 0x10;
      break;
    case 0x36:
      ExceptionList = local_14;
      return DAT_00721e78 >> 0xc & 0xf;
    case 0x37:
      uVar2 = DAT_00721e78;
      break;
    case 0x38:
      ExceptionList = local_14;
      return DAT_00721e7c >> 0x1c;
    case 0x39:
      uVar2 = DAT_00721e7c >> 0x10;
      break;
    case 0x3a:
      ExceptionList = local_14;
      return DAT_00721e7c >> 0xc & 0xf;
    case 0x3b:
      uVar2 = DAT_00721e7c;
    }
    uVar2 = uVar2 & 0xfff;
  }
switchD_00540add_caseD_28:
  ExceptionList = local_14;
  return uVar2;
switchD_005403d7_caseD_1:
  uVar2 = DAT_00721e64 >> 8 & 0xf;
  if (uVar2 == 4) {
    ExceptionList = local_14;
    return 1;
  }
  if (uVar2 == 5) {
    switch(DAT_00721e64 >> 4 & 0xf) {
    case 0:
    case 1:
    case 2:
    case 3:
      goto switchD_0054042d_caseD_0;
    case 4:
    case 5:
    case 6:
    case 7:
      ExceptionList = local_14;
      return 3;
    case 8:
      ExceptionList = local_14;
      return 4;
    case 9:
    case 10:
    case 0xb:
    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
      ExceptionList = local_14;
      return 5;
    default:
      ExceptionList = local_14;
      return 0;
    }
  }
  if (uVar2 == 6) {
    ExceptionList = local_14;
    return 6;
  }
  ExceptionList = local_14;
  return 0;
}
#endif
