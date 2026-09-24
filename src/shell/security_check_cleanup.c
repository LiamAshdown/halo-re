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
// blam-cc: EBX -> descriptor, ESI -> sentinel, EDI -> acl; stack -> sid, thread_token, impersonation_token
// UNSURE: the original's "unaff_ESI" comparand could in principle be nonzero at the call site;
// nothing in security_check_write_access ever sets ESI to anything but 0 on this path, but it is
// now modeled as a genuine parameter (sentinel) rather than a hardcoded NULL.
// FIXED (register inputs, objdump): ESI carries sentinel (read at 0x542a80, cmp ebx,esi, the
// first instruction); it was missing, and the body compared every handle against a literal 0
// instead of this parameter. Note: this function's only caller (security_check_write_access, not
// in this batch) still calls it with its old 5-argument signature; that file needs updating to
// pass its own ESI/sentinel value (always 0 on its path per the UNSURE note above) but is out of
// scope for this pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void *LocalFree(void *memory);
extern void *CloseHandle(void *handle);
extern void *FreeSid(void *sid);

// Releases the SID, ACL/security-descriptor buffer, and token handles allocated inside
// security_check_write_access, each compared against `sentinel` (always NULL on the known path)
// rather than a hardcoded 0.
void security_check_cleanup(void *descriptor, void *acl, void *sid, void *thread_token,
                             void *impersonation_token, void *sentinel)
{
    if (descriptor != sentinel) {
        LocalFree(descriptor);
    }
    if (acl != sentinel) {
        LocalFree(acl);
    }
    if (sid != sentinel) {
        FreeSid(sid);
    }
    if (thread_token != sentinel) {
        CloseHandle(thread_token);
    }
    if (impersonation_token != sentinel) {
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
