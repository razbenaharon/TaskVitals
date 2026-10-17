$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force -Path release,build | Out-Null
    & windres -I src src/TaskVitals.rc -O coff -o build/TaskVitals.res.o
    if ($LASTEXITCODE -ne 0) { throw 'Resource compile failed.' }
    & g++ -std=gnu++11 -O2 -Wall -Wextra -finput-charset=UTF-8 -mwindows -static -static-libgcc -static-libstdc++ -s src/main.cpp build/TaskVitals.res.o -o release/TaskVitals.exe -luser32 -lgdi32 -ladvapi32 -lcomctl32 -limm32 -loleacc -lole32 -loleaut32 -lshell32 -luuid
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
    Write-Host 'Built release\TaskVitals.exe'
} finally { Pop-Location }
