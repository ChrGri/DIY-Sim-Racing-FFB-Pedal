#!/usr/bin/env python3
"""Source-level guard for the COM callbacks actually registered by the UI.

This does not replace a Windows WPF build/runtime test.
"""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
PLUGIN = ROOT / "SimHubPlugin"
sources = {p: p.read_text(encoding="utf-8-sig", errors="replace")
           for p in (PLUGIN / "UICallback").glob("*.cs")}
handlers = set()
for text in sources.values():
    handlers.update(re.findall(
        r"ESP_host_serial_timer\.Tick\s*\+=\s*new EventHandler\((\w+)\)", text))
assert handlers, "No COM receive callbacks found"

for handler in handlers:
    matches = [text for text in sources.values()
               if re.search(r"void\s+" + re.escape(handler) + r"\s*\(", text)]
    assert len(matches) == 1, handler
    # Only the validated bridge-status branch may update the vibration UI.
    branch = matches[0].split("DAP_bridge_state_st bridge_state =", 1)[1]
    checked = branch.index("if ((check_payload_state_b) && check_crc_state_b)")
    update = branch.index("UpdateFanatecVibrationStatus(bridge_state.payloadBridgeState_.Bridge_action);")
    version = branch.index("Plugin._calculations.BridgeFirmwareVersion[")
    assert checked < update < version, handler

hid = sources[PLUGIN / "UICallback/HidRecieveCallback.cs"]
assert "UpdateFanatecVibrationStatus(bridge_state.payloadBridgeState_.Bridge_action);" in hid
ET.parse(PLUGIN / "UIFunction/SettingSection_System.xaml")
print("PASS: registered COM callbacks and HID route Fanatec status to the UI; XAML parses")
