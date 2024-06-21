#include "vm.hh"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std; // fuckyou

constexpr auto MEM_SIZE = 0xffff;

uint16_t swe(uint16_t x) { return (x >> 8) | (x << 8); }

uint loadProg(std::vector<uint16_t> &memory, string path) {
    ifstream prog(path, ios::binary);

    prog.read((char *)memory.data(), MEM_SIZE);
    auto n = prog.gcount();

    for (int i = 0; i < n; i++) {
        memory[i] = swe(memory[i]); // real
    }

    return n;
}

int main(int argc, char *argv[]) {
    vector<uint16_t> memory(MEM_SIZE / 2);

    if (argc != 2) {
        cerr << "lastBit <prog>" << endl;
        return -1;
    }
    loadProg(memory, argv[1]);

    char          charBuf  = 0;
    uint          bitCount = 0;
    OneBitMachine vm(
        memory,
        []() { return 0; },
        [&charBuf, &bitCount](bool v) {
            charBuf = (charBuf << 1) | v;
            ++bitCount;
            if (bitCount == 8) {
                cout << charBuf;
                charBuf  = 0;
                bitCount = 0;
            }
        }
    );

    for (;;) {
        vm.go();
    }
}