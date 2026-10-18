# Changelog

## Unreleased

- Added an optional CPU speed-limit column (`LIM`), off by default, from the Windows `% Performance Limit` counter; PDH loads only while it is enabled.
- Added an optional CPU temperature column (`TEMP`), off by default, read from Core Temp's shared memory; no driver or administrator access in TaskVitals itself.
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
