# TaskVitals 1.9.1

A small native Windows CPU and RAM monitor, with a transparent taskbar view and optional desktop cards.

Version 1.9.1 is a bug-fix release:

- **The widget no longer disappears behind an ordinary window.** TaskVitals hides while a full-screen app, such as a game or a full-screen video, is in front. Windows can report a full-screen app while only an invisible full-screen overlay from another program is open, and the widget then stayed hidden. It now hides only while the window in front actually covers its monitor.
- **The taskbar strip no longer floats above the taskbar** after one failed read of the taskbar's buttons. The read is retried after 1, 2, 4… seconds, at most once a minute, instead of waiting for the next taskbar change.

No settings change. The network, GPU, speed-limit and temperature columns from earlier versions are unchanged and stay off until you turn them on.

## Download and run

Download `TaskVitals-1.9.1-windows-x86.zip`, extract it to a permanent folder, and run `TaskVitals.exe`. No installer or separate runtime is needed. This x86 build was tested on x64 Windows 11. Right-click the visible text for options.

## Notes

The executable is unsigned. Taskbar mode overlays the primary horizontal taskbar and does not reserve shell space. Mixed-DPI and secondary/vertical taskbars have not been validated. The optional temperature column still relies on the separate Core Temp utility; see the README for its license and the temperature disclaimer.

Use the accompanying `.sha256` file to verify the downloaded ZIP. Source and build instructions are available in the repository.
