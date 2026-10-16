# Release verification

Version 1.7.0, prepared on 2026-10-16 for the public [TaskVitals repository](https://github.com/razbenaharon/TaskVitals) and [download release](https://github.com/razbenaharon/TaskVitals/releases/tag/v1.7.0).

Version 1.7.0 was rebuilt with optional GPU monitoring, then checked with the GUI smoke suite, GPU toggle/restart tests, live placement tests and built-in aggregation tests. Custom RGB-dialog interaction and library unloading were verified on the unchanged picker implementation in 1.5.0.

## Current checks

- GPU is disabled when its preference is missing; PDH is absent from a fresh GPU-off process.
- GPU enable expands the strip, survives restart, and disables back to two columns. Repeated disable closes PDH and unloads the library on this test machine.
- Real GPU Engine counters produced valid utilization samples on the test machine.
- Aggregation tests cover multiple processes on one engine, separate adapters/engines, invalid samples and the 100% cap.
- Independent live placement checks cover the wider GPU-enabled strip at all three sizes.

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
- The portable archive includes a self-tested executable and a SHA-256 sidecar.

## Binary identity

- File: TaskVitals.exe
- Architecture: x86
- Size: 53,760 bytes
- SHA-256: 0B2CD32459140E5BFD79E347A612B043D621AF39A9959D6DE2E49CB33739C611
- Signature: unsigned

The current [smoke report](measurements/publication-smoke.json) recorded 12.46 MiB working set and 2.77 MiB private memory during its 12-second desktop observation; GDI stayed at 14 and USER resources at 7. Process CPU time was 15.62 ms. Separate GPU-off/on observations and limitations are in [PERFORMANCE.md](PERFORMANCE.md).

The longer resource reference in [PERFORMANCE.md](PERFORMANCE.md) describes 1.4.1 before the color option. The current short observation is linked there separately; no controlled long-duration comparison has been made for 1.7.0. Colors are cached RGB values; the custom picker is event-driven and its library is released after the dialog closes.

The final process is left running with Default colors, the transparent taskbar view and the existing refresh interval. [Windows CI](https://github.com/razbenaharon/TaskVitals/actions/runs/38930431512) successfully built, self-tested and packaged MSVC x86 and x64. GUI and performance checks apply to the supplied MinGW x86 portable binary. Mixed-DPI, secondary/vertical taskbars, fresh sign-in startup and long-duration resource behavior remain unvalidated.
