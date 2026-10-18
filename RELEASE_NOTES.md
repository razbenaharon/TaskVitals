# TaskVitals 1.8.0

A small native Windows CPU and RAM monitor, with a transparent taskbar view and optional desktop cards.

Version 1.8.0 adds two optional columns, both **off by default**:

- **Show CPU speed limit (optional)** — `LIM`, the performance Windows says the processor currently guarantees, as a share of its base performance. Power policy, power budget or heat lower it. It is not a temperature. Windows PDH is loaded only while it is enabled.
- **Show CPU temperature (needs Core Temp)** — `TEMP`, the hottest core in °C, read from the data that the separate [Core Temp](https://www.alcpu.com/CoreTemp/) utility publishes while it runs. TaskVitals installs no driver and needs no administrator rights; when Core Temp is closed the column shows `--`.

It also adds an application icon and hides the widget while a full-screen app is in front on its monitor.

## Download and run

Download `TaskVitals-1.8.0-windows-x86.zip`, extract it to a permanent folder, and run `TaskVitals.exe`. No installer or separate runtime is needed. This x86 build was tested on x64 Windows 11. Right-click the visible text for options, including the new columns.

## About Core Temp

Core Temp is an independent product by ALCPU under its own license: free for personal use, with a commercial license for businesses. TaskVitals is not affiliated with it and does not include or redistribute it. If you use it, download version 1.21 or later only from the official site; it installs its own driver and asks for administrator approval.

Temperatures are informational, passed through from Core Temp and up to one refresh interval old. TaskVitals gives no overheating warnings; do not rely on it to protect your hardware.

## Resource use

Short observations on one Windows 11 machine at the default 5-second refresh: about 13.2 MiB working set with the optional columns off, the same with only the temperature column on, and about 17.0 MiB with both new columns on (the speed limit loads PDH). See `docs/PERFORMANCE.md` for raw reports and limitations.

## Notes

The executable is unsigned. Taskbar mode overlays the primary horizontal taskbar and does not reserve shell space. Mixed-DPI and secondary/vertical taskbars have not been validated. The ZIP now includes `THIRD-PARTY-NOTICES.txt` for the statically linked runtime libraries.

Use the accompanying `.sha256` file to verify the downloaded ZIP. Source and build instructions are available in the repository.
