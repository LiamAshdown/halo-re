// security_check_write_access  (Ghidra: security_check_write_access, already named)
// address 0x542840, size 568 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: classic "can this token write here" ACL probe (duplicate token, synthetic
// world-power-users SID, temporary security descriptor, AccessCheck against a GENERIC_MAPPING);
// caches its one-shot result in shell_access_check_state (0x0069eab0) and always reports
// access granted on non-NT platforms, matching the shell_access_check_state enum in shell.h.
// register convention: __cdecl, no arguments.
// VERIFIED against disassembly 0x542840..0x542a77 (2026-09-30): every API call, argument order and branch matches.
//   Fixed: the call to security_check_cleanup passed 5 arguments to a 6-parameter function (the missing sentinel, ESI == 0
//   in the original), so the callee compared every handle against garbage and freed NULL/handles wrongly.
// UNSURE: two dead stack writes in the original (local_48[1] = 1, local_48[2] = 3) do not feed
// any subsequent Win32 call argument and are kept here as writes to otherwise-unused locals for
// fidelity, without a resolved purpose. The SEH frame is dropped as in the other winmain-era
// functions.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"


extern int32_t os_platform_value; // 0x00721ef0, UNSURE: named os_platform_value not os_platform -- shell.h's os_platform enum typedef occupies that identifier in the same C namespace, so the documented global name does not compile as written
extern int32_t security_write_access_state; // 0x0069eab0

extern void os_platform_identify(void);        // 0x005427e0
extern void security_check_cleanup(void *descriptor, void *acl, void *sid, void *thread_token,
                                    void *impersonation_token, void *sentinel);                // 0x00542a80


// Determines (once, then caches) whether the current thread token has write access under a
// synthetic ACL, used to detect restricted/limited-user accounts on NT systems; always reports
// access granted on non-NT platforms.
int32_t security_check_write_access(void)
{
    void *thread_token;
    void *impersonation_token;
    void *sid;
    void *descriptor;
    void *acl;
    uint32_t sid_length;
    uint32_t acl_length;
    uint32_t privilege_set_length;
    int32_t access_granted;
    int32_t ok;
    sid_identifier_authority nt_authority;
    generic_mapping mapping;
    uint8_t privilege_set[0x14];
    uint32_t granted_access;
    uint32_t unused_local_1;
    uint32_t unused_local_2;

    if (security_write_access_state != -1) {
        return security_write_access_state == 1;
    }

    if (os_platform_value == 0) {
        os_platform_identify();
    }
    if (os_platform_value < k_os_platform_windows_nt) {
        security_write_access_state = k_shell_access_check_granted;
        return security_write_access_state == 1;
    }

    access_granted = 0;
    privilege_set_length = 0x14;
    acl = 0;
    sid = 0;
    thread_token = 0;
    impersonation_token = 0;
    descriptor = 0;
    nt_authority.value[0] = 0;
    nt_authority.value[1] = 0;
    nt_authority.value[2] = 0;
    nt_authority.value[3] = 0;
    nt_authority.value[4] = 0;
    nt_authority.value[5] = 5; // SECURITY_NT_AUTHORITY

    ok = OpenThreadToken(GetCurrentThread(), 10 /* TOKEN_QUERY|TOKEN_DUPLICATE */, 1, &thread_token);
    if (ok == 0) {
        if (GetLastError() == 0x3f0 /* ERROR_NO_TOKEN */) {
            ok = OpenProcessToken(GetCurrentProcess(), 10, &thread_token);
        }
    }

    if (ok != 0) {
        ok = DuplicateToken(thread_token, (SECURITY_IMPERSONATION_LEVEL)2 /* SecurityImpersonation */, &impersonation_token);
        if (ok != 0 &&
            (ok = AllocateAndInitializeSid((PSID_IDENTIFIER_AUTHORITY)&nt_authority, 2, 0x20, 0x220, 0, 0, 0, 0, 0, 0, &sid), ok != 0) &&
            (descriptor = LocalAlloc(0x40, 0x14), descriptor != 0) &&
            (ok = InitializeSecurityDescriptor(descriptor, 1), ok != 0)) {
            sid_length = GetLengthSid(sid);
            acl_length = sid_length + 0x10;
            acl = LocalAlloc(0x40, acl_length);
            if (acl != 0 && (ok = InitializeAcl((PACL)acl, acl_length, 2), ok != 0)) {
                unused_local_2 = 3;
                ok = AddAccessAllowedAce((PACL)acl, 2, 3, sid);
                if (ok != 0 && (ok = SetSecurityDescriptorDacl(descriptor, 1, (PACL)acl, 0), ok != 0)) {
                    SetSecurityDescriptorGroup(descriptor, sid, 0);
                    SetSecurityDescriptorOwner(descriptor, sid, 0);
                    ok = IsValidSecurityDescriptor(descriptor);
                    if (ok != 0) {
                        unused_local_1 = 1;
                        mapping.generic_read = 1;
                        mapping.generic_write = 2;
                        mapping.generic_execute = 0;
                        mapping.generic_all = 3;
                        ok = AccessCheck(descriptor, impersonation_token, 1, (PGENERIC_MAPPING)&mapping, (PPRIVILEGE_SET)privilege_set,
                                          (LPDWORD)&privilege_set_length, (LPDWORD)&granted_access, &access_granted);
                        if (ok == 0) {
                            access_granted = 0;
                        }
                    }
                }
            }
        }
    }

    (void)unused_local_1;
    (void)unused_local_2;

    security_check_cleanup(descriptor, acl, sid, thread_token, impersonation_token, 0); // 0x542a4a: ESI == 0 is the sentinel
    security_write_access_state = (access_granted == 1);
    return security_write_access_state == 1;
}

#if 0
Original Ghidra decompilation (0x542840):


int __cdecl security_check_write_access(void)

{
  HANDLE pvVar1;
  HLOCAL pSecurityDescriptor;
  PACL pAcl;
  SIZE_T uBytes;
  DWORD DVar2;
  BOOL BVar3;
  HANDLE *ppvVar4;
  _PRIVILEGE_SET local_74;
  GENERIC_MAPPING local_60;
  _SID_IDENTIFIER_AUTHORITY local_50;
  DWORD local_48 [3];
  SIZE_T local_3c;
  DWORD local_38;
  HLOCAL local_34;
  PACL local_30;
  BOOL local_2c;
  HANDLE local_28;
  HANDLE local_24;
  PSID local_20 [3];
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_00672e58;
  puStack_10 = &LAB_00628dfc;
  local_14 = ExceptionList;
  if (DAT_0069eab0 != 0xffffffff) goto LAB_00542a5d;
  ExceptionList = &local_14;
  if (DAT_00721ef0 == 0) {
    ExceptionList = &local_14;
    os_platform_identify();
  }
  if (DAT_00721ef0 < 3) {
    DAT_0069eab0 = 1;
    goto LAB_00542a5d;
  }
  local_2c = 0;
  local_38 = 0x14;
  local_30 = (PACL)0x0;
  local_20[0] = (PSID)0x0;
  local_24 = (HANDLE)0x0;
  local_28 = (HANDLE)0x0;
  local_34 = (HLOCAL)0x0;
  local_50.Value[0] = '\0';
  local_50.Value[1] = '\0';
  local_50.Value[2] = '\0';
  local_50.Value[3] = '\0';
  local_50.Value[4] = '\0';
  local_50.Value[5] = '\x05';
  local_8 = 0;
  ppvVar4 = &local_24;
  BVar3 = 1;
  DVar2 = 10;
  pvVar1 = GetCurrentThread();
  BVar3 = OpenThreadToken(pvVar1,DVar2,BVar3,ppvVar4);
  if (BVar3 == 0) {
    DVar2 = GetLastError();
    if (DVar2 == 0x3f0) {
      ppvVar4 = &local_24;
      DVar2 = 10;
      pvVar1 = GetCurrentProcess();
      BVar3 = OpenProcessToken(pvVar1,DVar2,ppvVar4);
      if (BVar3 != 0) goto LAB_00542910;
    }
  }
  else {
LAB_00542910:
    BVar3 = DuplicateToken(local_24,SecurityImpersonation,&local_28);
    if ((((BVar3 != 0) &&
         (BVar3 = AllocateAndInitializeSid(&local_50,'\x02',0x20,0x220,0,0,0,0,0,0,local_20),
         BVar3 != 0)) &&
        (pSecurityDescriptor = LocalAlloc(0x40,0x14), local_34 = pSecurityDescriptor,
        pSecurityDescriptor != (HLOCAL)0x0)) &&
       (BVar3 = InitializeSecurityDescriptor(pSecurityDescriptor,1), BVar3 != 0)) {
      DVar2 = GetLengthSid(local_20[0]);
      uBytes = DVar2 + 0x10;
      local_3c = uBytes;
      pAcl = LocalAlloc(0x40,uBytes);
      local_30 = pAcl;
      if ((pAcl != (PACL)0x0) && (BVar3 = InitializeAcl(pAcl,uBytes,2), BVar3 != 0)) {
        local_48[2] = 3;
        BVar3 = AddAccessAllowedAce(pAcl,2,3,local_20[0]);
        if ((BVar3 != 0) &&
           (BVar3 = SetSecurityDescriptorDacl(pSecurityDescriptor,1,pAcl,0), BVar3 != 0)) {
          SetSecurityDescriptorGroup(pSecurityDescriptor,local_20[0],0);
          SetSecurityDescriptorOwner(pSecurityDescriptor,local_20[0],0);
          BVar3 = IsValidSecurityDescriptor(pSecurityDescriptor);
          if (BVar3 != 0) {
            local_48[1] = 1;
            local_60.GenericRead = 1;
            local_60.GenericWrite = 2;
            local_60.GenericExecute = 0;
            local_60.GenericAll = 3;
            BVar3 = AccessCheck(pSecurityDescriptor,local_28,1,&local_60,&local_74,&local_38,
                                local_48,&local_2c);
            if (BVar3 == 0) {
              local_2c = 0;
            }
          }
        }
      }
    }
  }
  local_8 = 0xffffffff;
  security_check_cleanup();
  DAT_0069eab0 = (uint)(local_2c == 1);
LAB_00542a5d:
  ExceptionList = local_14;
  return (uint)(DAT_0069eab0 == 1);
}
#endif
