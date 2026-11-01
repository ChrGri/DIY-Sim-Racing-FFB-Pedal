#!/usr/bin/env python3
"""Run the v3-branch connection tests against both migrated implementations."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

here = Path(__file__).resolve().parent
projects = [here.parents[1] / 'Firmware_for_V3' / 'BridgeFirmware',
            here.parents[1] / 'ESP32_master']
compiler = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
if not compiler:
    raise SystemExit('Set CXX to a C++ compiler')
for project in projects:
    print(f'Testing {project.relative_to(here.parents[1])}', flush=True)
    with tempfile.TemporaryDirectory(prefix='fanatec-connection-test-') as tmp:
        executable = str(Path(tmp) / 'connection_test')
        subprocess.run([compiler, '-std=c++17', '-g', '-fsanitize=address,undefined',
                        '-I'+str(here/'stubs'), '-I'+str(project/'include'),
                        str(here/'connection_test.cpp'), str(project/'src/FanatecInterface.cpp'),
                        '-o', executable], check=True)
        subprocess.run([executable], check=True)
