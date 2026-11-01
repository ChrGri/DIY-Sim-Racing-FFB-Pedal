#!/usr/bin/env python3
"""Run the production console telemetry policy with a .NET 8 SDK; no SimHub installation needed."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
dotnet = sys.argv[1] if len(sys.argv) > 1 else "dotnet"
with tempfile.TemporaryDirectory(prefix="console-effects-tests-") as temp:
    work = Path(temp)
    shutil.copy2(ROOT / "SimHubPlugin/PluginCallback/ConsoleEffectPolicy.cs", work)
    shutil.copy2(Path(__file__).with_name("Program.cs"), work)
    (work / "Tests.csproj").write_text('''<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net8.0</TargetFramework></PropertyGroup>
</Project>''')
    (work / "NuGet.Config").write_text('<configuration><packageSources><clear /></packageSources></configuration>')
    env = dict(os.environ, DOTNET_CLI_HOME=temp, DOTNET_SKIP_FIRST_TIME_EXPERIENCE="1",
               DOTNET_CLI_TELEMETRY_OPTOUT="1")
    subprocess.run([dotnet, "run", "--project", str(work / "Tests.csproj")], env=env, check=True)

# Integration guards: the tested policy must remain wired into the production callback.
main = (ROOT / "SimHubPlugin/DIYFFBPedal.cs").read_text(encoding="utf-8-sig")
assert "ConsoleEffectPolicy.ShouldProcess(isConsole, effectTelemetryActive, data.NewData != data.OldData)" in main
assert "ConsoleEffectPolicy.IsDue(DateTime.UtcNow, consoleLastEffectAt[pedalIdx])" in main
assert "SendPedalAction(tmp, (byte)pedalIdx, isConsole)" in main
assert "data.NewData != null && (isConsole || data.OldData != null)" in main
io = (ROOT / "SimHubPlugin/PluginCallback/PedalIoCallback.cs").read_text(encoding="utf-8-sig")
assert "if (!preserveIncomingData) ESPsync_serialPort.DiscardInBuffer();" in io
assert "if (!preserveIncomingData) _serialPort[PedalID].DiscardInBuffer();" in io
ET.parse(ROOT / "SimHubPlugin/DIYFFBPedalControlUI.xaml")
project = ET.parse(ROOT / "SimHubPlugin/DIYFFBPedalUI.csproj")
assert any(e.get("Include") == "PluginCallback\\ConsoleEffectPolicy.cs" for e in project.iter())
print("PASS: production wiring, project inclusion and XAML syntax")
