#include "FanatecInterface.h"
#include "EEPROM.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <limits>
unsigned long testMillis=0;
bool testPlug=false;
HardwareSerial Serial, Serial1;
FakeEEPROM EEPROM;
static FanatecInterface* current;
static std::vector<bool> callbacks;
static const uint8_t finalRequest[]={
    0x7B,2,0xFF,0,0,0,0,0,0,0,0x26,0x7D,
    0x7B,0,0,0,0,0,0,0,0,0,0xAA,0x7D,
    0x7B,3,0,0,0,0,0,0,0,0,0x5F,0x7D};
static void receive(std::initializer_list<uint8_t> bytes) {
    Serial1.rx.insert(Serial1.rx.end(),bytes.begin(),bytes.end());
}
static void tick(FanatecInterface& f, unsigned long elapsed=10) {
    testMillis+=elapsed;
    f.communicationUpdate();
}
static void plug(FanatecInterface& f, bool state) {
    testPlug=state; tick(f); tick(f,30);
}
static void start(FanatecInterface& f) {
    testMillis=0; testPlug=false; Serial1=HardwareSerial{};
    callbacks.clear(); current=&f;
    f.onConnected([](bool state){
        assert(current->isConnected()==state);
        callbacks.push_back(state);
    });
    f.begin();
}
static void handshake(FanatecInterface& f) {
    assert(Serial1.baud==250000);
    receive({0x99,0x0A}); tick(f);
    assert(Serial1.tx.back()==0x1A);
    receive({0x05}); tick(f);
    assert(Serial1.tx.back()==0x15);
    assert(Serial1.txBauds.back()==250000);
    assert(Serial1.baud==115200);
    // Arbitrary UART fragmentation must not prevent the 36-byte match.
    for(auto b:finalRequest) { receive({b}); tick(f); }
    assert(f.isConnected());
    assert(Serial1.tx.size()>=38);
    assert(std::vector<uint8_t>(Serial1.tx.end()-36,Serial1.tx.end())==
           std::vector<uint8_t>(finalRequest,finalRequest+36));
}
int main() {
    {
        FanatecInterface f(18,17,16);
        // Fanatec must use setup's allocation, without touching MAC/pairing bytes.
        for(size_t i=0;i<EEPROM.bytes.size();++i) EEPROM.bytes[i]=uint8_t(i);
        const auto before=EEPROM.bytes;
        start(f); plug(f,true);
        f.update(); assert(Serial1.tx.empty()); // no pedal stream before handshake
        handshake(f);
        assert(f.setVibrationEnabled(true));
        for(size_t i=0;i<EEPROM.bytes.size();++i) {
            if(i!=64 && i!=65 && i!=128 && i!=129) assert(EEPROM.bytes[i]==before[i]);
        }
        // These are the original v3 constant packets, including their CRCs.
        receive({0x7B,0,0xFF,0,1}); f.update(); assert(f.throttleVibration()==0);
        receive({0,0,0,0,0,0x40,0x7D}); f.update(); assert(f.throttleVibration()==255);
        tick(f,200);
        receive({0x7B,0,0,0xFF,1,0,0,0,0,0,0x9B,0x7D}); f.update();
        assert(f.throttleVibration()==255 && f.brakeVibration()==255);
        tick(f,301); assert(f.throttleVibration()==0 && f.brakeVibration()==255);
        assert(f.setVibrationEnabled(false)); assert(f.brakeVibration()==0);
        assert(f.setVibrationEnabled(true));
        // Handbrake calibration commands must have no EEPROM effect in this port.
        const auto configured=EEPROM.bytes;
        f.setHandbrake(1234);
        receive({0x7B,3,1,3,0,0,0,0,0,0,0x5B,0x7D}); f.update();
        receive({0x7B,3,1,3,1,0,0,0,0,0,0x6C,0x7D}); f.update();
        assert(EEPROM.bytes==configured);
        assert(Serial1.tx[Serial1.tx.size()-4]==(1234 & 255));
        assert(Serial1.tx[Serial1.tx.size()-3]==(1234 >> 8));
        FanatecInterface rebooted(18,17,16); start(rebooted); assert(rebooted.vibrationEnabled());
    }

    {
        FanatecInterface f(18,17,16); start(f);
        for(int i=0;i<100;i++) tick(f,100);
        assert(Serial1.tx.empty()); assert(!f.isConnected());
        // A request arriving during insertion debounce must be retained.
        testPlug=true; receive({0x0A}); tick(f); assert(Serial1.tx.empty());
        tick(f,30); assert(Serial1.tx.back()==0x1A);
        receive({0x0A}); tick(f); // wheelbase retries first request
        assert(Serial1.tx.back()==0x1A);
        receive({0x05}); tick(f);
        for(auto b:finalRequest) { receive({b}); tick(f); }
        assert(f.isConnected());
        // A brief contact bounce does not generate disconnection callbacks.
        testPlug=false; tick(f); testPlug=true; tick(f);
        assert(f.isConnected()); assert(callbacks.size()==1);
        plug(f,false); assert(!f.isConnected()); assert(Serial1.baud==250000);
        auto count=Serial1.tx.size(); f.update(); assert(Serial1.tx.size()==count);
        plug(f,true); handshake(f); assert(callbacks==std::vector<bool>({true,false,true}));
        // A queued burst must not overrun update()'s local buffer.
        for(int i=0;i<200;i++) Serial1.rx.push_back(0x55);
        f.update(); assert(Serial1.rx.size()==152);
    }
    for(int stage=0;stage<3;stage++) {
        FanatecInterface f(18,17,16); start(f); plug(f,true);
        if(stage>=1) { receive({0x0A}); tick(f); }
        if(stage>=2) { receive({0x05,0x7B,0x02}); tick(f); }
        plug(f,false); assert(!f.isConnected()); assert(Serial1.baud==250000);
        assert(callbacks.empty()); plug(f,true); handshake(f);
    }
    {
        FanatecInterface f(18,17,16); start(f); plug(f,true);
        receive({0x0A,0x05}); tick(f); assert(Serial1.baud==115200);
        receive({0x7B,0x02}); tick(f); tick(f,2000);
        assert(Serial1.baud==250000); assert(!f.isConnected());
        // Silent wheelbase retries return immediately and later requests work.
        for(int i=0;i<10;i++) tick(f,2001);
        handshake(f);
    }
    {
        FanatecInterface f(18,17,16); start(f);
        testMillis=std::numeric_limits<unsigned long>::max()-15;
        plug(f,true); handshake(f);
    }
    puts("PASS: late insertion, bounce, unplug at every handshake stage, reconnect, timeout recovery, fragmented replies, bounded RX");
}
