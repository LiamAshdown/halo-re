// security_check_cleanup  (Ghidra: security_check_cleanup, already named)
// address 0x542a80, size 65 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: called once, at the end of security_check_write_access, to release exactly the
// five handles/pointers that function builds (security descriptor, ACL, SID, thread token,
// impersonation token).
// register convention: this helper does not set up its own frame; Ghidra shows it reading the
// caller's locals directly (unaff_EBX/ESI/EDI and two EBP-relative reads), which only makes
// sense read together with security_check_write_access's frame. Rewritten here as five explicit
// pointer parameters (the natural, semantics-preserving translation), in EBX, ESI, EDI, then the
// two stack-frame reads, and the caller updated to pass its five locals explicitly.
// blam-cc: descriptor in EBX, null-sentinel comparand in ESI (always NULL here), acl in EDI,
// sid and the two token handles read from the caller's frame -> now ordinary parameters
// UNSURE: the original's "unaff_ESI" comparand could in principle be nonzero at the call site;
// nothing in security_check_write_access ever sets ESI to anything but 0 on this path, so this
// rewrite compares each handle against NULL directly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void *LocalFree(void *memory);
extern void *CloseHandle(void *handle);
extern void *FreeSid(void *sid);

// Releases the SID, ACL/security-descriptor buffer, and token handles allocated inside
// security_check_write_access.
void security_check_cleanup(void *descriptor, void *acl, void *sid, void *thread_token,
                             void *impersonation_token)
{
    if (descriptor != 0) {
        LocalFree(descriptor);
    }
    if (acl != 0) {
        LocalFree(acl);
    }
    if (sid != 0) {
        FreeSid(sid);
    }
    if (thread_token != 0) {
        CloseHandle(thread_token);
    }
    if (impersonation_token != 0) {
        CloseHandle(impersonation_token);
    }
}

#if 0
Original Ghidra decompilation (0x542a80):


void security_check_cleanup(void)

{
  HLOCAL unaff_EBX;
  int unaff_EBP;
  PSID unaff_ESI;
  HLOCAL unaff_EDI;
  
  if (unaff_EBX != unaff_ESI) {
    LocalFree(unaff_EBX);
  }
  if (unaff_EDI != unaff_ESI) {
    LocalFree(unaff_EDI);
  }
  if (*(PSID *)(unaff_EBP + -0x1c) != unaff_ESI) {
    FreeSid(*(PSID *)(unaff_EBP + -0x1c));
  }
  if (*(HANDLE *)(unaff_EBP + -0x24) != unaff_ESI) {
    CloseHandle(*(HANDLE *)(unaff_EBP + -0x24));
  }
  if (*(HANDLE *)(unaff_EBP + -0x20) != unaff_ESI) {
    CloseHandle(*(HANDLE *)(unaff_EBP + -0x20));
  }
  return;
}
#endif
