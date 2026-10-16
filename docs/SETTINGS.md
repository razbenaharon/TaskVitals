# Settings and removal

TaskVitals saves preferences under `HKEY_CURRENT_USER\Software\TaskVitals`:

| Value | Meaning |
| --- | --- |
| X, Y | Desktop view position |
| Taskbar | 1 for taskbar view, 0 for desktop cards |
| RefreshMs | 1000, 2000, 5000 or 10000; default 5000 |
| TextScale | 85, 100 or 120; default 100 |
| Anchor | 0 Right, 1 Center, 2 Left, 3 Custom |
| TaskbarOffset | Custom horizontal offset in 96-DPI logical pixels |
| CpuColor | CPU label and progress-bar RGB color |
| RamColor | RAM label and progress-bar RGB color |
| ValueColor | Percentage text RGB color in both views |

Invalid refresh, scale and anchor values fall back to their defaults. Older language preferences are ignored; the current interface is English-only.

Colors are 24-bit Windows COLORREF values stored as DWORDs (red in the lowest byte). Invalid high-byte values fall back to the default palette. A foreground color exactly matching the taskbar transparency key is adjusted by one red-channel step so the text does not become transparent. Color changes are saved only when a preset or confirmed picker choice is applied; Cancel leaves the colors unchanged. Custom-picker swatches are retained for the running session.

Optional startup creates the string value `TaskVitals` under `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`, containing the executable's quoted absolute path. Administrator rights are not required.

The menu check mark reports this executable's startup registration. Windows Settings can independently disable startup approval. If you move the executable, turn startup off and on from the new location to update the registered path.

To remove the app, uncheck Start with Windows, exit, and delete its folder. Optionally delete the `Software\TaskVitals` preference key. If the executable was already deleted, remove only the `TaskVitals` value from the Run key; keep the Run key itself.

Taskbar placement is constrained to free gaps between accessible shell controls. If the taskbar is full or its controls cannot be read, the strip floats just above it. A saved custom offset is a preferred location; it never overrides collision avoidance. Layout changes are coalesced through a one-shot 200 ms event timer, separate from CPU/RAM sampling.

`GpuEnabled` is a DWORD: 0 (default) disables GPU monitoring; 1 enables it. The optional GPU column uses the RAM accent and the shared percentage-text color. Disabling it releases the PDH query and reusable buffer. Unsupported counters show `--`; toggle off/on to retry initialization.
