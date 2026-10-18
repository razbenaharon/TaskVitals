# Testing

## Sampling self-test

```powershell
$p = Start-Process .\release\TaskVitals.exe -ArgumentList '--self-test' -PassThru -Wait
$p.ExitCode
```

A zero exit code means success. Tests cover busy/idle arithmetic, invalid or reset counters, zero deltas, and live access to Windows sampling APIs. This mode opens no widget and is also registered as a CTest test.

## Interactive smoke test

Close any running TaskVitals instance, then use an interactive Windows desktop:

```powershell
.\tests\smoke.ps1
```

The default executable is `release/TaskVitals.exe`. Override it with `-Executable` for a CMake build. Evidence goes under `build/verification` by default.

The script exercises topmost and single-instance behavior, view changes, all text sizes, repeated font replacement, placement, refresh settings, startup registration and exit. It also verifies size and position persistence through a real process restart. It temporarily modifies startup and preference values, then restores the prior values in a finally block. Avoid changing those settings manually during the test.

Color checks apply presets and verify CPU, RAM and percentage colors survive the same restart.

The smoke test is intentionally separate from unattended CI because it needs a usable desktop and temporarily exercises the current user's startup entry.

## Manual UI checks

- Hover over visible text and confirm CPU, RAM, uptime and sample time appear.
- Move away to dismiss the tooltip; rehover for a fresh snapshot.
- Hold Ctrl and drag the strip horizontally. Wait longer than the sampling interval and confirm placement persists.
- Check Small, Normal and Large text in both views for clipping.
- Open the menu with right-click and Shift+F10; check Esc exits when the widget has keyboard focus.
- Check transparent areas pass clicks through to the taskbar.
- Apply the color presets in both views. Open each custom color picker, verify RGB choices apply, and verify Cancel keeps the previous colors. Reopen the app to check persistence. The picker library should not be imported at startup and the app releases its on-demand reference after closing the picker.

## Resource observation

Start exactly one instance, use hover and drag, then move the pointer away:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
.\tests\measure.ps1 -Seconds 60 -OutputFile .\build\measurement.json
```

The script records CPU time and cycles, sampled working/private memory, GDI/USER resources and handles. Use comparable view, refresh and desktop activity for comparisons. See [the reference measurements](PERFORMANCE.md) for interpretation and limits.

## Taskbar collision check

With the widget running, use `./tests/placement.ps1` on an interactive desktop. It independently queries actual shell button rectangles through UI Automation and checks Left and Right at all three text sizes. The app itself uses native accessibility and caches control bounds. The built-in self-test also covers synthetic centered, overlapping and full taskbar layouts.

## Optional CPU speed limit and temperature

Close the widget and run `./tests/thermal.ps1`. It checks that both options default to off, that PDH is absent while the speed limit is off, expansion, persistence, disable/unload and repeated toggles. It saves a desktop image under `build/verification`. The built-in self-test checks Core Temp parsing for Celsius, Fahrenheit, distance to TjMax and invalid core counts without Core Temp installed.

## Optional GPU

Close the widget and run `./tests/gpu.ps1`. It checks the missing-preference default, PDH absence while off, expansion, persistence, disable/unload and repeated toggles. It saves a desktop GPU image under `build/verification`. Counter availability depends on the driver. The built-in self-test checks per-engine aggregation, adapter separation, invalid samples and the 100% cap without loading PDH.
