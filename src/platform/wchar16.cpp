/**
 * @file src/platform/wchar16.cpp
 * 16-bit wide string functions with MSVC semantics (include/halo/platform/wchar16.h).
 */

#include "halo/platform/wchar16.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace {

wchar_t lower(wchar_t c)
{
    return c >= L'A' && c <= L'Z' ? static_cast<wchar_t>(c + (L'a' - L'A')) : c;
}

/** Output with a capacity: keeps writing past it only to count, like vsnprintf. */
struct wide_out {
    wchar_t *text;
    size_t capacity;  // units including the terminator
    size_t length;

    void put(wchar_t c)
    {
        if (length + 1 < capacity) {
            text[length] = c;
        }
        length++;
    }
};

void put_padded_wide(wide_out &out, const wchar_t *text, size_t length, int width, bool left)
{
    for (int pad = width - static_cast<int>(length); !left && pad > 0; pad--) out.put(L' ');
    for (size_t i = 0; i < length; i++) out.put(text[i]);
    for (int pad = width - static_cast<int>(length); left && pad > 0; pad--) out.put(L' ');
}

void put_padded_narrow(wide_out &out, const char *text, size_t length, int width, bool left)
{
    for (int pad = width - static_cast<int>(length); !left && pad > 0; pad--) out.put(L' ');
    for (size_t i = 0; i < length; i++) out.put(static_cast<wchar_t>(static_cast<unsigned char>(text[i])));
    for (int pad = width - static_cast<int>(length); left && pad > 0; pad--) out.put(L' ');
}

}  // namespace

extern "C" {

size_t halo_wcslen(const wchar_t *text)
{
    size_t length = 0;

    while (text[length] != 0) length++;
    return length;
}

wchar_t *halo_wcscpy(wchar_t *destination, const wchar_t *source)
{
    size_t i = 0;

    do {
        destination[i] = source[i];
    } while (source[i++] != 0);
    return destination;
}

wchar_t *halo_wcsncpy(wchar_t *destination, const wchar_t *source, size_t count)
{
    size_t i = 0;

    for (; i < count && source[i] != 0; i++) destination[i] = source[i];
    for (; i < count; i++) destination[i] = 0;
    return destination;
}

wchar_t *halo_wcscat(wchar_t *destination, const wchar_t *source)
{
    halo_wcscpy(destination + halo_wcslen(destination), source);
    return destination;
}

wchar_t *halo_wcsncat(wchar_t *destination, const wchar_t *source, size_t count)
{
    wchar_t *end = destination + halo_wcslen(destination);
    size_t i = 0;

    for (; i < count && source[i] != 0; i++) end[i] = source[i];
    end[i] = 0;
    return destination;
}

int halo_wcscmp(const wchar_t *a, const wchar_t *b)
{
    return halo_wcsncmp(a, b, SIZE_MAX);
}

int halo_wcsncmp(const wchar_t *a, const wchar_t *b, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        uint16_t x = static_cast<uint16_t>(a[i]);
        uint16_t y = static_cast<uint16_t>(b[i]);

        if (x != y) return x < y ? -1 : 1;
        if (x == 0) return 0;
    }
    return 0;
}

int halo_wcsicmp(const wchar_t *a, const wchar_t *b)
{
    return halo_wcsnicmp(a, b, SIZE_MAX);
}

int halo_wcsnicmp(const wchar_t *a, const wchar_t *b, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        uint16_t x = static_cast<uint16_t>(lower(a[i]));
        uint16_t y = static_cast<uint16_t>(lower(b[i]));

        if (x != y) return static_cast<int>(x) - static_cast<int>(y);
        if (x == 0) return 0;
    }
    return 0;
}

wchar_t *halo_wcschr(const wchar_t *text, wchar_t character)
{
    for (;; text++) {
        if (*text == character) return const_cast<wchar_t *>(text);
        if (*text == 0) return nullptr;
    }
}

wchar_t *halo_wcsstr(const wchar_t *text, const wchar_t *pattern)
{
    size_t length = halo_wcslen(pattern);

    if (length == 0) return const_cast<wchar_t *>(text);
    for (; *text != 0; text++) {
        if (halo_wcsncmp(text, pattern, length) == 0) return const_cast<wchar_t *>(text);
    }
    return nullptr;
}

int halo_vswprintf(wchar_t *out_text, size_t count, const wchar_t *format, va_list args)
{
    wide_out out = {out_text, count, 0};

    while (*format != 0) {
        char spec[32];
        size_t spec_length = 0;
        bool left = false;
        int width = -1;
        int precision = -1;
        enum { none, h, l, ll } size = none;
        wchar_t conversion;

        if (*format != L'%') {
            out.put(*format++);
            continue;
        }
        format++;
        if (*format == L'%') {
            out.put(L'%');
            format++;
            continue;
        }
        spec[spec_length++] = '%';
        while (*format == L'-' || *format == L'+' || *format == L' ' || *format == L'#' || *format == L'0') {
            left = left || *format == L'-';
            spec[spec_length++] = static_cast<char>(*format++);
        }
        if (*format == L'*') {
            width = va_arg(args, int);
            if (width < 0) {
                left = true;
                width = -width;
                spec[spec_length++] = '-';
            }
            spec_length += static_cast<size_t>(snprintf(spec + spec_length, sizeof(spec) - spec_length, "%d", width));
            format++;
        } else if (*format >= L'0' && *format <= L'9') {
            width = 0;
            while (*format >= L'0' && *format <= L'9') {
                width = width * 10 + (*format - L'0');
                if (spec_length < 20) spec[spec_length++] = static_cast<char>(*format);
                format++;
            }
        }
        if (*format == L'.') {
            spec[spec_length++] = '.';
            format++;
            precision = 0;
            if (*format == L'*') {
                precision = va_arg(args, int);
                spec_length += static_cast<size_t>(snprintf(spec + spec_length, sizeof(spec) - spec_length, "%d", precision));
                format++;
            } else {
                while (*format >= L'0' && *format <= L'9') {
                    precision = precision * 10 + (*format - L'0');
                    if (spec_length < 26) spec[spec_length++] = static_cast<char>(*format);
                    format++;
                }
            }
        }
        if (*format == L'h') {
            size = h;
            format++;
        } else if (*format == L'l') {
            size = l;
            format++;
            if (*format == L'l') {
                size = ll;
                format++;
            }
        } else if (*format == L'w') {
            size = l;
            format++;
        } else if (format[0] == L'I' && format[1] == L'6' && format[2] == L'4') {
            size = ll;
            format += 3;
        } else if (*format == L'I' && format[1] == L'3' && format[2] == L'2') {
            format += 3;
        }
        conversion = *format;
        if (conversion == 0) {
            break;
        }
        format++;

        switch (conversion) {
        case L's':
        case L'S':
            // MSVC: %s / %ls / %ws wide, %S / %hs narrow
            if ((conversion == L's' && size != h) || (conversion == L'S' && size == l)) {
                const wchar_t *text = va_arg(args, const wchar_t *);
                size_t length;

                if (text == nullptr) text = L"(null)";
                length = halo_wcslen(text);
                if (precision >= 0 && static_cast<size_t>(precision) < length) length = static_cast<size_t>(precision);
                put_padded_wide(out, text, length, width, left);
            } else {
                const char *text = va_arg(args, const char *);
                size_t length;

                if (text == nullptr) text = "(null)";
                length = strlen(text);
                if (precision >= 0 && static_cast<size_t>(precision) < length) length = static_cast<size_t>(precision);
                put_padded_narrow(out, text, length, width, left);
            }
            break;
        case L'c':
        case L'C': {
            wchar_t c = static_cast<wchar_t>(va_arg(args, int));

            if ((conversion == L'c' && size == h) || (conversion == L'C' && size != l)) {
                c = static_cast<wchar_t>(static_cast<unsigned char>(c));
            }
            put_padded_wide(out, &c, 1, width, left);
            break;
        }
        default: {
            // numbers: format with the C library, then widen (the result is ASCII)
            char text[128];
            int length = 0;

            if (size == ll) {
                spec[spec_length++] = 'l';
                spec[spec_length++] = 'l';
            } else if (size == l) {
                spec[spec_length++] = 'l';
            } else if (size == h) {
                spec[spec_length++] = 'h';
            }
            spec[spec_length++] = static_cast<char>(conversion);
            spec[spec_length] = '\0';
            switch (conversion) {
            case L'd': case L'i':
                length = size == ll ? snprintf(text, sizeof(text), spec, va_arg(args, long long))
                       : size == l ? snprintf(text, sizeof(text), spec, va_arg(args, long))
                                   : snprintf(text, sizeof(text), spec, va_arg(args, int));
                break;
            case L'u': case L'x': case L'X': case L'o':
                length = size == ll ? snprintf(text, sizeof(text), spec, va_arg(args, unsigned long long))
                       : size == l ? snprintf(text, sizeof(text), spec, va_arg(args, unsigned long))
                                   : snprintf(text, sizeof(text), spec, va_arg(args, unsigned int));
                break;
            case L'f': case L'F': case L'e': case L'E': case L'g': case L'G': case L'a': case L'A':
                spec_length -= size == ll ? 2 : size != none ? 1 : 0;  // no length modifier on doubles
                spec[spec_length - 1] = static_cast<char>(conversion);
                spec[spec_length] = '\0';
                length = snprintf(text, sizeof(text), spec, va_arg(args, double));
                break;
            case L'p':
                length = snprintf(text, sizeof(text), "%p", va_arg(args, void *));
                break;
            default:
                break;  // unknown conversion: nothing printed
            }
            if (length > static_cast<int>(sizeof(text)) - 1) length = static_cast<int>(sizeof(text)) - 1;
            for (int i = 0; i < length; i++) out.put(static_cast<wchar_t>(static_cast<unsigned char>(text[i])));
            break;
        }
        }
    }
    if (count != 0) {
        out_text[out.length < count ? out.length : count - 1] = 0;
    }
    return out.length < count ? static_cast<int>(out.length) : -1;
}

int halo_swprintf(wchar_t *out, size_t count, const wchar_t *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = halo_vswprintf(out, count, format, args);
    va_end(args);
    return result;
}

int halo_wprintf(const wchar_t *format, ...)
{
    wchar_t wide[1024];
    char text[3072];
    size_t length = 0;
    va_list args;
    int result;

    va_start(args, format);
    result = halo_vswprintf(wide, sizeof(wide) / sizeof(wide[0]), format, args);
    va_end(args);
    for (size_t i = 0; wide[i] != 0 && length + 4 < sizeof(text); i++) {
        uint32_t c = static_cast<uint16_t>(wide[i]);  // UTF-16 unit to UTF-8 (surrogates pass through as 3-byte units)

        if (c < 0x80) {
            text[length++] = static_cast<char>(c);
        } else if (c < 0x800) {
            text[length++] = static_cast<char>(0xc0 | (c >> 6));
            text[length++] = static_cast<char>(0x80 | (c & 0x3f));
        } else {
            text[length++] = static_cast<char>(0xe0 | (c >> 12));
            text[length++] = static_cast<char>(0x80 | ((c >> 6) & 0x3f));
            text[length++] = static_cast<char>(0x80 | (c & 0x3f));
        }
    }
    fwrite(text, 1, length, stdout);
    return result;
}

}  // extern "C"
