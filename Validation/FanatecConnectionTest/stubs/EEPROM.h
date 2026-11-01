#pragma once
#include <array>
#include <cassert>
#include <cstdint>
struct FakeEEPROM {
    std::array<uint8_t, 512> bytes{};
    size_t size = 512;
    size_t length() const { return size; }
    uint8_t read(size_t address) const { assert(address < size); return bytes[address]; }
    void write(size_t address, uint8_t value) { assert(address < size); bytes[address] = value; }
    bool commit() { return true; }
};
extern FakeEEPROM EEPROM;
