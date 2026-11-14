# Performance and verification

Reference measurements were collected on Windows 11 Pro 10.0.26300 x64 on 2026-10-16, using the x86 MinGW.org GCC 6.3.0 build at Normal text size, Right position and 5-second refresh.

The one-minute table below describes version 1.4.1, before the color options added in 1.5.0. The color feature adds stored RGB values and user-triggered repainting, without changing the sampling timer. The Windows color-picker library is loaded on request and its reference is released after closing; it is not a startup import.

## What the tables below leave out: taskbar changes and start-up

Measured on 2026-11-14 on the same machine with the x86 MinGW.org GCC 6.3.0 build, Normal size, 5-second refresh. The tables in the later sections were taken on a quiet desktop. They do not include the cost of reading the taskbar's buttons again, which is the largest cost in ordinary use.

**Each taskbar change costs one read.** Opening or closing a window, or a notification-area icon appearing or leaving, makes TaskVitals read every taskbar button again so that it does not cover one.

| Metric | Observed |
| --- | ---: |
| Accessibility nodes visited per read | about 230, of which about 35 are real buttons |
| Time per read, quiet machine | 245–465 ms |
| Time per read, busy machine | up to about 650 ms |
| CPU time per read | 100–330 ms, typically about 235 ms |
| Reads for 16 window openings and closings | 12–13 |
| CPU time, 3 quiet minutes, network columns on | 703 ms (0.39% of one logical processor) |

The strip does not repaint or answer input during a read. Nearly all of the time goes to the real buttons, which the taskbar serves across processes at several milliseconds each; skipping the unseen window furniture cut the nodes visited to 94 without shortening the read, so that change was not kept. Waiting for 200 ms without events before reading did not lower the number of reads either, because each window change is a separate change. The work Explorer does to answer these calls is not included in any figure here and was not measured.

A process that had run for 32 minutes during heavy testing, with many windows and processes started and stopped, had used about 27 seconds of CPU time. That is an upper example, not a typical day: at about a quarter of a second per change, 200 window changes in a day cost about 50 seconds.

**After the change that skips harmless changes** (Right position, Windows 11 taskbar), the same 16 window openings and closings cost 1 read instead of 12, and 328–516 ms of CPU time instead of 1,531–2,578 ms. The one read is a check about a minute after the first skipped change. A change is skipped only when it cannot put a button under the strip: something leaving, or a new or moved button that reports a place more than a button's width away, while the strip is exactly where it wants to be. With changes several minutes apart, each still costs one read, a minute later. The other positions and Windows 10 are unchanged. A centered taskbar, new notification-area icons and a second display were not tested.

**Start-up.** CPU time shortly after launch, three runs each:

| Configuration | Strip visible after | CPU time |
| --- | ---: | ---: |
| Desktop view (no taskbar read) | 29–42 ms | 16–31 ms in the first 8 s |
| Taskbar view, read at start (1.9.2) | 307–481 ms | 140–330 ms in the first 8 s |
| Taskbar view, read 10 s after start (Right or Custom) | 38–44 ms after the first launch | 16–62 ms in the first 6 s, 141–219 ms by 15 s |
| Taskbar view with GPU, speed limit and network columns (1.9.2) | 291–384 ms | 1,450–2,170 ms in the first 8 s |

Starting the Windows performance-counter library (PDH) for the GPU column or the speed-limit column takes 0.86–1.4 seconds of CPU time each, on the UI thread, before the first sample. The later sections report only the steady cost of those columns. Whether the delayed read changes the start-up impact that Task Manager reports has not been checked; the read still happens, 10 seconds later.

These are short observations on one machine with ordinary background activity, and the timings varied between rounds by a factor of two or more. The read counts are reliable; the exact times are not.

## Version 1.9.0 network traffic

Sequential 30-second observations on the same machine on 2026-10-25, Normal size, Right position, 5-second refresh, with GPU, speed limit and temperature off.

| Metric | Optional columns off (default) | Network traffic on |
| --- | ---: | ---: |
| Duration | 30.08 s | 30.09 s |
| Working set | 13.27–13.34 MiB | 13.29–13.35 MiB |
| Private memory | 2.53–2.71 MiB | 2.40–2.55 MiB |
| CPU time | 31.25 ms | 15.63 ms |
| CPU, whole machine | 0.0087% | 0.0043% |
| IP Helper loaded | no | yes |
| GDI / USER | 16 / 9 | 16 / 9 |
| Handles start/end | 187/187 | 189/189 |

Raw reports: [off](measurements/network-off-1.9.0.json), [network on](measurements/network-on-1.9.0.json).

The network columns read one adapter table per sample through IP Helper and keep at most 32 adapter counters (under 1 KiB). They add no measurable memory or CPU in these runs. The CPU times are one or two ticks of the 15.6 ms process-time clock, so the off/on difference is noise rather than a saving. These remain short, uncontrolled desktop observations, not fixed limits.

## Version 1.8.0 CPU speed limit and temperature

Sequential 30-second observations on the same machine, Normal size, Right position, 5-second refresh, GPU off. Core Temp 1.21 was running throughout, so the temperature column was reading live data.

| Metric | Optional columns off (default) | Temperature only | Speed limit and temperature |
| --- | ---: | ---: | ---: |
| Duration | 30.06 s | 30.10 s | 30.08 s |
| Working set | 13.20–13.25 MiB | 13.22–13.28 MiB | 16.93–16.98 MiB |
| Private memory | 2.54–2.69 MiB | 2.50–2.68 MiB | 6.06–6.21 MiB |
| CPU time | 0.00 ms | 0.00 ms | 15.63 ms |
| CPU, whole machine | below reported resolution | below reported resolution | 0.0043% |
| PDH loaded | no | no | yes |
| GDI / USER | 16 / 9 | 16 / 9 | 16 / 9 |
| Handles start/end | 187/187 | 187/187 | 197/197 |

Raw reports: [off](measurements/thermal-off-1.8.0.json), [temperature only](measurements/temperature-only-1.8.0.json), [speed limit and temperature](measurements/thermal-on-1.8.0.json).

Reading Core Temp's shared memory adds no measurable memory or CPU: TaskVitals copies 2,686 bytes (about 2.6 KiB) and closes the block once per sample. The extra ~3.7 MiB with both columns on comes from the speed-limit column loading PDH, the same library GPU uses. Core Temp itself is a separate process with its own driver and was observed at about 26 MiB working set on this machine; that cost belongs to Core Temp, not TaskVitals, and applies only if you choose to run it.

An earlier off-phase run on the same build recorded 500 ms of CPU time over 30 seconds. It did not repeat in the run above or in the temperature-only run, and is attributed to a one-off desktop event (such as a taskbar layout re-read); it is reported here rather than discarded. These remain short, uncontrolled desktop observations, not fixed limits.

## Version 1.7.0 optional GPU

Sequential observations in the same process used Normal size, Right position and a 5-second refresh interval. The GPU-on phase followed a warm-up and the live placement check. These are short desktop observations, not a controlled benchmark or fixed resource limits.

| Metric | GPU off (default) | GPU on |
| --- | ---: | ---: |
| Duration | 30.09 s | 30.09 s |
| Working set | 12.12–12.18 MiB | 16.25–16.31 MiB |
| Private memory | 2.31–2.46 MiB | 6.11–6.41 MiB |
| CPU time | 0.00 ms | 62.50 ms |
| CPU, one logical processor | below reported resolution | 0.2077% |
| CPU, whole machine | below reported resolution | 0.0173% |
| GDI start/end | 13/13 | 13/13 |
| USER start/end | 7/7 | 7/7 |
| Handles start/end | 168/168 | 178/179 |

Raw reports: [GPU off](measurements/gpu-off-1.7.0.json), [GPU on](measurements/gpu-on-1.7.0.json), [toggle/module checks](measurements/gpu-checks-1.7.0.json).

The enabled reader added about 4 MiB working set on this machine. No zero-overhead claim is made. The reusable PDH data buffer was about 58 KiB during a separate real-counter probe; its allocation is capped at 4 MiB. Disabling releases the query, buffer and PDH library reference; Windows may retain other caches/pages, so immediate working-set return is not guaranteed. A fresh GPU-off process did not load PDH. There is no extra sampling timer or worker, and no forced working-set trimming. Data collection occurs on the UI thread and may take longer with many GPU processes or a slow performance provider. Test other drivers and workloads before drawing broad conclusions.

The GPU calculation groups per-process counters by adapter LUID, physical adapter and engine ID, then displays the busiest engine across all adapters, clamped to 100%. Engine types are not summed together. Unsupported/invalid counters display `--`. Native [PDH array queries](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw) and [language-neutral counter names](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhaddenglishcounterw) are used; no vendor SDK or temperature sensor driver is added.

## Version 1.6.0 taskbar collision avoidance

A 34.67-second observation at Normal size, Right and five-second refresh recorded **12.10–12.16 MiB working set**, **2.36–2.50 MiB private memory**, and **0.1352% of one logical processor** (**0.0113% across this machine**). GDI stayed at 14, USER resources at 7, and handles changed from 171 to 170. [Raw report](measurements/placement-1.6.0.json).

This short observation includes native COM/accessibility initialization used for avoiding shell controls, but few taskbar changes; see the first section for the cost of each one. It is a few MiB larger than the older 1.4.1 core. Layout bounds are cached between meaningful Explorer events; the ordinary CPU/RAM sampling cadence is unchanged. This is an uncontrolled desktop observation, not a fixed resource limit or a long-term leak test. It excludes work performed in Explorer and DWM.

## Observed memory reduction

| Metric | Before read-only input opt-out | Version 1.4.1 |
| --- | ---: | ---: |
| Observation duration | 60.15 s | 60.20 s |
| Working set | 13.09 MiB | 9.25–9.33 MiB |
| Private memory | 1.88 MiB | 1.50–1.72 MiB |
| Process CPU time | below reported resolution | 31.25 ms |
| CPU, fraction of one logical processor | below reported resolution | 0.0519% |
| GDI objects, start/end | 19/19 | 16/16 |
| USER objects, start/end | 15/15 | 5/5 |
| Handles, start/end | 165/165 | 113/113 |

Raw reference reports: [before](measurements/before.json), [after](measurements/after.json).

The observed working-set reduction was about 29%. Private memory was also lower. These were sequential runs under normal desktop activity, not controlled benchmarks. Both had undergone tooltip and Ctrl+drag interaction; the old process had been running longer. No CPU improvement is claimed. Zero reported CPU time does not mean no work occurred. These figures are not fixed limits, and a minute of stable resources does not establish long-term leak freedom.

Working set includes shareable Windows code and is not the same as exclusive private memory. Process-only CPU excludes Explorer and compositor activity. Machine configuration, Windows initialization, CPU frequency and background activity can change the results.

## Design choices

- One ordinary sampling timer, with a 5-second default and 1/2/5/10-second choices.
- Repaint only when visible rounded percentages or validity change.
- Update taskbar geometry only when it changes. At the Right position on the Windows 11 taskbar, a change that cannot reach the strip is not read at once; one full read follows within about a minute.
- At start-up, show the strip first and read the taskbar's buttons 10 seconds later for the Right and Custom positions. The read is a quarter to half a second of cross-process accessibility calls (about 230 nodes), nearly all of the start-up work; Left and Center need the buttons and read at once.
- Cache up to 128 occupied taskbar spans in a fixed array. Refresh native accessibility bounds on Explorer layout events, coalesced by a one-shot 200 ms timer; ignore ordinary focus, selection and visible pressed/flashing state changes. No periodic accessibility scan when the event hook is available and the layout is unchanged.
- Release all accessibility object references after each layout scan. Native Windows COM/accessibility initialization adds some resident memory relative to the older core.
- Reuse cached CPU/RAM samples for hover details.
- Create fonts on size changes and release the previous handles.
- Use a native common-controls tooltip, without a background sampling worker.
- Opt the UI thread out of input-method initialization before creating windows, using [ImmDisableIME](https://learn.microsoft.com/en-us/windows/win32/api/imm/nf-imm-immdisableime). The main widget has no free-text input; the optional native RGB picker uses numeric fields. Revisit this choice if general text-entry controls are added later.

No working-set trimming, forced paging, global Windows setting changes or additional polling service was used.

## Validation status

The local release passed sampling self-tests and interactive checks for single instance, topmost style, views, refresh choices, startup, exit, all sizes, font resource reuse, saved size/position, native hover and actual Ctrl+drag. Startup preferences were restored after testing. The packaged executable also passes its self-test.

[MSVC x86/x64 CI](https://github.com/razbenaharon/TaskVitals/actions/runs/38930431512) passed build, self-test and packaging. Interactive GUI and resource observations apply to the MinGW x86 download. Mixed-DPI hardware, secondary/vertical taskbars, fresh sign-in startup and long-duration resource behavior remain unvalidated.
