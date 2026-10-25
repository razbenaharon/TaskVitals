# Release verification

Version 1.9.0, prepared on 2026-10-25 for the public [TaskVitals repository](https://github.com/razbenaharon/TaskVitals) and [download release](https://github.com/razbenaharon/TaskVitals/releases/tag/v1.9.0).

Version 1.9.0 adds the optional network traffic columns and recovery after Explorer restarts. The final build was checked with the built-in self-test, the GUI smoke suite, the GPU, thermal and network toggle/restart tests and the live placement test with the network columns on. Custom RGB-dialog interaction and library unloading were verified on the unchanged picker implementation in 1.5.0.

## Current checks

- Network traffic is disabled when its preference is missing; IP Helper is absent from a fresh process with the columns off and present with them on.
- Enabling network traffic adds two columns, survives restart and disables back to the previous width; repeated toggles release IP Helper on this test machine.
- The adapter-table layout was checked against Windows on the test machine: the byte counters read by offset matched `Get-NetAdapterStatistics` for the Wi-Fi adapter, the VPN adapter was excluded, and the link rate matched `Get-NetAdapter` (Wi-Fi rates vary from moment to moment).
- Network self-tests cover adapter filtering (physical, filter, loopback), counter resets, adapters that appear or disappear, the link rate (connected adapters only, unknown speeds ignored) and rate formatting.
- Independent live placement checks covered a four-column strip (CPU, RAM, DOWN, UP) at all three sizes with no app/system button overlap.
- Speed limit and temperature are disabled when their preferences are missing; PDH is absent from a fresh process with GPU and the speed limit off.
- Each new column expands the strip, survives restart, and disables back; repeated speed-limit toggles release PDH on this test machine.
- The temperature column was checked live against Core Temp 1.21 on the test machine: it read all 10 cores in Celsius, the values read directly from the shared memory agreed with Core Temp, and the column followed Core Temp's hottest core. Displayed values can differ from Core Temp's own window by sampling time, because Core Temp updates every second and TaskVitals at its refresh interval. With Core Temp not running, it showed `--`.
- Core Temp parsing self-tests cover Celsius, Fahrenheit, distance to TjMax, out-of-range readings and invalid core counts; a simulated block in Core Temp's documented layout was also read end to end.
- A normal-user TaskVitals process could read the shared memory published by an elevated Core Temp.
- GPU is disabled when its preference is missing; PDH is absent from a fresh GPU-off process.
- GPU enable expands the strip, survives restart, and disables back to two columns. Repeated disable closes PDH and unloads the library on this test machine.
- Real GPU Engine counters produced valid utilization samples on the test machine.
- Aggregation tests cover multiple processes on one engine, separate adapters/engines, invalid samples and the 100% cap.
- Independent live placement checks covered a three-column strip at all three sizes with no app/system button overlap.

- Built with MinGW.org GCC 6.3.0, -O2, -Wall and -Wextra, without warnings.
- Sampling self-test, single instance, topmost, view changes, refresh choices, startup registration/removal and exit passed.
- Text sizes, repeated font replacement, placement and size/position persistence passed.
- Independent UI Automation checks found no app/system button intersection for Left and Right at all three sizes.
- On version 1.6.0, a temporary app button moved the Left strip from x=593 to x=637 within two seconds at a ten-second sampling interval, then it returned after the app closed.
- Synthetic centered, overlapping and full taskbar layouts passed the built-in gap-selection checks; a full/unreadable layout uses the above-taskbar fallback.
- Ocean colors survived a real restart for all three saved RGB fields; White applied to all three, then Default was restored.
- The actual Windows RGB dialog was opened, canceled and confirmed through its real controls. Cancel kept the prior color; a custom CPU color was saved correctly.
- A percentage color equal to the transparency key was safely adjusted by one red-channel step, so foreground pixels do not become fully transparent.
- COMDLG32 was present while the picker was open and absent after closing. It is not a startup import. No additional sampling timer was introduced.
- English UI/documentation and relative Markdown links are checked before packaging.
- The portable archive includes a self-tested executable, LICENSE, THIRD-PARTY-NOTICES.txt and a SHA-256 sidecar; the extracted executable passed the self-test and matched the tested build byte for byte.

## Binary identity

- File: TaskVitals.exe
- Architecture: x86
- Size: 100,352 bytes
- SHA-256: 07934FB62FB22C8802DAFC8B59CB1FEB489D4AF152D4A44C61DBD7670F34FEF7
- Signature: unsigned

The current [smoke report](measurements/publication-smoke.json) recorded 13.65 MiB working set and 2.98 MiB private memory during its 12-second desktop observation; GDI stayed at 17 and USER resources at 9. Process CPU time was 15.62 ms, one tick of the process-time clock. Separate network, speed-limit/temperature and GPU observations and limitations are in [PERFORMANCE.md](PERFORMANCE.md).

The longer resource reference in [PERFORMANCE.md](PERFORMANCE.md) describes 1.4.1 before the color option. The current short observation is linked there separately; no controlled long-duration comparison has been made for 1.9.0. Colors are cached RGB values; the custom picker is event-driven and its library is released after the dialog closes.

The final process is left running with Default colors, the transparent taskbar view and the existing refresh interval. [Windows CI](https://github.com/razbenaharon/TaskVitals/actions/workflows/build.yml) builds, self-tests and packages MSVC x86 and x64 and a MinGW-w64 x86 build for each push, including the release commit. GUI and performance checks apply to the supplied MinGW x86 portable binary. Mixed-DPI, secondary/vertical taskbars, fresh sign-in startup and long-duration resource behavior remain unvalidated.
