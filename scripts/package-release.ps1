param(
    [string]$Executable = (Join-Path $PSScriptRoot '..\release\TaskVitals.exe'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\dist')
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$Executable = (Resolve-Path -LiteralPath $Executable).Path
$project = Get-Content -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt') -Raw
$match = [regex]::Match($project, '\bVERSION\s+(\d+\.\d+\.\d+)')
if (-not $match.Success) { throw 'Could not read the project version.' }
$version = $match.Groups[1].Value

# Derive the architecture from the binary rather than trusting a filename.
$bytes = [IO.File]::ReadAllBytes($Executable)
if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) { throw 'Not a PE executable.' }
$peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
if ($peOffset -lt 0 -or $peOffset -gt $bytes.Length - 6 -or [BitConverter]::ToUInt32($bytes, $peOffset) -ne 0x4550) {
    throw 'Invalid PE signature.'
}
$machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
$architecture = switch ($machine) {
    0x14c { 'x86' }
    0x8664 { 'x64' }
    default { throw 'Only x86 and x64 release packaging is supported.' }
}
$check = Start-Process -FilePath $Executable -ArgumentList '--self-test' -WindowStyle Hidden -PassThru -Wait
if ($check.ExitCode -ne 0) { throw "Sampling self-test failed: $($check.ExitCode)" }

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$buildRoot = Join-Path $projectRoot 'build'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$staging = Join-Path $buildRoot ('package-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $staging | Out-Null
try {
    Copy-Item -LiteralPath $Executable -Destination (Join-Path $staging 'TaskVitals.exe')
    Copy-Item -LiteralPath (Join-Path $projectRoot 'LICENSE') -Destination $staging
    Copy-Item -LiteralPath (Join-Path $projectRoot 'CHANGELOG.md') -Destination $staging
    Copy-Item -LiteralPath (Join-Path $projectRoot 'THIRD-PARTY-NOTICES.txt') -Destination $staging
    @"
TaskVitals $version - Windows $architecture portable build

Extract this folder to a permanent location and run TaskVitals.exe.
The interface is English-only. No installer or separate runtime is needed.

Right-click the visible text for options and Exit.
Hover over the text for CPU/RAM details and uptime.
Use Show GPU (optional) to enable GPU usage; it is off by default.
GPU uses the existing refresh interval and adds a third column.
Unavailable GPU counters show --. Disabling GPU releases its query.
Use Show CPU speed limit (optional) to see the performance Windows
guarantees, as a share of the processor's base performance; power policy,
power budget or heat lower it. It is off by default.
Use Show CPU temperature (needs Core Temp) to show the hottest core while
the separate Core Temp utility is running; it is off by default.
TaskVitals installs no driver and only reads what Core Temp publishes.
Core Temp is an independent product by ALCPU under its own license (free
for personal use; businesses need its commercial license). Download
version 1.21 or later only from https://www.alcpu.com/CoreTemp/
Temperatures are informational and up to one refresh interval old.
TaskVitals gives no overheating warnings; do not rely on it for safety.
Use Text size for Small, Normal or Large.
Use Colors for five presets or custom label, bar and percentage colors.
Use Taskbar position for presets and small adjustments.
Hold Ctrl and drag the visible text for custom placement.
Refresh interval defaults to 5 seconds; 1, 2, 5 and 10 are available.
Uncheck Taskbar view for draggable desktop cards.
Start with Windows is optional and off by default.

The taskbar strip uses free gaps on the primary horizontal taskbar.
If no gap fits or controls cannot be read, it moves just above the taskbar.
It does not reserve shell space.
The executable is unsigned. Read LICENSE for the MIT license and
THIRD-PARTY-NOTICES.txt for the statically linked runtime libraries.

Source, documentation and issues:
https://github.com/razbenaharon/TaskVitals

To remove: uncheck Start with Windows, exit, then delete this folder.
Preferences: HKEY_CURRENT_USER\Software\TaskVitals
"@ | Set-Content -LiteralPath (Join-Path $staging 'README.txt') -Encoding ascii
    $fileName = "TaskVitals-$version-windows-$architecture.zip"
    $archive = Join-Path $OutputDirectory $fileName
    Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $archive -Force
    $hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $fileName" | Set-Content -LiteralPath ($archive + '.sha256') -Encoding ascii
    Write-Output "Packaged $archive"
    Write-Output "SHA256 $hash"
} finally {
    # Only remove the fresh staging directory beneath this project's build folder.
    $resolvedStaging = [IO.Path]::GetFullPath($staging)
    $buildPrefix = [IO.Path]::GetFullPath($buildRoot).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if ($resolvedStaging.StartsWith($buildPrefix, [StringComparison]::OrdinalIgnoreCase) -and
        [IO.Path]::GetFileName($resolvedStaging) -match '^package-[0-9a-f]{32}$') {
        Remove-Item -LiteralPath $resolvedStaging -Recurse -Force
    }
}
