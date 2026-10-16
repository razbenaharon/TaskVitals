# Performance and verification

Reference measurements were collected on Windows 11 Pro 10.0.26300 x64 on 2026-10-16, using the x86 MinGW.org GCC 6.3.0 build at Normal text size, Right position and 5-second refresh.

The one-minute table below describes version 1.4.1, before the color options added in 1.5.0. The color feature adds stored RGB values and user-triggered repainting, without changing the sampling timer. The Windows color-picker library is loaded on request and its reference is released after closing; it is not a startup import.

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

This short observation includes native COM/accessibility initialization used for avoiding shell controls. It is a few MiB larger than the older 1.4.1 core. Layout bounds are cached between meaningful Explorer events; the ordinary CPU/RAM sampling cadence is unchanged. This is an uncontrolled desktop observation, not a fixed resource limit or a long-term leak test. It excludes work performed in Explorer and DWM.

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
- Update taskbar geometry only when it changes.
- Cache up to 128 occupied taskbar spans in a fixed array. Refresh native accessibility bounds on Explorer layout events, coalesced by a one-shot 200 ms timer; ignore ordinary focus, selection and visible pressed/flashing state changes. No periodic accessibility scan when the event hook is available and the layout is unchanged.
- Release all accessibility object references after each layout scan. Native Windows COM/accessibility initialization adds some resident memory relative to the older core.
- Reuse cached CPU/RAM samples for hover details.
- Create fonts on size changes and release the previous handles.
- Use a native common-controls tooltip, without a background sampling worker.
- Opt the UI thread out of input-method initialization before creating windows, using [ImmDisableIME](https://learn.microsoft.com/en-us/windows/win32/api/imm/nf-imm-immdisableime). The main widget has no free-text input; the optional native RGB picker uses numeric fields. Revisit this choice if general text-entry controls are added later.

No working-set trimming, forced paging, global Windows setting changes or additional polling service was used.

## Validation status

The local candidate passed sampling self-tests and interactive checks for single instance, topmost style, views, refresh choices, startup, exit, all sizes, font resource reuse, saved size/position, native hover and actual Ctrl+drag. Startup preferences were restored after testing. The publication candidate is checked again before packaging.

MSVC CI is configured but has not yet run for the unpublished repository. Mixed-DPI hardware, secondary/vertical taskbars, fresh sign-in startup and long-duration resource behavior remain unvalidated.
