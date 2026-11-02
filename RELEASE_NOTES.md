# TaskVitals 1.9.2

A small native Windows CPU and RAM monitor, with a transparent taskbar view and optional desktop cards.

Version 1.9.2 is a bug-fix release for the taskbar strip, which could float above the taskbar instead of sitting in it:

- **After the display scale changes while TaskVitals runs** (for example from 100% to 125%). TaskVitals measured the taskbar's buttons and the taskbar itself in different scales, found no buttons and floated. The buttons are now mapped into the same scale.
- **After waking from sleep or hibernation.** TaskVitals now reads the taskbar again shortly after resuming and after display changes, and also retries when a read finds no free room, since Explorer can report its buttons before laying them out.

No settings change. The network, GPU, speed-limit and temperature columns from earlier versions are unchanged and stay off until you turn them on.

## Download and run

Download `TaskVitals-1.9.2-windows-x86.zip`, extract it to a permanent folder, and run `TaskVitals.exe`. No installer or separate runtime is needed. This x86 build was tested on x64 Windows 11. Right-click the visible text for options.

## Notes

The executable is unsigned. Taskbar mode overlays the primary horizontal taskbar and does not reserve shell space. Mixed-DPI and secondary/vertical taskbars have not been validated. The optional temperature column still relies on the separate Core Temp utility; see the README for its license and the temperature disclaimer.

Use the accompanying `.sha256` file to verify the downloaded ZIP. Source and build instructions are available in the repository.
