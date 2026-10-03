#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
constexpr int INPUT_PULLDOWN = 0;
constexpr int SERIAL_8N1 = 0;
constexpr int HEX = 16;
extern unsigned long testMillis;
extern bool testPlug;
inline unsigned long millis() { return testMillis; }
inline int digitalRead(int) { return testPlug; }
inline void pinMode(int, int) {}
inline long map(long x, long a, long b, long c, long d) {
    return a == b ? -1 : (x-a)*(d-c)/(b-a)+c;
}
