#include "vm.hh"
#include <immintrin.h>

using namespace std;

constexpr auto IN_REG    = 16;
constexpr auto IN_REG_A  = 17;
constexpr auto OUT_REG   = 18;
constexpr auto OUT_REG_A = 19;

uint16_t get16(uint8_t *p) {
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

void set16(uint8_t *p, uint16_t data) {
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

OneBitMachine::OneBitMachine(
    vector<uint16_t>    &prog,
    function<bool()>     in,
    function<void(bool)> out
)
    : inCb(in), outCb(out), rom(prog) {
    regFile[IN_REG_A] = 1;
    regFile[IN_REG]   = inCb() ? 1 : 0;
}

void OneBitMachine::icopy(uint8_t a, uint8_t b) {
    _mm_storeu_si128(
        (__m128i *)(this->regFile + b),
        _mm_loadu_si128((__m128i *)(regFile + a))
    );
    // set16(this->regFile + b, get16(this->regFile + a));
}

void OneBitMachine::iload(uint8_t a, uint8_t b) {
    set16(this->regFile + b, this->rom[a]);
}

void OneBitMachine::inand(uint8_t a, uint8_t b) {
    this->regFile[b] = 1 & ~(this->regFile[a] & this->regFile[b]);
}

void OneBitMachine::ixor(uint8_t a, uint8_t b) {
    this->regFile[b] = 1 & (this->regFile[a] ^ this->regFile[b]);
}

const char *hexab = "0123456789ABCDEF";

void debugState(uint8_t w[128]) {
    for (int i = 0; i < 32; i++) {
        uint8_t hc = 0;
        for (int j = 0; j < 4; j++) {
            hc <<= 1;
            hc |= w[4 * i + j];
        }
        cout << hexab[hc];
    }
    cout<<endl;
}

void OneBitMachine::go() {
    // increment PC
    auto oldPc = get16(this->regFile);
    set16(this->regFile, oldPc + 1);

    // decode
    auto inst = this->rom[oldPc];
    auto op   = inst & 0x3;
    auto a1   = (inst >> 9) & 0x7f;
    auto a2   = (inst >> 2) & 0x7f;

    // exec
    switch (op) {
    case 0:
        icopy(a1, a2);
        break;
    case 1:
        iload(a1, a2);
        break;
    case 2:
        inand(a1, a2);
        break;
    case 3:
        ixor(a1, a2);
        break;
    }

    auto newPc = get16(this->regFile);

    if (oldPc == newPc) {
        cout << "HALT" << endl;
        exit(0);
    }

    // handle IO
    if (this->regFile[OUT_REG_A]) {
        this->outCb(this->regFile[OUT_REG]);
        this->regFile[OUT_REG_A] = 0;
    }

    if (!this->regFile[IN_REG_A]) {
        this->regFile[IN_REG]   = inCb();
        this->regFile[IN_REG_A] = 1;
    }
}