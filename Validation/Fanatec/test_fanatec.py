#!/usr/bin/env python3
"""Host regression tests for the root Fanatec parser and action conversion."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="fanatec-tests-") as tmp:
    path = Path(tmp)
    (path / "Arduino.h").write_text(r'''
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <deque>
#include <vector>
#include <cassert>
#define SERIAL_8N1 0
#define INPUT_PULLDOWN 0
#define HEX 16
inline unsigned long clockMs = 100;
inline bool plugged = true;
inline unsigned long millis() { return clockMs; }
inline void delay(unsigned long n) { clockMs += n; }
inline void pinMode(int, int) {}
inline bool digitalRead(int) { return plugged; }
class HardwareSerial {
public:
 std::deque<uint8_t> rx;
 void begin(int, int, int, int) {}
 void updateBaudRate(unsigned long) {}
 void flush() {}
 size_t available() { return rx.size(); }
 uint8_t read() { auto b = rx.front(); rx.pop_front(); return b; }
 void write(const uint8_t*, size_t) {}
 template<class T> void print(T, int = 0) {}
 template<class T> void println(T) {}
 void println() {}
};
inline HardwareSerial Serial, Serial1;
''')
    (path / "HardwareSerial.h").write_text('#include "Arduino.h"\n')
    (path / "EEPROM.h").write_text(r'''
#pragma once
#include "Arduino.h"
class FakeEEPROM {
public:
 std::vector<uint8_t> bytes = std::vector<uint8_t>(512, 0);
 bool commitOk = true;
 size_t length() const { return bytes.size(); }
 uint8_t read(size_t at) { return bytes.at(at); }
 void write(size_t at, uint8_t value) { bytes.at(at) = value; }
 bool commit() { return commitOk; }
};
inline FakeEEPROM EEPROM;
''')
    main = (ROOT / "ESP32_master/src/Main.cpp").read_text()
    start = main.index("static DapActions_t makeFanatecAction(")
    helper = main[start:main.index("\n#endif", start)]
    scale_start = main.index("          uint16_t throttleValue = g_pedalThrottleValue_u16;")
    scale = main[scale_start:main.index("          // Set pedal values", scale_start)]
    scale_test = r'''
constexpr uint16_t JOYSTICK_MAX_VALUE = UINT16_MAX;
void testPedalScale() {
 uint16_t previousBrake = 0;
 for (uint32_t input = 0; input <= UINT16_MAX; ++input) {
  uint16_t g_pedalThrottleValue_u16 = input, g_pedalBrakeValue_u16 = input, g_pedalClutchValue_u16 = input;
''' + scale + r'''
  assert(throttleValue == input && clutchValue == input && handbrakeValue == 0);
  assert(brakeValue >= previousBrake && brakeValue <= 22000);
  if (input == 0) assert(brakeValue == 0);
  if (input == 32768) assert(brakeValue == 11000);
  if (input == UINT16_MAX) assert(brakeValue == 22000);
  previousBrake = brakeValue;
 }
}
'''
    test = r'''
#include "Arduino.h"
#include "EEPROM.h"
#define private public
#include "FanatecInterface.h"
#undef private
#include "PayloadHeader.h"
#include "PayloadFooter.h"
#include "PayloadAction.h"
#include "PedalDefine.h"
#include "PedalEnum.h"
struct __attribute__((packed)) DapActions_t {
 PayloadHeader_t payloadHeader_st;
 PayloadPedalAction_t payloadPedalAction_st;
 PayloadFooter_t payloadFooter_st;
};
uint16_t checksumCalculator(uint8_t* data, size_t size) {
 uint16_t result = 0;
 while (size--) result += *data++;
 return result;
}
''' + helper + scale_test + r'''
std::vector<uint8_t> packet(FanatecInterface& f, uint8_t throttle, uint8_t brake) {
 std::vector<uint8_t> p = {0x7B,0,throttle,brake,1,0,0,0,0,0,0,0x7D};
 p[10] = f.generateCRC(p.data()+1,9);
 return p;
}
void feed(FanatecInterface& f, const std::vector<uint8_t>& p) {
 Serial1.rx.insert(Serial1.rx.end(),p.begin(),p.end()); f.update();
}
int main() {
 testPedalScale();
 // Existing MAC/pairing bytes survive startup and setting persistence.
 for (size_t i=0;i<128;++i) EEPROM.bytes[i] = uint8_t(i);
 const auto before = EEPROM.bytes;
 FanatecInterface f(18,17,16); f.begin();
 assert(!f.vibrationEnabled());
 assert(f.setVibrationEnabled(true));
 for (size_t i=0;i<128;++i) assert(EEPROM.bytes[i] == before[i]);
 assert(EEPROM.bytes[128] == 0xA7 && EEPROM.bytes[129] == 1);
 FanatecInterface rebooted(18,17,16); rebooted.begin(); assert(rebooted.vibrationEnabled());
 f._connected = f._initialized = f._plugState = f._lastRawPlugState = true;
 auto p = packet(f,123,0);
 feed(f, {p.begin(),p.begin()+5}); assert(f.throttleVibration() == 0);
 feed(f, {p.begin()+5,p.end()}); assert(f.throttleVibration() == 123);
 clockMs += 200; feed(f,packet(f,0,234));
 assert(f.throttleVibration() == 123 && f.brakeVibration() == 234);
 clockMs += 301; assert(f.throttleVibration() == 0 && f.brakeVibration() == 234);
 auto corrupt=packet(f,222,0); corrupt[10]^=1;
 feed(f,corrupt); assert(f.throttleVibration() == 0);
 // Match v3: discard a malformed frame, then accept the next complete one.
 p=packet(f,42,0); feed(f,p);
 assert(f.throttleVibration() == 42);
 assert(f.setVibrationEnabled(false)); assert(f.throttleVibration()==0 && f.brakeVibration()==0);
 assert(f.setVibrationEnabled(true)); feed(f,packet(f,7,8));
 plugged=false; f.communicationUpdate(); clockMs += 30; f.communicationUpdate(); assert(!f.isConnected());
 assert(f.throttleVibration()==0 && f.brakeVibration()==0 && f._rxFrameIndex==0);
 // Refreshes preserve continuous host effects, but never replay commands.
 DapActions_t previous{};
 memset(&previous.payloadPedalAction_st,9,sizeof(previous.payloadPedalAction_st));
 auto action=makeFanatecAction(PEDAL_ID_THROTTLE,&previous,0,255);
 auto& a=action.payloadPedalAction_st;
 assert(a.rpm_u8==100 && a.gValue_u8==9);
 assert(a.systemAction_u8==0 && a.returnPedalConfig_u8==0 && a.startSystemIdentification_u8==0);
 assert(a.triggerCv1_u8==0 && a.triggerCv2_u8==0 && a.triggerCv3_u8==0 && a.triggerCv4_u8==0);
 assert(a.rudderAction_u8==0 && a.rudderBrakeAction_u8==0 && a.wheelSlip_u8==0 && a.impactValue_u8==0);
 assert(previous.payloadPedalAction_st.rpm_u8==9);
 auto stopped=makeFanatecAction(PEDAL_ID_THROTTLE,&previous,0,0);
 assert(stopped.payloadPedalAction_st.rpm_u8==9);
 auto standalone=makeFanatecAction(PEDAL_ID_BRAKE,nullptr,255,0);
 assert(standalone.payloadPedalAction_st.triggerAbs_u8==1 && standalone.payloadPedalAction_st.gValue_u8==128);
 assert(standalone.payloadHeader_st.startOfFrame0_u8==SOF_BYTE_0_U8);
 assert(standalone.payloadHeader_st.pedalTag_u8==PEDAL_ID_BRAKE);
 assert(standalone.payloadFooter_st.enfOfFrame1_u8==EOF_BYTE_1_U8);
 assert(standalone.payloadFooter_st.checkSum_u16==checksumCalculator((uint8_t*)&standalone,sizeof(PayloadHeader_t)+sizeof(PayloadPedalAction_t)));
}
'''
    (path / "test.cpp").write_text(test)
    subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-I"+tmp,
                    "-I"+str(ROOT / "ESP32_master/include"),
                    "-I"+str(ROOT / "Common_Libs/DiyActivePedal_types/src"),
                    str(path / "test.cpp"), str(ROOT / "ESP32_master/src/FanatecInterface.cpp"),
                    "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
print("PASS: Fanatec parser, EEPROM preservation, timeouts, disconnect, and root action conversion")
