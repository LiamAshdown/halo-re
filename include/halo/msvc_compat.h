/* Forced include (/FI) for the MSVC x86 build of src/ (tools/msvc_build.py). Not part of the reconstruction.
   __thiscall: the sound EAX effect methods were C++ members (ECX = this, callee-cleaned stack arguments).
   MSVC rejects __thiscall on a plain C function, so in this build they compile as ordinary C functions: calls
   between rewritten functions stay consistent, and the harness adapter for each such function supplies ECX
   as the first argument when original code calls it. */
#ifndef BLAM_MSVC_COMPAT_H
#define BLAM_MSVC_COMPAT_H
#define __thiscall
#endif
