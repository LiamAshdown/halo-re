exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())

BN = '''
extern unsigned int gt2_bignum_length;          // 0x00683944, bytes per big number
extern unsigned char gt2_bignum_modulus[0x400]; // 0x00723240
'''

emit(0x617db0, 505, 'gt2_bignum_mul_mod', 'a = a * b mod m for big-endian byte strings of gt2_bignum_length bytes (m the global modulus at 0x723240): shift-and-add over a copy of a (from its low bit) with a doubled copy of b, each partial sum and doubling reduced by one subtraction when not below m (the first differing byte of the first n-1 decides; the last byte is compared unsigned). Carries and borrows are arithmetic shifts.', BN + '''
void gt2_bignum_mul_mod(unsigned char *a, const unsigned char *b)
{
    unsigned char x[0x400];
    unsigned char y[0x400];
    unsigned int n = gt2_bignum_length;
    int bits;
    int carry;
    unsigned int i;
    int j;

    memcpy(x, a, n);
    memcpy(y, b, n);
    memset(a, 0, n);
    for (bits = (int)(gt2_bignum_length * 8); bits != 0; bits--) {
        n = gt2_bignum_length;
        if (x[n - 1] & 1) {
            carry = 0;
            for (i = n; i != 0; i--) {
                carry = a[i - 1] + carry + y[i - 1];
                a[i - 1] = (unsigned char)carry;
                carry >>= 8;
            }
            n = gt2_bignum_length;
            for (j = 0; j < (int)(n - 1) && a[j] == gt2_bignum_modulus[j]; j++) {
            }
            if (a[j] >= gt2_bignum_modulus[j]) {
                carry = 0;
                for (i = n; i != 0; i--) {
                    carry += a[i - 1] - gt2_bignum_modulus[i - 1];
                    a[i - 1] = (unsigned char)carry;
                    carry >>= 8;
                }
                n = gt2_bignum_length;
            }
        }
        carry = 0;
        for (j = 0; j < (int)n; j++) {
            carry |= x[j];
            x[j] = (unsigned char)(carry / 2);
            carry = (carry & 1) << 8;
        }
        carry = 0;
        for (i = n; i != 0; i--) {
            carry += y[i - 1] * 2;
            y[i - 1] = (unsigned char)carry;
            carry >>= 8;
        }
        for (j = 0; j < (int)(n - 1) && y[j] == gt2_bignum_modulus[j]; j++) {
        }
        if (y[j] >= gt2_bignum_modulus[j]) {
            carry = 0;
            for (i = n; i != 0; i--) {
                carry += y[i - 1] - gt2_bignum_modulus[i - 1];
                y[i - 1] = (unsigned char)carry;
                carry >>= 8;
            }
        }
    }
}
''', name_confidence='0.5')

emit(0x617fb0, 135, 'gt2_bignum_from_hex', 'zeroes the gt2_bignum_length-byte number, then for each character while it is positive (signed): shifts the number left four bits and ORs in the digit. The character is lower-cased IN PLACE (| 0x20) and its value is c - 0x30, less 0x27 more when above 0x60.', BN + '''
void gt2_bignum_from_hex(char *hex, unsigned char *number)
{
    int shift;
    int carry;
    unsigned int i;

    memset(number, 0, gt2_bignum_length);
    while (*hex > 0) {
        char c;

        for (shift = 4; shift != 0; shift--) {
            carry = 0;
            for (i = gt2_bignum_length; i != 0; i--) {
                carry += number[i - 1] * 2;
                number[i - 1] = (unsigned char)carry;
                carry >>= 8;
            }
        }
        c = (char)(*hex | 0x20);
        *hex = c;
        number[gt2_bignum_length - 1] |= (unsigned char)(c - (c > 0x60 ? 0x27 : 0) - 0x30);
        hex++;
    }
}
''', name_confidence='0.5')

emit(0x618040, 272, 'gt2_bignum_mod_exp', 'result = base ^ exponent mod modulus, all three given in hex (parsed with gt2_bignum_from_hex, the modulus into the global at 0x723240; the strings get lower-cased): result starts at 1, then for every bit from the exponent  low end: multiply in the base when set, square the base, shift the exponent right.', BN + '''
extern void gt2_bignum_mul_mod(unsigned char *a, const unsigned char *b);
extern void gt2_bignum_from_hex(char *hex, unsigned char *number);

void gt2_bignum_mod_exp(char *base_hex, char *exponent_hex, char *modulus_hex, unsigned char *result)
{
    unsigned char base[0x400];
    unsigned char exponent[0x400];
    unsigned int n;
    int bits;
    int carry;
    int j;

    gt2_bignum_from_hex(base_hex, base);
    gt2_bignum_from_hex(exponent_hex, exponent);
    gt2_bignum_from_hex(modulus_hex, gt2_bignum_modulus);
    memset(result, 0, gt2_bignum_length);
    result[gt2_bignum_length - 1] = 1;
    n = gt2_bignum_length;
    for (bits = (int)(gt2_bignum_length * 8); bits != 0; bits--) {
        if (exponent[n - 1] & 1) {
            gt2_bignum_mul_mod(result, base);
            n = gt2_bignum_length;
        }
        gt2_bignum_mul_mod(base, base);
        carry = 0;
        for (j = 0; j < (int)n; j++) {
            carry |= exponent[j];
            exponent[j] = (unsigned char)(carry / 2);
            carry = (carry & 1) << 8;
        }
    }
}
''', name_confidence='0.5')

emit(0x618150, 109, 'gt2_bignum_to_hex', 'upper-case hex of the number without its leading zero bytes (the skip is unbounded; an all-zero number gives ""), NUL terminated.', BN + '''
void gt2_bignum_to_hex(const unsigned char *number, char *hex)
{
    unsigned int i = 0;

    while (number[i] == 0) {
        i++;
    }
    for (; (int)i < (int)gt2_bignum_length; i++) {
        unsigned char high = (unsigned char)(number[i] >> 4);
        unsigned char low = (unsigned char)(number[i] & 0xf);

        *hex++ = (char)(high + (number[i] > 0x9f ? 7 : 0) + '0');
        *hex++ = (char)(low + (low > 9 ? 7 : 0) + '0');
    }
    *hex = 0;
}
''', name_confidence='0.5')

# ---- TEA encryption (the decrypt side is src/cseries/tea_decrypt_block.c / tea_decrypt_buffer.c)
def tea(addr, size, name, note, code):
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    open('src/cseries/%s.c' % name, 'w', encoding='utf-8').write(
        '// %s  (game library code; no C existed)\n// address 0x%x, size %d bytes\n'
        '// name confidence: 0.8   rewrite confidence: 0.85\n%s// blam-cc: cdecl\n\n#include "tags.h"\n\n%s'
        % (name, addr, size, wr, code.lstrip('\n')))


import textwrap
tea(0x6181c0, 142, 'tea_encrypt_block', 'TEA encryption of one 8-byte block: 32 rounds, sum += 0x9e3779b9; v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1); v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3). The inverse of tea_decrypt_block 0x6182b0.', '''
void tea_encrypt_block(uint32_t *block, const uint32_t *key)
{
    uint32_t v0 = block[0];
    uint32_t v1 = block[1];
    uint32_t sum = 0;
    int32_t round;

    for (round = 32; round != 0; round--) {
        sum += 0x9e3779b9;
        v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
        v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
    }
    block[0] = v0;
    block[1] = v1;
}
''')
tea(0x618250, 93, 'tea_encrypt_buffer', 'nothing for fewer than 8 bytes; every full 8-byte block from the start through tea_encrypt_block, then, when the length is not a multiple of 8 (signed remainder), the LAST 8 bytes again (overlapping the final full block). The inverse of tea_decrypt_buffer 0x618350.', '''
extern void tea_encrypt_block(uint32_t *block, const uint32_t *key); // 0x6181c0

void tea_encrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key)
{
    int32_t blocks;
    uint8_t *block;

    if (length < 8) {
        return;
    }
    block = data;
    for (blocks = length / 8; blocks > 0; blocks--) {
        tea_encrypt_block((uint32_t *)block, key);
        block += 8;
    }
    if (length % 8 != 0) {
        tea_encrypt_block((uint32_t *)(data + length - 8), key);
    }
}
''')

# ---- GT2 buffers (gt2Buffer.c): GTI2Buffer {buffer, size, len}
GB = '''
typedef struct GTI2Buffer {
    unsigned char *buffer;            // 0x0
    int size;                         // 0x4
    int len;                          // 0x8
} GTI2Buffer;
'''
emit(0x620520, 36, 'gti2AllocateBuffer', 'mallocs the buffer data; 0 when out of memory, else the size is set and 1.', GB + '''
int gti2AllocateBuffer(GTI2Buffer *buffer, int size)
{
    buffer->buffer = (unsigned char *)malloc(size);
    if (buffer->buffer == 0) {
        return 0;
    }
    buffer->size = size;
    return 1;
}
''')
emit(0x620550, 11, 'gti2GetBufferFreeSpace', 'size - len.', GB + '''
int gti2GetBufferFreeSpace(const GTI2Buffer *buffer)
{
    return buffer->size - buffer->len;
}
''')
emit(0x620560, 22, 'gti2BufferWriteByte', 'appends one byte.', GB + '''
void gti2BufferWriteByte(GTI2Buffer *buffer, unsigned char b)
{
    buffer->buffer[buffer->len] = b;
    buffer->len++;
}
''')
emit(0x620580, 35, 'gti2BufferWriteUShort', 'appends a short, high byte first.', GB + '''
void gti2BufferWriteUShort(GTI2Buffer *buffer, unsigned short s)
{
    buffer->buffer[buffer->len] = (unsigned char)(s >> 8);
    buffer->len++;
    buffer->buffer[buffer->len] = (unsigned char)s;
    buffer->len++;
}
''')
emit(0x6205b0, 82, 'gti2BufferWriteData', 'appends length bytes (strlen for -1); nothing for NULL data or 0 bytes.', GB + '''
void gti2BufferWriteData(GTI2Buffer *buffer, const unsigned char *data, int length)
{
    if (data == 0 || length == 0) {
        return;
    }
    if (length == -1) {
        length = (int)strlen((const char *)data);
    }
    memcpy(buffer->buffer + buffer->len, data, (unsigned int)length);
    buffer->len += length;
}
''')
emit(0x620610, 60, 'gti2BufferShorten', 'removes shortenBy bytes at start (-1: from the end) by moving the rest down.', GB + '''
void gti2BufferShorten(GTI2Buffer *buffer, int start, int shortenBy)
{
    if (start == -1) {
        start = buffer->len - shortenBy;
    }
    memmove(buffer->buffer + start, buffer->buffer + start + shortenBy, buffer->len - start - shortenBy);
    buffer->len -= shortenBy;
}
''')

# ---- GT2 challenge / response (gt2Auth.c)
AU = '''
static const char GT2ChallengeKey[] = "3b8dd8995f7c40a9a5c5b7dd5b481341"; // the string at 0x00683db0
'''
emit(0x620650, 105, 'gti2VerifyChallenge', 'EDI challenge (32 bytes): byte i (1..31) must have the parity of the running XOR of ((c[i-1] ^ c[0] ^ i) & 1) ^ (c[i-1] < c[0]) ^ (c[0] < 0x4f).', '''
int gti2VerifyChallenge(const unsigned char *challenge)
{
    unsigned char first = challenge[0];
    int parity = 0;
    int i;

    for (i = 1; i < 0x20; i++) {
        parity ^= ((challenge[i - 1] ^ first ^ i) & 1) ^ (challenge[i - 1] < first) ^ (first < 0x4f);
        if (parity != 0) {
            if ((challenge[i] & 1) == 0) {
                return 0;
            }
        } else if ((challenge[i] & 1) != 0) {
            return 0;
        }
    }
    return 1;
}
''', cc='EDI -> challenge')
emit(0x6206c0, 137, 'gti2GetChallenge', 'seeds rand with current_time, then 32 printable bytes (rand %% 0x5d + 0x21), each after the first bumped by one when its parity does not match the running check gti2VerifyChallenge applies.'.replace('%%', '%'), '''
unsigned char *gti2GetChallenge(unsigned char *challenge)
{
    int parity = 0;
    int i;

    srand(current_time());
    challenge[0] = (unsigned char)(rand() % 0x5d + 0x21);
    for (i = 1; i < 0x20; i++) {
        unsigned char c;

        parity ^= ((challenge[i - 1] ^ challenge[0] ^ i) & 1) ^ (challenge[i - 1] < challenge[0]) ^ (challenge[0] < 0x4f);
        c = (unsigned char)(rand() % 0x5d + 0x21);
        challenge[i] = c;
        if (parity != 0) {
            if ((c & 1) == 0) {
                challenge[i] = (unsigned char)(c + 1);
            }
        } else if ((c & 1) != 0) {
            challenge[i] = (unsigned char)(c + 1);
        }
    }
    return challenge;
}
''')
emit(0x620750, 259, 'gti2GetResponse', 'the 32-byte response to a challenge: bytes 0 and 13, and all of them when the challenge fails gti2VerifyChallenge (EDI), are random printable; the rest mix the challenge with the key string 0x683db0 (signed chars, % its length): idx = (key[(c[i] + i) % klen] + c[i] * i) & 31 (signed), v = c[idx] ^ key[(prev * i * 0x4647) % klen] where prev is c[i] for i 1 and 14 else c[i-1]; byte = |v| % 0x5d + 0x21.', AU + '''
extern int gti2VerifyChallenge(const unsigned char *challenge);

unsigned char *gti2GetResponse(unsigned char *response, const unsigned char *challenge)
{
    int key_length = (int)strlen(GT2ChallengeKey);
    int valid = gti2VerifyChallenge(challenge);
    int i;

    for (i = 0; i < 0x20; i++) {
        if (valid == 0 || i == 0 || i == 0xd) {
            response[i] = (unsigned char)(rand() % 0x5d + 0x21);
        } else {
            char previous;
            int index;
            int value;

            if (i == 1 || i == 0xe) {
                previous = (char)challenge[i];
            } else {
                previous = (char)challenge[i - 1];
            }
            index = ((int)GT2ChallengeKey[(int)(challenge[i] + i) % key_length] + (int)challenge[i] * i) % 0x20;
            value = (int)challenge[index] ^ (int)GT2ChallengeKey[(previous * i * 0x4647) % key_length];
            if (value < 0) {
                value = -value;
            }
            response[i] = (unsigned char)(value % 0x5d + 0x21);
        }
    }
    return response;
}
''')
emit(0x620860, 59, 'gti2CheckResponse', '1 when the responses match everywhere but bytes 0 and 13.', '''
int gti2CheckResponse(const unsigned char *response1, const unsigned char *response2)
{
    int i;

    for (i = 0; i < 0x20; i++) {
        if (i != 0 && i != 0xd && response1[i] != response2[i]) {
            return 0;
        }
    }
    return 1;
}
''')
print('ok')
