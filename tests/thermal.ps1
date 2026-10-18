param([string]$Executable=(Join-Path $PSScriptRoot '..\release\TaskVitals.exe'))
$ErrorActionPreference='Stop'
$Executable=(Resolve-Path -LiteralPath $Executable).Path
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;
public static class ThermalTest {
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]public static extern IntPtr FindWindow(string c,string t);
 [DllImport("user32.dll")]public static extern IntPtr SendMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
 [DllImport("user32.dll")]public static extern bool GetWindowRect(IntPtr h,out R r);
 [DllImport("user32.dll")]public static extern bool PrintWindow(IntPtr h,IntPtr dc,uint flags);
 public struct R {public int L,T,Right,B;}
}
"@
function Assert($condition,$message){if(-not $condition){throw $message}}
$prefs='HKCU:\Software\TaskVitals'
Assert ([ThermalTest]::FindWindow('TaskVitals.Window.1','TaskVitals') -eq [IntPtr]::Zero) 'Close TaskVitals before this test.'
$before=Get-ItemProperty $prefs -ErrorAction SilentlyContinue
New-Item -Path $prefs -Force | Out-Null
$names=@('LimitEnabled','TemperatureEnabled','GpuEnabled','Taskbar','TextScale','X','Y','Anchor','TaskbarOffset','RefreshMs','CpuColor','RamColor','ValueColor')
$process=$null
function Open-Widget {
 $script:process=Start-Process -FilePath $Executable -WindowStyle Hidden -PassThru
 Start-Sleep -Seconds 2
 $script:handle=[ThermalTest]::FindWindow('TaskVitals.Window.1','TaskVitals')
 Assert ($handle -ne [IntPtr]::Zero) 'Widget did not open'
}
function Command($id){[void][ThermalTest]::SendMessage($handle,0x111,[IntPtr]$id,[IntPtr]::Zero)}
function PdhLoaded {$process.Refresh();return ($process.Modules.ModuleName -contains "pdh.dll")}
function Width {$r=New-Object ThermalTest+R;[void][ThermalTest]::GetWindowRect($handle,[ref]$r);return ($r.Right-$r.L)}
function CoreTempRunning {return [bool](Get-Process -Name 'Core Temp','CoreTemp' -ErrorAction SilentlyContinue)}
try {
 # Missing preferences must default to disabled; GPU off keeps PDH checks meaningful.
 Remove-ItemProperty $prefs LimitEnabled,TemperatureEnabled -ErrorAction SilentlyContinue
 New-ItemProperty $prefs GpuEnabled -Value 0 -PropertyType DWord -Force | Out-Null
 New-ItemProperty $prefs Taskbar -Value 1 -PropertyType DWord -Force | Out-Null
 New-ItemProperty $prefs TextScale -Value 100 -PropertyType DWord -Force | Out-Null
 Open-Widget
 Assert ((Get-ItemPropertyValue $prefs LimitEnabled) -eq 0) 'Speed limit is not disabled by default'
 Assert ((Get-ItemPropertyValue $prefs TemperatureEnabled) -eq 0) 'Temperature is not disabled by default'
 Assert (-not (PdhLoaded)) 'Default startup loaded PDH'
 $offWidth=Width
 Command 602
 Assert ((Get-ItemPropertyValue $prefs LimitEnabled) -eq 1) 'Speed limit choice was not saved'
 Assert ((Width) -gt $offWidth) 'Speed limit column did not expand the strip'
 $limitWidth=Width
 Command 603
 Assert ((Get-ItemPropertyValue $prefs TemperatureEnabled) -eq 1) 'Temperature choice was not saved'
 Assert ((Width) -gt $limitWidth) 'Temperature column did not expand the strip'
 # Keep both enabled over a real process restart.
 Command 103;Assert ($process.WaitForExit(3000)) 'Could not close widget'
 Open-Widget
 Assert ((Width) -gt $limitWidth) 'Optional columns did not persist'
 Assert (PdhLoaded) 'Speed limit did not load PDH after restart'
 Command 104
 Start-Sleep -Seconds 6
 $r=New-Object ThermalTest+R;[void][ThermalTest]::GetWindowRect($handle,[ref]$r)
 $image=New-Object Drawing.Bitmap ($r.Right-$r.L),($r.B-$r.T)
 $graphics=[Drawing.Graphics]::FromImage($image);$dc=$graphics.GetHdc()
 try {Assert ([ThermalTest]::PrintWindow($handle,$dc,2)) 'Capture failed'} finally {$graphics.ReleaseHdc($dc);$graphics.Dispose()}
 $evidence=Join-Path $PSScriptRoot '..\build\verification';New-Item -Path $evidence -ItemType Directory -Force | Out-Null
 try {$image.Save((Join-Path $evidence 'thermal.png'))} finally {$image.Dispose()}
 Command 602
 Assert (-not (PdhLoaded)) 'Speed limit disable retained PDH library'
 Command 603
 Command 104
 Assert ((Width) -eq $offWidth) 'Disabling both did not restore strip size'
 for($i=0;$i -lt 3;$i++){Command 602;Command 602;Command 603;Command 603}
 Assert (-not (PdhLoaded)) 'Repeated toggles retained PDH library'
 [ordered]@{defaultOff='passed';noPdhWhenOff='passed';toggleAndResize='passed';restartPersistence='passed';repeatedDisableAndUnload='passed';coreTempRunningOnHost=(CoreTempRunning)} | ConvertTo-Json | Tee-Object -FilePath (Join-Path $evidence 'thermal-results.json')
} finally {
 if($process -and -not $process.HasExited){Command 103;if(-not $process.WaitForExit(3000)){$process.Kill()}}
 foreach($name in $names){if($null -ne $before.$name){New-ItemProperty $prefs $name -Value $before.$name -PropertyType DWord -Force | Out-Null}else{Remove-ItemProperty $prefs $name -ErrorAction SilentlyContinue}}
}
