$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force -Path release | Out-Null
    & g++ -std=gnu++11 -O2 -Wall -Wextra -finput-charset=UTF-8 -mwindows -static -static-libgcc -static-libstdc++ -s src/main.cpp -o release/TaskVitals.exe -luser32 -lgdi32 -ladvapi32 -lcomctl32 -limm32 -loleacc -lole32 -loleaut32 -luuid
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
    Write-Host 'Built release\TaskVitals.exe'
} finally { Pop-Location }
