#pragma once
#include "Arduino.h"
#include <deque>
#include <vector>
struct HardwareSerial {
    unsigned long baud = 0;
    std::deque<uint8_t> rx;
    std::vector<uint8_t> tx;
    std::vector<unsigned long> txBauds;
    void begin(unsigned long rate, int, int, int) { baud = rate; }
    int available() { return static_cast<int>(rx.size()); }
    int read() { auto b=rx.front(); rx.pop_front(); return b; }
    void write(const uint8_t* p, size_t n) {
        tx.insert(tx.end(), p, p+n);
        txBauds.insert(txBauds.end(), n, baud);
    }
    void flush() {}
    void updateBaudRate(unsigned long rate) { baud=rate; }
    template<class T> void print(T) {}
    template<class T> void print(T, int) {}
    template<class T> void println(T) {}
    void println() {}
};
extern HardwareSerial Serial, Serial1;
