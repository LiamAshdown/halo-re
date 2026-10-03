# glm (vendored)

OpenGL Mathematics (https://github.com/g-truc/glm), tag 1.0.1, commit 0af55ccecd98d4e5a8d1fad7de25ba429d60e863,
header-only, MIT licence (copying.txt). Only the glm/ header tree is kept (its CMakeLists.txt, glm.cpp and glm.cppm
were removed); nothing here is built and the build needs no network.

The engine includes it only through include/halo/math/glm_interop.hpp, which defines GLM_FORCE_PURE (no SIMD
intrinsics, the scalar code path the simulation's bit-exact checks were made with) and GLM_FORCE_XYZW_ONLY.
To update: replace glm/ and copying.txt from a new tag, then run tests/math/run_difftest.py; every glm-based engine
function must still match the original build byte for byte.
