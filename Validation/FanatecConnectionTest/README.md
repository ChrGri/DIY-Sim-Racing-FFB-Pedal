# Fanatec branch migration tests

Source: `origin/v3` commit `b7536a6cee688442902db85a41c9d42ddad4613c`,
`Firmware_for_V3/BridgeFirmware`.

Run `python3 Validation/FanatecConnectionTest/run.py` with a C++17 compiler.
The original connection tests now exercise both the develop V3 bridge and root
bridge with AddressSanitizer/UndefinedBehaviorSanitizer. Tests also cover the
original vibration packet constants, persistence, and rejection of handbrake
calibration commands. Root action conversion is covered separately by
`python3 Validation/Fanatec/test_fanatec.py`.

Migration scope:
- Preserve source handshake, 30 ms plug debounce, 2 s timeout, reconnect,
  bounded UART reads, vibration patterns, and 500 ms motor timeouts.
- Exclude handbrake ADC input, calibration patterns, scaling and persistence.
  The pre-existing handbrake packet field and direct setter remain unchanged.
- Keep EEPROM initialization in bridge setup; V3 uses bytes 64/65 and root
  uses 128/129 to preserve each target's MAC/pairing storage and existing setting.
- Keep the destination's task-shared vibration flag declaration and root
  action packet/transport adapters. No SimHub protocol or UI change is needed.
