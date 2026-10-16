param([int]$Seconds=60,[string]$OutputFile='measurement.json')
$ErrorActionPreference='Stop'
if($Seconds -lt 10) { throw 'Measure at least 10 seconds.' }
$instances=@(Get-Process TaskVitals -ErrorAction Stop)
if($instances.Count -ne 1) { throw 'Exactly one TaskVitals process must be running.' }
$process=$instances[0]
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class MeterResources {
 [DllImport("user32.dll")] public static extern uint GetGuiResources(IntPtr h,uint flag);
 [DllImport("kernel32.dll")] public static extern bool QueryProcessCycleTime(IntPtr h,out ulong cycles);
}
'@
$process.Refresh()
$cpuBefore=$process.TotalProcessorTime.TotalMilliseconds
[uint64]$cyclesBefore=0
if(-not [MeterResources]::QueryProcessCycleTime($process.Handle,[ref]$cyclesBefore)) { throw 'Could not read process cycles.' }
$gdiBefore=[MeterResources]::GetGuiResources($process.Handle,0)
$userBefore=[MeterResources]::GetGuiResources($process.Handle,1)
$handlesBefore=$process.HandleCount
$working=@($process.WorkingSet64)
$private=@($process.PrivateMemorySize64)
$clock=[Diagnostics.Stopwatch]::StartNew()
while($clock.Elapsed.TotalSeconds -lt $Seconds) {
    Start-Sleep -Seconds 5
    $process.Refresh()
    $working+=$process.WorkingSet64
    $private+=$process.PrivateMemorySize64
}
$clock.Stop()
$process.Refresh()
[uint64]$cyclesAfter=0
if(-not [MeterResources]::QueryProcessCycleTime($process.Handle,[ref]$cyclesAfter)) { throw 'Could not read process cycles.' }
$cpuMs=$process.TotalProcessorTime.TotalMilliseconds-$cpuBefore
$report=[ordered]@{
    processId=$process.Id
    durationSeconds=[Math]::Round($clock.Elapsed.TotalSeconds,2)
    cpuMilliseconds=$cpuMs
    cpuPercentOfOneLogicalProcessor=[Math]::Round($cpuMs/($clock.Elapsed.TotalSeconds*10),4)
    cpuPercentOfWholeMachine=[Math]::Round($cpuMs/($clock.Elapsed.TotalSeconds*10*[Environment]::ProcessorCount),4)
    processCycles=($cyclesAfter-$cyclesBefore)
    workingSetMiBMin=[Math]::Round(($working | Measure-Object -Minimum).Minimum/1MB,2)
    workingSetMiBMax=[Math]::Round(($working | Measure-Object -Maximum).Maximum/1MB,2)
    privateMiBMin=[Math]::Round(($private | Measure-Object -Minimum).Minimum/1MB,2)
    privateMiBMax=[Math]::Round(($private | Measure-Object -Maximum).Maximum/1MB,2)
    gdiBefore=$gdiBefore; gdiAfter=[MeterResources]::GetGuiResources($process.Handle,0)
    userBefore=$userBefore; userAfter=[MeterResources]::GetGuiResources($process.Handle,1)
    handlesBefore=$handlesBefore; handlesAfter=$process.HandleCount
}
$report | ConvertTo-Json | Set-Content -LiteralPath $OutputFile
$report | ConvertTo-Json
