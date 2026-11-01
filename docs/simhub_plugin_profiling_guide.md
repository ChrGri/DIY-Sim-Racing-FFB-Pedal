# SimHub Plugin Performance Profiling Guide

This guide describes how to profile the running **SimHub** application (`SimHubWPF.exe`) and find hot paths, CPU spikes, and UI stalls in the **`DiyFfbPedal.dll`** plugin using the automated profiling script.

---

## 1. Overview & Architecture

* **Target Process**: `SimHubWPF.exe` (32-bit .NET Framework 4.8 application)
* **Target Plugin**: `DiyFfbPedal.dll` (located at `C:\Program Files (x86)\SimHub\DiyFfbPedal.dll`)
* **Symbols (PDB)**: `DiyFfbPedal.pdb` (enables exact method name and source code line resolution)
* **Profiler**: [Microsoft PerfView](https://github.com/microsoft/perfview) (low-overhead Event Tracing for Windows - ETW)
* **Automation Script**: `SimHubPlugin\profile_simhub.bat`

Because SimHub runs elevated (**as Administrator**) to access raw hardware and game hooks, capturing CPU sample stacks and CLR ETW events requires administrative elevation. The batch script handles this elevation automatically via Windows UAC.

---

## 2. Prerequisites

1. **SimHub** is installed and running with the DIY FFB Pedal plugin enabled.
2. The pedal hardware (USB bridge / pedals) is connected and actively streaming data (or telemetry is being replayed in SimHub).
3. The following files exist in `SimHubPlugin\`:
   - `profile_simhub.bat` (the runner script)
   - `PerfView.exe` (the ETW profiler executable)

---

## 3. Step-by-Step Profiling Procedure

### Step 1: Prepare the Test Scenario
1. Launch **SimHub**.
2. Open the **FFB Pedal Dashboard** (or navigate to the specific tab you wish to evaluate, e.g., the **Rudder** tab, **Kinematics**, or **Effects** tab).
3. If testing game telemetry or pedal feedback, start the game or trigger pedal motion so the hot path is active.

### Step 2: Execute the Profiling Script
1. Navigate to:
   ```text
   DIY-Sim-Racing-FFB-Pedal_Takeover_From_V7\SimHubPlugin\
   ```
2. Double-click **`profile_simhub.bat`** (or right-click and select **Run as administrator**).
3. Accept the **Windows User Account Control (UAC)** prompt.
4. A console window will open and begin a **20-second sample collection**:
   ```text
   =======================================================================
        SimHub / DiyFfbPedal Hot Path Profiler (Microsoft PerfView)
   =======================================================================

   Target Process: SimHubWPF.exe
   Target Plugin:  DiyFfbPedal.dll
   Output File:    SimHubProfile.etl.zip

   [1/2] Starting 20-second CPU sampling and .NET CLR event collection...
   ```

### Step 3: Perform the Action Under Test
During the 20-second window, actively exercise the feature you want to measure (e.g., move the pedals, interact with the UI, or switch tabs).

### Step 4: Automatic Completion & Launch
Once 20 seconds elapse:
* The trace data is merged into `SimHubPlugin\SimHubProfile.etl.zip`.
* **PerfView** will launch automatically and load the trace file.

---

## 4. Analyzing Hot Paths in PerfView

When PerfView opens with `SimHubProfile.etl.zip`:

### 1. Open the CPU Stacks Window
1. In the left-hand tree view, double-click **CPU Stacks**.
2. If prompted by the **Select Process** dialog:
   * Double-click **`SimHubWPF`** (PID is typically shown next to it).
   * Do **not** select `devenv` or background Windows services.

### 2. Filter Directly to `DiyFfbPedal`
At the top of the CPU Stacks window:
1. Locate the **`IncFilter`** (Inclusive Filter) input box.
2. Type:
   ```text
   DiyFfbPedal
   ```
3. Press **Enter** (or click **Update**).
4. All third-party SimHub modules, Windows kernel functions, and unrelated background threads will be hidden, leaving only methods executing in `DiyFfbPedal.dll`.

### 3. Ungroup Methods to See Exact Functions
By default, PerfView groups methods by module (e.g. `module clr <<clr!?>>`):
1. In the **`GroupPats`** dropdown at the top, select **`[no grouping]`** (or clear the box completely).
2. Set **`Fold%`** to **`0`** or **`1`** to avoid folding low-percentage functions.
3. Press **Enter**.

### 4. Switch to CallTree or Flame Graph
* **CallTree Tab**:
  - Shows top-down call stacks starting from the thread roots down into your plugin methods.
  - Expand `DIY_FFB_Pedal.DataUpdate` or `HidDeviceController.ReadLoop` to see exactly which lines or sub-functions consume execution time.
* **Flame Graph Tab**:
  - Provides an immediate visual chart where horizontal width represents % of CPU time consumed.
  - Wider bars indicate the biggest hot paths.

### 5. Interpreting Metrics
* **`Inc %` (Inclusive CPU %)**: Total time spent in this method **plus** all methods it calls. Use this to identify the high-level bottlenecks (e.g. `ReadLoop` or `UpdateRudderTelemetryInternal`).
* **`Exc %` (Exclusive CPU %)**: Time spent strictly inside this method's own instructions (excluding child calls). High exclusive time points to heavy in-place computation, loops, or coordinate math (e.g., `MapRssiToY`).

---

## 5. Typical Bottlenecks & Best Practices

| Symptom | Typical Root Cause in Plugin | Recommended Fix |
|---|---|---|
| **High `wpfgfx_v0400` CPU (10–20%)** | Calling UI/graph redraw logic (`poly.Points = ...`) directly on high-frequency HID/serial packet receipt. | Decouple packet reception from rendering. Update volatile variables on packet arrival; throttle UI/graph redraws to a 25–30 Hz `DispatcherTimer`. |
| **High `Dispatcher.BeginInvoke` samples** | Dispatches to UI thread at 500–1,000 Hz. | Process calculations, packet checksums, and joystick state on background threads; only dispatch when UI values change. |
| **High Gen-0 GC / allocations** | Re-allocating `new byte[]` in `ReadLoop` or creating `new PointCollection()` per redraw tick. | Use `ArrayPool<byte>.Shared` or pre-allocate reusable collections and update coordinates in place. |
| **CPU spikes in `DataUpdate`** | Dynamic reflection / `NCalc` string parsing and logging on every tick. | Cache compiled expressions, avoid string conversions for numerical evaluations, and rate-limit error logs. |

---

## 6. Adjusting Profiling Duration or Parameters

If you need a shorter or longer capture window, edit `SimHubPlugin\profile_simhub.bat`:
* Change `/MaxCollectSec:20` to the desired number of seconds (e.g. `/MaxCollectSec:30` for 30 seconds).
* To stop collection manually before the timer expires, run PerfView with `/Stop` or press any key in the collection window.
