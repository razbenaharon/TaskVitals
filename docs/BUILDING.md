# Building TaskVitals

Build on Windows. The source is UTF-8 and uses C++11 with Windows system libraries only.

## MSVC and CMake

Install Visual Studio Build Tools with Desktop development with C++, the Windows SDK, and CMake 3.20 or newer. From a developer terminal in the repository root:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\TaskVitals.exe
```

Use `-A Win32` in a separate build directory for x86. The workflow builds and self-tests both architectures, plus a MinGW-w64 GCC x86 build from the runner's MSYS2. [The initial Windows CI run](https://github.com/razbenaharon/TaskVitals/actions/runs/38930431512) built, self-tested and packaged MSVC x86/x64 successfully. The downloadable x86 release and interactive GUI checks use the locally verified MinGW build.

Direct build from an x64 Native Tools Command Prompt:

```bat
mkdir release
rc /nologo /fo release\TaskVitals.res src\TaskVitals.rc
cl /nologo /O2 /W4 /MT /EHsc /utf-8 src\main.cpp release\TaskVitals.res /Fo:release\main.obj /Fe:release\TaskVitals.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib advapi32.lib comctl32.lib imm32.lib oleacc.lib ole32.lib oleaut32.lib shell32.lib uuid.lib
```

## MinGW / MinGW-w64

Put g++ on PATH, then run from PowerShell:

```powershell
.\build-mingw.ps1
.\release\TaskVitals.exe
```

Or create the `release` and `build` directories and use the tools directly:

```powershell
windres -I src src/TaskVitals.rc -O coff -o build/TaskVitals.res.o
g++ -std=gnu++11 -O2 -Wall -Wextra -finput-charset=UTF-8 -mwindows -static -static-libgcc -static-libstdc++ -s src/main.cpp build/TaskVitals.res.o -o release/TaskVitals.exe -luser32 -lgdi32 -ladvapi32 -lcomctl32 -limm32 -loleacc -lole32 -loleaut32 -lshell32 -luuid
```

The executable architecture follows the compiler. The supplied x86 release was built with MinGW.org GCC 6.3.0 and imports only Windows system DLLs. Current MinGW-w64 should support the same source, but was not used for the supplied release.

## Package a portable release

```powershell
.\scripts\package-release.ps1 -Executable .\release\TaskVitals.exe
```

The script reads the project version and the executable's PE architecture, runs the built-in self-test, and writes a portable ZIP and SHA-256 checksum under `dist/`. The ZIP contains the executable, a short user guide, the license and changelog. The script does not upload or publish anything.

The application icon is `src/TaskVitals.ico`, embedded through `src/TaskVitals.rc`. To change it, edit and run `python scripts/make-icon.py` (requires Pillow); it rewrites the icon in all sizes.

GPU monitoring uses `src/gpu.h` and dynamically resolves PDH from the Windows system directory only when enabled. Do not add a static PDH link or vendor SDK dependency. Minimal ABI declarations and compile-time size/offset checks support the older MinGW SDK used for the portable build.
