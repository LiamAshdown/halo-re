// Checks src/platform/wchar16.cpp against the MSVC C library it stands in for (wchar_t is 16-bit there).
//   cmake --build build/cxx --config Release --target wchar16_test && build\cxx\Release\wchar16_test.exe
#include "halo/platform/wchar16.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

static int failures;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            printf("FAIL line %d: %s\n", __LINE__, #condition);              \
            failures++;                                                      \
        }                                                                    \
    } while (0)

static int sign(int x) { return x < 0 ? -1 : x > 0 ? 1 : 0; }

template <typename... Args>
static void check_format(const wchar_t *format, Args... args)
{
    wchar_t expected[256];
    wchar_t actual[256];
    int expected_result = swprintf(expected, 256, format, args...);
    int actual_result = halo_swprintf(actual, 256, format, args...);

    if (expected_result != actual_result || wcscmp(expected, actual) != 0) {
        printf("FAIL format \"%ls\": MSVC %d \"%ls\", ours %d \"%ls\"\n", format, expected_result, expected, actual_result, actual);
        failures++;
    }
}

int main()
{
    wchar_t a[64], b[64];
    const wchar_t *text = L"Blood Gulch";

    CHECK(halo_wcslen(text) == wcslen(text));
    CHECK(halo_wcslen(L"") == 0);
    CHECK(wcscmp(halo_wcscpy(a, text), text) == 0);
    wmemset(a, L'x', 64);
    wmemset(b, L'x', 64);
    halo_wcsncpy(a, text, 20);
    wcsncpy(b, text, 20);
    CHECK(memcmp(a, b, sizeof(a)) == 0);
    halo_wcsncpy(a, text, 5);
    wcsncpy(b, text, 5);
    CHECK(memcmp(a, b, sizeof(a)) == 0);
    wcscpy(a, L"Red ");
    wcscpy(b, L"Red ");
    CHECK(wcscmp(halo_wcscat(a, L"Team"), wcscat(b, L"Team")) == 0);
    CHECK(wcscmp(halo_wcsncat(a, L"12345", 2), wcsncat(b, L"12345", 2)) == 0);
    CHECK(sign(halo_wcscmp(L"abc", L"abd")) == sign(wcscmp(L"abc", L"abd")));
    CHECK(sign(halo_wcscmp(L"abc", L"ab")) == sign(wcscmp(L"abc", L"ab")));
    CHECK(halo_wcscmp(L"abc", L"abc") == 0);
    CHECK(sign(halo_wcscmp(L"\x00e9", L"z")) == sign(wcscmp(L"\x00e9", L"z")));
    CHECK(sign(halo_wcsncmp(L"abcx", L"abcy", 3)) == sign(wcsncmp(L"abcx", L"abcy", 3)));
    CHECK(sign(halo_wcsicmp(L"Blood", L"bLOOD")) == sign(_wcsicmp(L"Blood", L"bLOOD")));
    CHECK(sign(halo_wcsicmp(L"Alpha", L"beta")) == sign(_wcsicmp(L"Alpha", L"beta")));
    CHECK(sign(halo_wcsnicmp(L"PROMPT_a", L"prompt_b", 7)) == sign(_wcsnicmp(L"PROMPT_a", L"prompt_b", 7)));
    CHECK(sign(halo_wcsnicmp(L"PROMPT_a", L"prompt_b", 8)) == sign(_wcsnicmp(L"PROMPT_a", L"prompt_b", 8)));
    CHECK(halo_wcschr(text, L'G') == wcschr(text, L'G'));
    CHECK(halo_wcschr(text, L'Q') == nullptr);
    CHECK(halo_wcschr(text, 0) == text + wcslen(text));
    CHECK(halo_wcsstr(text, L"Gul") == wcsstr(text, L"Gul"));
    CHECK(halo_wcsstr(text, L"gul") == nullptr);

    check_format(L"plain");
    check_format(L"(%s)", L"label");
    check_format(L"(%s - \"%s\")", L"a", L"b");
    check_format(L"%s%s %d", L"|", L"Slayer", 25);
    check_format(L"%S/%hs", "narrow", "also");
    check_format(L"%ls|%ws", L"wide", L"too");
    check_format(L"[%10s][%-10s][%.3s]", L"right", L"left", L"truncate");
    check_format(L"[%5d][%-5d][%05d][%+d][%x][%X][%u]", 42, 42, 42, 42, 0xbeef, 0xbeef, 4000000000u);
    check_format(L"%ld %lu %I64d %lld", 123456L, 654321UL, -1234567890123LL, 9876543210LL);
    check_format(L"%f %.2f %e %g %8.3f", 3.5, 2.0 / 3.0, 12345.678, 0.0001, -1.25);
    check_format(L"%c%c%C", L'A', 0x00e9, 'z');
    check_format(L"100%% %d%%", 7);
    check_format(L"%*d|%-*d|%.*s", 6, 12, 4, 3, 2, L"abcdef");
    {
        wchar_t small[8];
        int ours = halo_swprintf(small, 8, L"%s", L"far too long");

        CHECK(ours < 0);
        CHECK(small[7] == 0);
    }
    printf(failures == 0 ? "wchar16: all checks passed\n" : "wchar16: %d failures\n", failures);
    return failures != 0;
}
