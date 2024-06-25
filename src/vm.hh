#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct RegState {
    uint64_t data[2];

    bool operator==(const RegState &other) const;
    bool operator<(const RegState &other) const;
};

struct RegHash {
    size_t operator()(const RegState &key) const;
};

struct ExecutionCacheBlock {
  public:
    RegState          start;
    std::vector<bool> io{};
    RegState          end;
};

class OneBitMachine {
  public:
    OneBitMachine(
        std::vector<uint16_t>    &prog,
        std::function<bool()>     in,
        std::function<void(bool)> out
    );

    void go();

  protected:
    alignas(32) uint8_t regFile[128 + 15] = {0};

    std::vector<uint16_t>    &rom;
    std::function<bool()>     inCb;
    std::function<void(bool)> outCb;

    // TODO: pick one? performance seems to be about the same?
    std::unordered_map<RegState, ExecutionCacheBlock, RegHash> cache{};
    // std::map<RegState, ExecutionCacheBlock> cache{};

    std::optional<ExecutionCacheBlock> ecb{};

    RegState getState();

    void runCached(const ExecutionCacheBlock &c);

    void createEcb(RegState rs);
    void finishEcb(RegState rs);

    void icopy(uint8_t a, uint8_t b);
    void iload(uint8_t a, uint8_t b);
    void inand(uint8_t a, uint8_t b);
    void ixor(uint8_t a, uint8_t b);
};