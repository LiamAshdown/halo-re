/**
 * @file src/shell/crash_reporter_posix.cpp
 * The crash reporter off Windows: there is no Windows Error Reporting, and the guarded blocks that would hand it a
 * fault (include/halo/platform/fault.hpp) run unguarded, so it only answers "handle it" if ever asked.
 */

#include "halo/shell/diagnostics.hpp"

namespace halo::shell {

namespace {

class NullCrashReporter final : public CrashReporter {
public:
    int32_t handle_exception(win32_exception_pointers *) const override
    {
        return 1;  // EXCEPTION_EXECUTE_HANDLER
    }
};

constexpr NullCrashReporter k_null_crash_reporter{};

}  // namespace

const CrashReporter &CrashReporter::current()
{
    return k_null_crash_reporter;
}

}  // namespace halo::shell
