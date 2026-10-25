# TaskVitals 1.9.0

A small native Windows CPU and RAM monitor, with a transparent taskbar view and optional desktop cards.

Version 1.9.0 adds optional **network traffic** columns, **off by default**:

- **Show network traffic (optional)** adds `DOWN` and `UP`, the bytes per second received and sent by the physical network adapters, such as Ethernet and Wi-Fi. TaskVitals reads the byte counters Windows already keeps. It sends nothing and runs no speed test, so the columns show the traffic in use right now, not the maximum speed of your connection. Virtual, VPN and filter adapters are left out so traffic is not counted twice.
- Units are decimal bytes per second (`K` = 1,000 bytes/s, `M` = 1,000,000 bytes/s). One `M` is 8 megabits per second.
- The hover details also show the connected adapter's **link rate to the router**, such as `1.1 Gbps`. It is the local connection's ceiling, not your internet speed.
- The Windows IP Helper library loads only while the columns are on.

It also recovers by itself after Explorer restarts. The widget re-attaches to the new taskbar, and the executable's file properties now show its version.

## Download and run

Download `TaskVitals-1.9.0-windows-x86.zip`, extract it to a permanent folder, and run `TaskVitals.exe`. No installer or separate runtime is needed. This x86 build was tested on x64 Windows 11. Right-click the visible text for options, including the new columns.

## Resource use

Short observations on one Windows 11 machine at the default 5-second refresh: about 13.3 MiB working set with the optional columns off, and the same with the network columns on. The executable is 100,352 bytes (about 98 KiB). See `docs/PERFORMANCE.md` for raw reports and limitations.

## Notes

The executable is unsigned. Taskbar mode overlays the primary horizontal taskbar and does not reserve shell space. Mixed-DPI and secondary/vertical taskbars have not been validated. The optional temperature column still relies on the separate Core Temp utility; see the README for its license and the temperature disclaimer.

Use the accompanying `.sha256` file to verify the downloaded ZIP. The file is now a single LF-terminated line, so `sha256sum -c` also works outside Windows. Source and build instructions are available in the repository.
