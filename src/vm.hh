#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

class OneBitMachine {
  public:
    OneBitMachine(
        std::vector<uint16_t>    &prog,
        std::function<bool()>     in,
        std::function<void(bool)> out
    );

    void go();

  protected:
    uint8_t                   regFile[128 + 15] = {0};
    std::vector<uint16_t>    &rom;
    std::function<bool()>     inCb;
    std::function<void(bool)> outCb;

    void icopy(uint8_t a, uint8_t b);
    void iload(uint8_t a, uint8_t b);
    void inand(uint8_t a, uint8_t b);
    void ixor(uint8_t a, uint8_t b);

    // todo: cache
    // todo: ecb
};