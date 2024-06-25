#include "vm.hh"
#include "simd.hh"

using namespace std;

constexpr auto IN_REG    = 16;
constexpr auto IN_REG_A  = 17;
constexpr auto OUT_REG   = 18;
constexpr auto OUT_REG_A = 19;

const char *hexab = "0123456789ABCDEF";
void        debugState(uint8_t w[128]) {
    for (int i = 0; i < 32; i++) {
        uint8_t hc = 0;
        for (int j = 0; j < 4; j++) {
            hc <<= 1;
            hc |= w[4 * i + j];
        }
        cout << hexab[hc];
    }
    cout << endl;
}

bool RegState::operator==(const RegState &other) const {
    return data[0] == other.data[0] && data[1] == other.data[1];
}

bool RegState::operator<(const RegState &other) const {
    if (data[0] == other.data[0]) {
        return data[1] < other.data[1];
    }
    return (data[0] < other.data[0]);
}

size_t RegHash::operator()(const RegState &key) const {
    return std::hash<uint64_t>()(key.data[0])
           ^ (std::hash<uint64_t>()(key.data[1]) << 1);
}

OneBitMachine::OneBitMachine(
    vector<uint16_t>    &prog,
    function<bool()>     in,
    function<void(bool)> out
)
    : inCb(in), outCb(out), rom(prog) {
    regFile[IN_REG_A] = 1;
    regFile[IN_REG]   = inCb() ? 1 : 0;

    createEcb(getState());
}

void OneBitMachine::icopy(uint8_t a, uint8_t b) {
    copy16(this->regFile + b, regFile + a);
}

void OneBitMachine::iload(uint8_t a, uint8_t b) {
    unpack16(this->regFile + b, this->rom[a]);
}

void OneBitMachine::inand(uint8_t a, uint8_t b) {
    this->regFile[b] = 1 & ~(this->regFile[a] & this->regFile[b]);
}

void OneBitMachine::ixor(uint8_t a, uint8_t b) {
    this->regFile[b] = 1 & (this->regFile[a] ^ this->regFile[b]);
}

void OneBitMachine::go() {
    if (!this->ecb.has_value()) {
        auto it = cache.find(getState());
        if (it != cache.end()) {
            runCached(it->second);
            return;
        }
    }

    // increment PC
    auto oldPc = pack16(this->regFile);
    unpack16(this->regFile, oldPc + 1);

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

    auto newPc = pack16(this->regFile);

    if (oldPc == newPc) {
        cout << "HALT" << endl;
        exit(0);
    }

    // handle IO
    if (this->regFile[OUT_REG_A]) {
        this->outCb(this->regFile[OUT_REG]);
        if (this->ecb.has_value()) {
            this->ecb->io.emplace_back(this->regFile[OUT_REG]);
        }
        this->regFile[OUT_REG_A] = 0;
    }
    if (!this->regFile[IN_REG_A]) {
        this->regFile[IN_REG]   = inCb();
        this->regFile[IN_REG_A] = 1;
        // todo: is this even cachable?
    }

    if (oldPc + 1 != newPc) {
        auto newState = getState();

        finishEcb(newState); // finish current ecb (if it)

        if (cache.contains(newState)) {
            this->ecb.reset();
        } else {
            createEcb(newState);
        }
    }
}

void OneBitMachine::runCached(const ExecutionCacheBlock &c) {
    for (bool b : c.io) {
        outCb(b);
    }

    auto w = c.end.data[0];
    unpack32(regFile, w);
    unpack32(regFile + 32, w >> 32);

    w = c.end.data[1];
    unpack32(regFile + 64, w);
    unpack32(regFile + 96, w >> 32);
}

RegState OneBitMachine::getState() {
    RegState o;
    o.data[0] =
        ((uint64_t)pack32(regFile + 32) << 32) | (uint64_t)pack32(regFile);
    o.data[1] =
        ((uint64_t)pack32(regFile + 96) << 32) | (uint64_t)pack32(regFile + 64);
    return o;
}

void OneBitMachine::createEcb(RegState rs) {
    ecb.emplace();
    ecb->start = getState();
}

void OneBitMachine::finishEcb(RegState rs) {
    if (!ecb.has_value()) return;

    ecb->end = rs;

    this->cache.emplace(ecb->start, ecb.value());
}
