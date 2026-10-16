# TaskVitals 1.7.0

A small native Windows CPU and RAM monitor, with a transparent taskbar view and optional desktop cards.

Version 1.7.0 adds **Show GPU (optional)**, disabled by default. Enabling it adds a third column and samples the busiest physical GPU engine at the existing refresh interval. Windows PDH is loaded only while enabled; disabling GPU releases its query, buffer and library reference. Taskbar collision avoidance remains active for the wider view.

## Download and run

Download `TaskVitals-1.7.0-windows-x86.zip`, extract it to a permanent folder, and run `TaskVitals.exe`. No installer or separate runtime is needed. This x86 build was tested on x64 Windows 11.

Right-click the visible text for text sizes, position presets, refresh speed, optional startup and Exit. Hover for detailed memory information. Hold Ctrl and drag to choose a custom taskbar position.

## Included

- CPU and RAM percentages, hover details and uptime.
- Optional GPU utilization, off by default; unavailable counters show `--`.
- Small, Normal and Large text.
- Five color presets and custom CPU/RAM accent and percentage-text colors.
- Saved placement and 1/2/5/10-second refresh choices.
- Transparent taskbar overlay and a draggable desktop view.
- English-only interface and MIT-licensed source.

The default refresh is 5 seconds. GPU monitoring adds no sampling timer, worker, service or driver installation. Short local observations recorded about 12.2 MiB working set with GPU off and 16.3 MiB with GPU on; GPU-on CPU was about 0.02% across the test machine. See the performance document for raw reports and limitations.

## Notes

The executable is unsigned. Taskbar mode overlays the primary horizontal taskbar and does not reserve shell space; a full or unreadable taskbar moves the strip just above it. Mixed-DPI and secondary/vertical taskbars have not been validated.

Use the accompanying `.sha256` file to verify the downloaded ZIP. Source and build instructions are available in the repository.
