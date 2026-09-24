// cpu_query_identification  (Ghidra: cpu_query_identification, already named)
// address 0x540da0, size 310 bytes
// name confidence: 0.7   rewrite confidence: 0.55
// evidence: the EFLAGS.ID-bit toggle test (pushfd/xor 0x200000/popfd/pushfd, compare) is the
// standard CPUID-availability probe; Ghidra could not model pushfd/popfd and instead
// reconstructed the flags word bit by bit from uninitialized-register reads (in_CF, in_PF, ...),
// which is not compilable C, so this rewrite uses the equivalent inline asm directly (same as
// rasterizer_fpu_reset_control_word's __GNUC__/MSVC split in
// src/rasterizer/rasterizer_initialize_direct3d.c). The register store order for each CPUID leaf
// (EAX, EBX, EDX, ECX -- EDX before ECX) is pinned by the vendor string ("Genu""ineI""ntel" needs
// EBX, EDX, ECX at +0/+4/+8) and matches every other assignment in this function once applied
// consistently.
// UNSURE: the original called several tiny wrapper functions (cpuid, cpuid_basic_info,
// cpuid_Version_info, cpuid_brand_part1_info, cpuid_brand_part2_info, cpuid_brand_part3_info)
// that each just execute the cpuid instruction and return a pointer to a static register buffer;
// none of them appear in out/functions.json (folded/local symbols from a statically linked CPU-ID
// sample library, not part of this module), so their addresses are not known and this rewrite
// inlines the cpuid instruction directly instead of declaring calls to unresolvable externs.
// register convention: no arguments; return value in EAX only (the original's CONCAT44 high half,
// the 0x80000000 leaf's EDX, is never read by the one caller, cpu_get_type).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern char cpu_vendor_string[0x10];      // 0x006e35ac EBX/EDX/ECX of leaf 0, 12 bytes
extern char cpu_brand_string[0x30];       // 0x006e357c EAX/EBX/ECX/EDX of leaves 0x80000002..4
extern uint32_t cpu_signature;            // 0x00721e64 leaf 1 EAX
extern uint32_t cpu_features;             // 0x00721e5c leaf 1 EDX
extern uint32_t cpu_extended_features;    // 0x00721e60 leaf 0x80000001 EDX
extern uint32_t cpu_l1_tlb_large;         // 0x00721e68 leaf 0x80000005 EAX
extern uint32_t cpu_l1_tlb_4k;            // 0x00721e6c leaf 0x80000005 EBX
extern uint32_t cpu_l1_data_cache;        // 0x00721e70 leaf 0x80000005 ECX
extern uint32_t cpu_l1_code_cache;        // 0x00721e74 leaf 0x80000005 EDX
extern uint32_t cpu_l2_tlb_large;         // 0x00721e78 leaf 0x80000006 EAX
extern uint32_t cpu_l2_tlb_4k;            // 0x00721e7c leaf 0x80000006 EBX
extern uint32_t cpu_l2_cache;             // 0x00721e80 leaf 0x80000006 ECX
extern uint32_t cpu_l2_unknown;           // 0x00721e84 leaf 0x80000006 EDX, stored but never queried

static void cpu_execute_cpuid(uint32_t leaf, uint32_t *out_eax, uint32_t *out_ebx,
                               uint32_t *out_ecx, uint32_t *out_edx)
{
#if defined(__GNUC__)
    __asm__ volatile ("cpuid"
                       : "=a"(*out_eax), "=b"(*out_ebx), "=c"(*out_ecx), "=d"(*out_edx)
                       : "a"(leaf));
#else
    uint32_t a, b, c, d;
    __asm {
        mov eax, leaf
        cpuid
        mov a, eax
        mov b, ebx
        mov c, ecx
        mov d, edx
    }
    *out_eax = a; *out_ebx = b; *out_ecx = c; *out_edx = d;
#endif
}

// Detects CPUID availability (the EFLAGS.ID bit can be toggled) and, if present, fills the
// module's cached CPU-identification globals (vendor string, feature/family DWORD, extended
// cache and brand-string leaves) from the CPUID instruction. Returns 1 on success, -1 if the
// processor has no CPUID instruction.
int32_t cpu_query_identification(void)
{
    uint32_t original_flags;
    uint32_t readback_flags;
    uint32_t eax, ebx, ecx, edx;
    uint32_t max_extended_leaf;

#if defined(__GNUC__)
    __asm__ volatile (
        "pushfl\n\t"
        "popl %0\n\t"
        "movl %0, %1\n\t"
        "xorl $0x200000, %1\n\t"
        "pushl %1\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %1\n\t"
        "pushl %0\n\t"
        "popfl\n\t"
        : "=&r"(original_flags), "=&r"(readback_flags));
#else
    __asm {
        pushfd
        pop eax
        mov original_flags, eax
        xor eax, 0x200000
        push eax
        popfd
        pushfd
        pop eax
        mov readback_flags, eax
        push original_flags
        popfd
    }
#endif

    if ((readback_flags ^ original_flags) == 0) {
        return -1;
    }

    cpu_execute_cpuid(0, &eax, &ebx, &ecx, &edx);
    *(uint32_t *)(cpu_vendor_string + 0) = ebx;
    *(uint32_t *)(cpu_vendor_string + 4) = edx;
    *(uint32_t *)(cpu_vendor_string + 8) = ecx;

    if (eax == 0) {
        return 1;
    }

    cpu_execute_cpuid(1, &eax, &ebx, &ecx, &edx);
    cpu_signature = eax;
    cpu_features = edx;

    cpu_execute_cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    max_extended_leaf = eax;
    if (max_extended_leaf <= 0x80000000) {
        return 1;
    }

    if (max_extended_leaf > 0x80000004) {
        if (max_extended_leaf > 0x80000005) {
            cpu_execute_cpuid(0x80000006, &eax, &ebx, &ecx, &edx);
            cpu_l2_tlb_large = eax;
            cpu_l2_tlb_4k = ebx;
            cpu_l2_unknown = edx;
            cpu_l2_cache = ecx;
        }
        cpu_execute_cpuid(0x80000005, &eax, &ebx, &ecx, &edx);
        cpu_l1_tlb_large = eax;
        cpu_l1_tlb_4k = ebx;
        cpu_l1_code_cache = edx;
        cpu_l1_data_cache = ecx;

        cpu_execute_cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
        *(uint32_t *)(cpu_brand_string + 0x00) = eax;
        *(uint32_t *)(cpu_brand_string + 0x04) = ebx;
        *(uint32_t *)(cpu_brand_string + 0x08) = ecx;
        *(uint32_t *)(cpu_brand_string + 0x0c) = edx;

        cpu_execute_cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
        *(uint32_t *)(cpu_brand_string + 0x10) = eax;
        *(uint32_t *)(cpu_brand_string + 0x14) = ebx;
        *(uint32_t *)(cpu_brand_string + 0x18) = ecx;
        *(uint32_t *)(cpu_brand_string + 0x1c) = edx;

        cpu_execute_cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
        *(uint32_t *)(cpu_brand_string + 0x20) = eax;
        *(uint32_t *)(cpu_brand_string + 0x24) = ebx;
        *(uint32_t *)(cpu_brand_string + 0x28) = ecx;
        *(uint32_t *)(cpu_brand_string + 0x2c) = edx;
    }

    cpu_execute_cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
    cpu_extended_features = edx;
    return 1;
}

#if 0
Original Ghidra decompilation (0x540da0):


/* WARNING: Removing unreachable block (ram,0x00540ebe) */
/* WARNING: Removing unreachable block (ram,0x00540ea0) */
/* WARNING: Removing unreachable block (ram,0x00540e82) */
/* WARNING: Removing unreachable block (ram,0x00540e64) */
/* WARNING: Removing unreachable block (ram,0x00540e46) */
/* WARNING: Removing unreachable block (ram,0x00540e28) */
/* WARNING: Removing unreachable block (ram,0x00540dfd) */
/* WARNING: Removing unreachable block (ram,0x00540deb) */
/* WARNING: Removing unreachable block (ram,0x00540dc5) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 cpu_query_identification(void)

{
  int *piVar1;
  undefined4 *puVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  byte in_CF;
  byte in_PF;
  byte in_AF;
  byte in_ZF;
  byte in_SF;
  byte in_TF;
  byte in_IF;
  byte in_OF;
  byte in_NT;
  byte in_AC;
  byte in_VIF;
  byte in_VIP;
  byte in_ID;
  uint uVar6;
  
  uVar6 = (uint)(in_NT & 1) * 0x4000 | (uint)(in_OF & 1) * 0x800 | (uint)(in_IF & 1) * 0x200 |
          (uint)(in_TF & 1) * 0x100 | (uint)(in_SF & 1) * 0x80 | (uint)(in_ZF & 1) * 0x40 |
          (uint)(in_AF & 1) * 0x10 | (uint)(in_PF & 1) * 4 | (uint)(in_CF & 1) |
          (uint)(in_ID & 1) * 0x200000 | (uint)(in_VIP & 1) * 0x100000 |
          (uint)(in_VIF & 1) * 0x80000 | (uint)(in_AC & 1) * 0x40000;
  uVar5 = uVar6 ^ 0x200000;
  if (((uint)((uVar5 & 0x4000) != 0) * 0x4000 | (uint)((uVar5 & 0x800) != 0) * 0x800 |
       (uint)((uVar5 & 0x200) != 0) * 0x200 | (uint)((uVar5 & 0x100) != 0) * 0x100 |
       (uint)((uVar5 & 0x80) != 0) * 0x80 | (uint)((uVar5 & 0x40) != 0) * 0x40 |
       (uint)((uVar5 & 0x10) != 0) * 0x10 | (uint)((uVar5 & 4) != 0) * 4 | (uint)((uVar5 & 1) != 0)
       | (uint)((uVar5 & 0x200000) != 0) * 0x200000 | (uint)((uVar5 & 0x40000) != 0) * 0x40000) ==
      uVar6) {
    uVar5 = 0xffffffff;
  }
  else {
    piVar1 = (int *)cpuid_basic_info(0);
    _DAT_006e35ac = piVar1[1];
    _DAT_006e35b0 = piVar1[2];
    _DAT_006e35b4 = piVar1[3];
    uVar5 = 1;
    uVar6 = _DAT_006e35b0;
    if (*piVar1 != 0) {
      puVar2 = (undefined4 *)cpuid_Version_info(1);
      DAT_00721e64 = *puVar2;
      DAT_00721e5c = puVar2[2];
      puVar3 = (uint *)cpuid(0x80000000);
      uVar5 = *puVar3;
      uVar6 = puVar3[2];
      if (0x80000000 < uVar5) {
        if (0x80000003 < uVar5) {
          if (0x80000004 < uVar5) {
            if (0x80000005 < uVar5) {
              puVar2 = (undefined4 *)cpuid(0x80000006);
              DAT_00721e78 = *puVar2;
              DAT_00721e7c = puVar2[1];
              _DAT_00721e84 = puVar2[2];
              DAT_00721e80 = puVar2[3];
            }
            puVar2 = (undefined4 *)cpuid(0x80000005);
            DAT_00721e68 = *puVar2;
            DAT_00721e6c = puVar2[1];
            DAT_00721e74 = puVar2[2];
            DAT_00721e70 = puVar2[3];
          }
          puVar2 = (undefined4 *)cpuid_brand_part1_info(0x80000002);
          _DAT_006e357c = *puVar2;
          _DAT_006e3580 = puVar2[1];
          _DAT_006e3588 = puVar2[2];
          _DAT_006e3584 = puVar2[3];
          puVar2 = (undefined4 *)cpuid_brand_part2_info(0x80000003);
          _DAT_006e358c = *puVar2;
          _DAT_006e3590 = puVar2[1];
          _DAT_006e3598 = puVar2[2];
          _DAT_006e3594 = puVar2[3];
          puVar2 = (undefined4 *)cpuid_brand_part3_info(0x80000004);
          _DAT_006e359c = *puVar2;
          _DAT_006e35a0 = puVar2[1];
          _DAT_006e35a8 = puVar2[2];
          _DAT_006e35a4 = puVar2[3];
        }
        iVar4 = cpuid(0x80000001);
        uVar6 = *(uint *)(iVar4 + 8);
        uVar5 = 1;
        DAT_00721e60 = uVar6;
      }
    }
  }
  return CONCAT44(uVar6,uVar5);
}
#endif
