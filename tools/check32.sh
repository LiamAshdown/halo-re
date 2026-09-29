#!/bin/bash
# 32-bit Windows-layout type check of src files on a Linux host: clang -target i686-w64-windows-gnu with the mingw-w64
# headers (apt install clang mingw-w64-i686-dev), so the offset checks in types/ are evaluated with the game's layout.
# No MSVC, no binary: it proves the C still type-checks, not that the compiled code is identical.
# Usage: tools/check32.sh [file.c ...]     (default: every src/*/*.c); prints FAIL lines, exit 1 if any failed
cd "$(dirname "$0")/.." || exit 2
WIN_INCLUDE=${HALO_WIN_INCLUDE:-/usr/i686-w64-mingw32/include}
one() {
  out=$(clang -target i686-w64-windows-gnu -fsyntax-only -fms-extensions -Wno-everything \
        -Werror=implicit-function-declaration -ferror-limit=5 -isystem "$WIN_INCLUDE" -I types "$1" 2>&1)
  if [ $? -ne 0 ]; then echo "FAIL $1"; echo "$out" | grep error | head -3; fi
}
export -f one; export WIN_INCLUDE
if [ $# -eq 0 ]; then set -- src/*/*.c; fi
out=$(printf '%s\n' "$@" | xargs -P "$(nproc)" -I{} bash -c 'one {}')
[ -n "$out" ] && echo "$out"
fails=$(printf '%s' "$out" | grep -c '^FAIL')
echo "check32: $(( $# - fails )) ok, $fails failed"
[ "$fails" -eq 0 ]
