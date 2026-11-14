# Changelog

## Unreleased

- Far fewer taskbar reads at the Right position on Windows 11. A change that cannot put a button under the strip is no longer read at once: a button or icon leaving, or a new or moved button that reports a place more than a button's width from the strip. One full read still follows within about a minute, at a quiet moment, to catch buttons that move without reporting it, and any change is read at once when the strip is not exactly where it wants to be. In a test of 16 window openings and closings the reads fell from 12 to 1, and CPU time from about 1.5–2.6 seconds to 0.3–0.5. When app buttons were added until they reached a wide strip, the strip still moved away and came back, and an independent check found no covered button at any step. Left, Center and Custom positions, and Windows 10, read every change as before. Not tested: a centered taskbar, notification-area icons appearing, more than one display.
- The taskbar's buttons are no longer read while the strip is hidden behind a full-screen app; they are read when it returns. After a change, the read now waits for 200 ms without further taskbar events (at most one second) instead of 200 ms from the first, and events raised before the last read began no longer cause another. In a test of 16 window openings and closings this did not reduce the number of reads (12–13 either way): each window change is still one read of a quarter to half a second.
- Faster start. The taskbar strip now appears first, at its Right or Custom place, and reads the taskbar's buttons 10 seconds later (or at once when a menu command is used). Reading them took a quarter to half a second and was nearly all of the start-up work: on the test machine the strip appeared after about 40 ms instead of about 300 ms, and start-up CPU time in the first 6 seconds fell from 140–330 ms to 16–62 ms. Left and Center still read at once, since they are defined by where the buttons are. If a button sits at the strip's place, the strip covers it for those 10 seconds and then moves.
- Fixed the taskbar strip staying hidden behind the taskbar while still running. Explorer can bring the taskbar in front of it without notice; the strip now takes the front again on each sample and shortly after focus changes, but not while its menu is open.
- Fixed another app's tray menu (for example the Claude desktop app's) dropping behind the taskbar, leaving items such as Exit unclickable. Taking the front again made Explorer raise the taskbar over the open menu; the strip now waits while any menu or small popup is open over the taskbar, including menus that Chromium and Electron apps draw themselves.
- The smoke test now also turns off the network columns before checking the taskbar strip's width.
- Fixed `--self-test` being ignored when the launcher pads the command line. Windows PowerShell 5.1's `Start-Process` can add a trailing space; TaskVitals then opened the widget instead, so the smoke test hung, or, with TaskVitals already running, the packaging script read the second instance's exit code 0 as a passed self-test.
- The check for hidden (cloaked) windows now loads `dwmapi.dll` from the Windows system directory only, like the other on-demand libraries, instead of by name.
- The GPU, thermal and network test scripts now require PowerShell 7. Under 64-bit Windows PowerShell 5.1 their library checks could not see a 32-bit process's modules, so they passed or failed regardless of the widget's behaviour.

## 1.9.2 — 2026-11-02

- Fixed the taskbar strip floating above the taskbar after the display scale changed while TaskVitals was running (for example 100% to 125%). The taskbar's buttons were measured in screen pixels but compared with the taskbar in the scale TaskVitals started with, so none were found; the buttons are now mapped into the same scale.
- Fixed the taskbar strip floating after waking from sleep or hibernation. TaskVitals now reads the taskbar again shortly after resuming and after display changes, outside the system's notification (where reading the taskbar fails), and also retries a read that finds no free room, since Explorer can report its buttons before laying them out.

## 1.9.1 — 2026-11-01

- Fixed the widget staying hidden behind an ordinary maximized window. Windows can report a full-screen app while only an invisible full-screen overlay from another program is open; TaskVitals now hides only while the window in front actually covers its monitor.
- Fixed the taskbar strip staying above the taskbar after one failed read of the taskbar's buttons. The read is now retried after 1, 2, 4… seconds, at most once a minute, instead of waiting for the next taskbar change.

## 1.9.0 — 2026-10-25

- Added optional network traffic columns (`DOWN` and `UP`), off by default: bytes per second received and sent by the physical adapters, from the counters Windows already keeps. TaskVitals sends nothing and runs no speed test. IP Helper loads only while the columns are on. The hover details also show the connected adapter's link rate to the router, labeled as not the internet speed.
- Added network self-tests (adapter filtering, counter resets, link rate, rate formatting) and `tests/network.ps1`.
- Recovered after Explorer restarts: the widget re-attaches to the new taskbar, re-reads its layout and re-registers for full-screen notifications.
- Added version details (product, version, description, copyright) to the executable's file properties; packaging refuses a mismatch with the project version.
- Wrote `.sha256` files as one LF-terminated line so `sha256sum -c` also works outside Windows.
- Added a MinGW-w64 GCC x86 build, self-test and package to Windows CI alongside MSVC.

## 1.8.0 — 2026-10-18

- Added an optional CPU speed-limit column (`LIM`), off by default, from the Windows `% Performance Limit` counter: the performance the processor guarantees as a share of its base performance, lowered by power policy, power budget or heat. PDH loads only while it is enabled.
- Added an optional CPU temperature column (`TEMP`), off by default, showing the hottest core from Core Temp's shared memory; no driver or administrator access in TaskVitals itself. Checked live with Core Temp 1.21.
- Added `THIRD-PARTY-NOTICES.txt` to the repository and the release ZIP for the statically linked MinGW.org runtime, with an informational note on the GCC runtime libraries.
- Rewrote the README; documented Core Temp's separate license, a temperature disclaimer and 1.8.0 resource observations.
- Generalized the column layout so GPU, speed limit and temperature can be combined in both views.
- Added Core Temp parsing self-tests (Celsius, Fahrenheit, distance to TjMax, invalid counts) and `tests/thermal.ps1`.
- Added an application icon.
- Hid the widget while a full-screen app is in front on its monitor, following the shell's own full-screen notification; it returns with the taskbar.

## 1.7.0 — 2026-10-16

- Added optional GPU utilization, disabled by default and enabled through Show GPU (optional).
- Loaded native Windows PDH only when GPU monitoring is enabled; released the query, reusable buffer and library reference when disabled.
- Used the existing refresh interval with no extra sampling timer or worker.
- Combined per-process counters by physical GPU engine and displayed the busiest engine across all GPUs.
- Expanded both views only while GPU is enabled; kept taskbar collision avoidance.
- Added GPU aggregation, default-off, module-unload, toggle and persistence checks.

## 1.6.0 — 2026-10-16

- Measured actual taskbar controls through Windows accessibility instead of the legacy app-area window.
- Constrained Right, Left, Center and custom placement to unoccupied gaps, including all three text sizes.
- Moved the strip just above the taskbar when no gap fits or control bounds cannot be read safely.
- Cached the layout and coalesced Explorer change events; kept the existing sampling cadence.
- Added placement regression checks for occupied, centered and full taskbar layouts.

## 1.5.1 — 2026-10-16

- Renamed the project, application, build targets, settings and release packages to TaskVitals.
- Updated the README, publishing links and desktop screenshots for the new name.
- Preserved the same native UI, features and sampling interval.

## 1.5.0 — 2026-10-16

- Added five color presets: Default, White, Ocean, Warm and Dark text.
- Added custom CPU/RAM accent and percentage-text colors using the Windows RGB picker.
- Saved colors across restarts and applied them to both taskbar and desktop views.
- Kept the transparent taskbar background and protected foreground colors from matching the transparency key exactly.
- Loaded the system color-picker library on demand and released it after the dialog closed; no new sampling timer.

## 1.4.1 — 2026-10-16

Earlier local release candidate; not previously published.

- Native CPU and physical-memory monitoring in a transparent taskbar strip.
- Optional draggable desktop cards.
- Hover details with RAM usage, available memory, uptime and sample time.
- Three text sizes, taskbar position presets, nudges and Ctrl+drag placement.
- Configurable 1/2/5/10-second refresh, with a 5-second default.
- Saved settings, optional Windows startup and single-instance behavior.
- Conditional repainting and placement to avoid redundant work.
- Reduced input-method initialization overhead for the read-only interface.
- English-only interface, source, documentation and release package.
