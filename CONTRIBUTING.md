# Contributing

TaskVitals aims to stay small, useful and easy to maintain.

Please open an issue for substantial changes before implementing them. Include your Windows version, display scaling, build architecture, and a short description of the problem or proposed behavior.

For code changes:

1. Keep the interface and documentation in English.
2. Prefer the existing Win32 APIs and cached samples.
3. Keep idle work event-driven and preserve the configurable sampling interval.
4. Build with compiler warnings enabled and run the sampling self-test.
5. Check the affected UI manually. For allocation, drawing or timer changes, also run the resource measurement script.
6. Update documentation when controls or saved settings change.

The interactive smoke test temporarily changes startup and preference values, then restores the prior values. Read [testing](docs/TESTING.md) before running it.

Pull requests should describe the user-visible change, the reason for it, and the validation performed. Contributions are licensed under the project's MIT license.
