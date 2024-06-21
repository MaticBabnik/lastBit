#include <cstdint>
#include <immintrin.h>

// https://stackoverflow.com/a/21673221
static inline __m256i get_mask3(const uint32_t mask) {
    __m256i       vmask(_mm256_set1_epi32(mask));
    const __m256i shuffle(_mm256_setr_epi64x(
        0x0000000000000000,
        0x0101010101010101,
        0x0202020202020202,
        0x0303030303030303
    ));
    vmask = _mm256_shuffle_epi8(vmask, shuffle);
    const __m256i bit_mask(_mm256_set1_epi64x(0x7fbfdfeff7fbfdfe));
    vmask = _mm256_or_si256(vmask, bit_mask);
    return _mm256_cmpeq_epi8(vmask, _mm256_set1_epi64x(-1));
}

/*
Unlike the 16b variant this one doesnt flip the order
*/
static inline uint32_t pack32(uint8_t *p) {
    auto vec32 = _mm256_loadu_si256((__m256i *)p);

    vec32 = _mm256_slli_epi16(vec32, 7);

    return _mm256_movemask_epi8(vec32);
}

/*
Unlike the 16b variant this one doesnt flip the order.

While pack32 might be decent, this one isn't. We could
probably save the shift right op, but I CBA
*/
static inline void unpack32(uint8_t *p, uint32_t s) {
    auto vec32 = get_mask3(s);

    vec32 = _mm256_srli_epi16(vec32, 7);

    _mm256_store_si256((__m256i *)p, vec32);
}

static inline uint16_t pack16(uint8_t *p) {
    // This actually reverses bytes in a vector, trust me
    const __m128i shuffle_reverse =
        _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

    __m128i vec16 = _mm_loadu_si128((__m128i *)p);

    // Shift the bits up, so movemask sees them
    vec16 = _mm_slli_epi16(vec16, 7);

    // flip order of bytes (later bits)
    vec16 = _mm_shuffle_epi8(vec16, shuffle_reverse);

    return _mm_movemask_epi8(vec16); // extract u16
}

static inline void unpack16(uint8_t *p, uint16_t data) {
    const __m128i extract_bits = _mm_set_epi8(
        0x1,
        0x2,
        0x4,
        0x8,
        0x10,
        0x20,
        0x40,
        0x80,
        0x1,
        0x2,
        0x4,
        0x8,
        0x10,
        0x20,
        0x40,
        0x80
    );

    uint8_t b = data, t = data >> 8; // split u16

    // load top and bottom half
    __m128i vec = _mm_set_epi8(b, b, b, b, b, b, b, b, t, t, t, t, t, t, t, t);
    // check if respective bit is set
    vec = _mm_min_epu8(_mm_and_si128(vec, extract_bits), _mm_set1_epi8(1));
    // store
    _mm_storeu_si128((__m128i *)p, vec);
}

static inline void copy16(uint8_t *dst, uint8_t *src) {
    _mm_storeu_si128((__m128i *)dst, _mm_loadu_si128((__m128i *)src));
}