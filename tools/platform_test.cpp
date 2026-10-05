// Checks the portable platform pieces against the Windows originals they stand in for: src/platform/wchar16.cpp
// against the MSVC C library (wchar_t is 16-bit there), SHA-1 against known digests, and Windows-1252 conversion
// against MultiByteToWideChar / WideCharToMultiByte.
//   cmake --build build/cxx --config Release --target platform_test && build\cxx\Release\platform_test.exe
#include "halo/platform/wchar16.h"
#include "system_portable.hpp"

#include <windows.h>
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

static int call_vsnwprintf(int (*function)(wchar_t *, size_t, const wchar_t *, va_list), wchar_t *out, size_t count, const wchar_t *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = function(out, count, format, args);
    va_end(args);
    return result;
}

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
        wchar_t tiny[8];
        int ours = halo_swprintf(tiny, 8, L"%s", L"far too long");

        CHECK(ours < 0);
        CHECK(tiny[7] == 0);
    }
    {
        // _vsnwprintf: no terminator when the text exactly fills the buffer, -1 when it does not fit
        static const wchar_t *const texts[] = {L"fits", L"exactly8", L"far too long"};

        for (const wchar_t *text : texts) {
            wchar_t ours[8], theirs[8];
            int ours_result, theirs_result;

            wmemset(ours, L'#', 8);
            wmemset(theirs, L'#', 8);
            ours_result = call_vsnwprintf(halo_vsnwprintf, ours, 8, L"%s", text);
            theirs_result = call_vsnwprintf(_vsnwprintf, theirs, 8, L"%s", text);
            if (ours_result != theirs_result || memcmp(ours, theirs, sizeof(ours)) != 0) {
                printf("FAIL _vsnwprintf(\"%ls\"): MSVC %d, ours %d\n", text, theirs_result, ours_result);
                failures++;
            }
        }
        CHECK(halo_wtol(L"  -1234x") == _wtol(L"  -1234x"));
        CHECK(halo_wtol(L"+77") == _wtol(L"+77"));
        CHECK(halo_wtol(L"abc") == _wtol(L"abc"));
    }
    {
        // SHA-1: FIPS 180 test vectors, plus one input that needs a second padding block
        static const struct { const char *text; const char *digest; } vectors[] = {
            {"abc", "a9993e364706816aba3e25717850c26c9cd0d89d"},
            {"", "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
            {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "84983e441c3bd26ebaae4aa1f95129e5e54670f1"},
            {"The quick brown fox jumps over the lazy dog", "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"},
        };

        for (const auto &v : vectors) {
            uint8_t digest[20];
            char hex[41];

            halo::platform::portable::sha1(reinterpret_cast<const uint8_t *>(v.text), static_cast<uint32_t>(strlen(v.text)), digest);
            for (int i = 0; i < 20; i++) sprintf(hex + i * 2, "%02x", digest[i]);
            if (strcmp(hex, v.digest) != 0) {
                printf("FAIL sha1(\"%s\") = %s\n", v.text, hex);
                failures++;
            }
        }
    }
    {
        // Windows-1252 against the Windows conversions, every byte value
        char bytes[256];
        uint16_t ours[256];
        wchar_t theirs[256];
        char back_ours[256];
        char back_theirs[256];

        for (int i = 0; i < 255; i++) bytes[i] = static_cast<char>(i + 1);
        bytes[255] = 0;
        CHECK(halo::platform::portable::ansi_to_wide(bytes, -1, ours, 256) == MultiByteToWideChar(1252, 0, bytes, -1, theirs, 256));
        CHECK(memcmp(ours, theirs, sizeof(ours)) == 0);
        CHECK(halo::platform::portable::ansi_to_wide(bytes, -1, nullptr, 0) == 256);
        CHECK(halo::platform::portable::wide_to_ansi(ours, -1, back_ours, 256) ==
              WideCharToMultiByte(1252, 0, theirs, -1, back_theirs, 256, nullptr, nullptr));
        CHECK(memcmp(back_ours, back_theirs, sizeof(back_ours)) == 0);
        {
            const uint16_t unmappable[3] = {0x4e2d, 0x0041, 0};  // a CJK character has no Windows-1252 byte
            char a[3], b[3];

            halo::platform::portable::wide_to_ansi(unmappable, -1, a, 3);
            WideCharToMultiByte(1252, 0, reinterpret_cast<const wchar_t *>(unmappable), -1, b, 3, nullptr, nullptr);
            CHECK(memcmp(a, b, 3) == 0);
        }
    }
    printf(failures == 0 ? "platform_test: all checks passed\n" : "platform_test: %d failures\n", failures);
    return failures != 0;
}
