# TaskVitals

**A portable Windows 11 taskbar CPU and RAM monitor with optional GPU usage.**

See CPU and memory usage at a glance in a small transparent taskbar strip. TaskVitals is a native C++/Win32 system monitor: no installation, no Electron and no .NET runtime. GPU monitoring is optional and off by default. English-only interface, open source under the MIT license.

Built for people who want a few readable numbers on the taskbar, with hover details and simple controls for size, color, position and refresh speed.

![TaskVitals on the Windows taskbar](docs/images/taskbar.png)

## Download

**[Download for Windows (.zip)](https://github.com/razbenaharon/TaskVitals/releases/download/v1.7.0/TaskVitals-1.7.0-windows-x86.zip)** · [Release notes and source](https://github.com/razbenaharon/TaskVitals/releases/latest)

1. Download `TaskVitals-1.7.0-windows-x86.zip` and choose **Extract all**.
2. Keep the extracted folder somewhere permanent, then double-click `TaskVitals.exe`.
3. Look for **CPU / RAM** on your taskbar. Right-click the visible text for options, including **Show GPU (optional)** and **Exit**.

The portable x86 build is tested on x64 Windows 11. No account, installer or administrator permission is required. The ZIP contains `TaskVitals.exe`, `README.txt`, `LICENSE` and `CHANGELOG.md`. The first CPU percentage arrives with the next sample, normally after 5 seconds. GPU and startup with Windows are off by default.

The executable is unsigned. Windows may prompt when running a downloaded copy. Source and build instructions are included for anyone who prefers to build it.

## Features

- Transparent CPU and RAM percentages over the taskbar, plus an optional desktop card view.
- Optional GPU utilization, **off by default**, with no GPU sampling or PDH startup load while disabled.
- Optional CPU speed limit and CPU temperature (read from Core Temp), both **off by default**. No driver or administrator access in TaskVitals itself.
- Hover details: used, total and available RAM, uptime, and the last sample time.
- Small, Normal and Large text, with a layout that scales to fit.
- Five color presets and custom CPU/RAM accent and percentage-text colors.
- Taskbar position presets, small position adjustments, and Ctrl+drag placement.
- Placement avoids taskbar controls and reacts to app buttons appearing or moving.
- Refresh every 1, 2, 5 or 10 seconds; **5 seconds by default**.
- Saved preferences, optional startup, and single-instance operation.
- Native Win32 message loop, one sampling timer, and conditional repainting.

![Hover details](docs/images/tooltip.png)

## Controls

| Action | How |
| --- | --- |
| Open options | Right-click the visible text |
| See more detail | Hover over the visible text |
| Show GPU usage | Check **Show GPU (optional)**; off by default |
| Show CPU speed limit | Check **Show CPU speed limit (optional)**; off by default |
| Show CPU temperature | Check **Show CPU temperature (needs Core Temp)**; off by default |
| Change text size | **Text size** in the menu |
| Change colors | **Colors** in the menu |
| Change placement | **Taskbar position** in the menu |
| Set a custom taskbar position | Hold **Ctrl** and drag the visible text |
| Show the desktop cards | Uncheck **Taskbar view** |
| Move the desktop cards | Drag the title or either card |
| Change refresh speed | **Refresh interval** in the menu |
| Start at sign-in | Check **Start with Windows** |
| Quit | **Exit**, or **Esc** when the widget has focus |

Transparent pixels pass mouse input through to the taskbar. Aim at the letters or numbers when hovering or opening the menu. There is no taskbar button or tray icon; the right-click menu is the main control.

The **Colors** menu offers Default, White, Ocean, Warm and Dark text presets. Use **CPU label and bar...**, **RAM label and bar...**, or **Percentage text...** for a custom Windows RGB picker. Colors apply to both views and survive restart. The background stays transparent in taskbar mode. The picker loads only while it is being used; it adds no sampling timer.

![Ocean color preset](docs/images/colors.png)

![Desktop card view](docs/images/desktop.png)

## Optional GPU

Right-click the visible text and check **Show GPU (optional)**. Both views gain a third column. The choice is saved; new users start with GPU disabled. GPU labels and bars share the RAM accent color, and percentage text uses the selected value color.

GPU is sampled at the existing 1/2/5/10-second refresh interval. The native Windows PDH library and query are initialized only when enabled. A bounded buffer is reused between samples, and the query, buffer and library reference are released when disabled. No separate service, worker, driver installation or sampling timer is added.

The percentage is the **busiest physical engine across all GPUs**: per-process readings are added only within the same adapter/physical engine, then the largest engine total is shown. This includes 3D, compute, copy and video engines exposed by Windows. It is not a sum of all GPUs, a temperature reading, VRAM usage or a promise of an exact Task Manager match. Unsupported drivers, invalid samples and startup warm-up show `--`. A longer interval smooths spikes.

![Optional GPU in the desktop view](docs/images/gpu.png)

## Optional CPU speed limit and temperature

Windows does not give ordinary programs the CPU temperature. Reading the CPU's own sensor needs a kernel driver and administrator rights, which TaskVitals does not install. Two optional columns cover this instead; each is off by default, saved, and adds one column to both views.

**Show CPU speed limit (optional)** shows `LIM`: the share of maximum performance Windows currently allows the processor (the `% Performance Limit` counter). 100% means no limit. A lower value means the CPU is being held back, by heat or by power policy such as battery saving; the counter does not say which. It is not a temperature. It uses the same on-demand PDH loading as GPU, sampled at the refresh interval, and releases the query and library reference when disabled.

**Show CPU temperature (needs Core Temp)** shows `TEMP`: the hottest core in °C, read from the shared memory that the separate, free [Core Temp](https://www.alcpu.com/CoreTemp/) utility publishes while it runs. Core Temp loads its own driver and asks for administrator approval; TaskVitals only reads the published block, opening and closing it on each sample, so a closed Core Temp shows `--` instead of a stale value. Fahrenheit and distance-to-TjMax modes are converted, and readings outside 0–150 °C are ignored. The reader was checked against a simulated block in Core Temp's documented layout; Core Temp itself was not running on the test machine.

## Small by design

The portable executable is about **53 KiB**. Short local observations at the 5-second refresh interval recorded:

| Mode | Working set | Private memory | Process CPU across the test machine |
| --- | --- | --- | --- |
| GPU off (default) | 12.12–12.18 MiB | 2.31–2.46 MiB | below reported timing resolution |
| GPU on | 16.25–16.31 MiB | 6.11–6.41 MiB | about 0.02% |

GPU adds about 4 MiB of resident memory in this observation. Zero reported CPU time does not mean zero work. The observations lasted about 30 seconds each, sequentially on one Windows 11 machine, and exclude Explorer/DWM work. Layouts are cached between shell changes; GPU data is queried only while enabled.

These are observations from one Windows 11 machine, not fixed requirements or performance guarantees. [Measurement details and limitations](docs/PERFORMANCE.md) include the raw reports and the method.

CPU is the average busy time between samples. A longer interval smooths short spikes. RAM is total physical-memory usage. The first CPU reading is `--` until a second valid sample arrives. Hover details are a snapshot; move away and hover again to update them.

## FAQ

### Does TaskVitals work on Windows 11?

The supplied portable x86 executable has been tested on x64 Windows 11. The taskbar view targets the primary horizontal taskbar. Mixed-DPI setups and secondary or vertical taskbars have not been validated. The widget hides while a full-screen app, such as a full-screen video, is in front on its monitor, and returns when the taskbar does. The Windows secure desktop may still cover it.

### Is it a real taskbar extension? Will it cover my app icons?

It is a topmost window positioned **over** the taskbar, not a shell extension. It does not inject code into Explorer or reserve shell space; it registers only for the shell's full-screen notifications. It reads shell-control bounds and selects a free gap; if no gap fits or those bounds cannot be read, it moves just above the taskbar. Placement has been checked on the test machine, including the wider GPU view, rather than every Windows configuration.

### Does it need Electron, Python, .NET or administrator access?

No. The app is compiled native C++ using Windows system libraries. PowerShell and CMake are development tools, not background runtime requirements. Extract the Windows ZIP and run the executable as your normal user.

### How much RAM and CPU does it use?

Short observations on one Windows 11 machine recorded about 12.2 MiB working set with GPU off and 16.3 MiB with GPU on. These are measurements, not minimum requirements or fixed limits. See [Small by design](#small-by-design) and the [raw reports](docs/PERFORMANCE.md) for CPU results, private memory and limitations.

### How do I enable GPU monitoring? Does it run while disabled?

Right-click the visible text and check **Show GPU (optional)**. It is off by default. No GPU counters are sampled while disabled, and a fresh GPU-off process does not load PDH for GPU monitoring. Disabling GPU closes its query and releases its buffer and library reference; Windows may retain other caches. Details are in [Optional GPU](#optional-gpu).

### Why does CPU or GPU show `--`, or differ from Task Manager?

CPU needs two valid samples; GPU needs available Windows performance counters and an initial sampling interval. Unsupported drivers or invalid samples also show `--`. CPU is averaged between samples, and GPU shows the busiest physical engine across all GPUs. Different averaging and aggregation can produce different results from Task Manager.

### Can I change the text, colors, position and refresh interval?

Yes. Right-click the visible letters or numbers for three text sizes, color presets/custom colors, placement options, and 1/2/5/10-second refresh intervals. The default is 5 seconds. Hold Ctrl and drag the text for a preferred custom position; collision avoidance still applies.

### Does TaskVitals send telemetry or require an account?

The app has no telemetry, account system or automatic update checker. There is no app-level network request code. Downloads and source are hosted on GitHub; updating means downloading a newer release.

### Can it start automatically with Windows? How do I quit?

Check **Start with Windows** in the right-click menu; it is off by default and applies only to your user account. Windows Settings can disable that startup entry independently. Use **Exit** in the same menu to quit. There is no tray icon or taskbar button, so aim at the visible widget text to open its menu.

### Why might Windows warn about the download?

The executable is unsigned. Windows may show a warning for a downloaded copy. The release includes SHA-256 checksums, and the source and [build instructions](docs/BUILDING.md) are available for inspection or building your own copy.

### How is it different from TrafficMonitor or a full hardware monitor?

TaskVitals focuses on a small CPU/RAM percentage display, optional GPU usage and a portable native executable. It does not currently include history graphs, network speed, disk activity or VRAM monitoring, and it reads CPU temperature only from Core Temp. Choose it when that focused feature set fits what you need; no claim is made that it uses fewer resources than other tools.

## Build and test

With Visual Studio C++ Build Tools, the Windows SDK and CMake 3.20 or newer:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The supplied download was built and tested locally with MinGW. [Windows CI](https://github.com/razbenaharon/TaskVitals/actions/runs/38930431512) also built, self-tested and packaged MSVC x86 and x64 successfully. Interactive desktop checks and resource measurements apply to the supplied MinGW x86 build; CI does not validate the taskbar UI on every Windows configuration.

[Build instructions](docs/BUILDING.md) · [Testing](docs/TESTING.md) · [Contributing](CONTRIBUTING.md) · [Changelog](CHANGELOG.md)

## Remove

Uncheck **Start with Windows**, exit, then delete the folder. Preferences live under `HKEY_CURRENT_USER\Software\TaskVitals`. See [settings and removal](docs/SETTINGS.md) for the exact registry entries.

## License

[MIT](LICENSE). Contributions and small, focused improvements are welcome.
