param([string]$Executable=(Join-Path $PSScriptRoot '..\release\TaskVitals.exe'))
$ErrorActionPreference='Stop'
$Executable=(Resolve-Path -LiteralPath $Executable).Path
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;
public static class NetworkTest {
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]public static extern IntPtr FindWindow(string c,string t);
 [DllImport("user32.dll")]public static extern IntPtr SendMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
 [DllImport("user32.dll")]public static extern bool GetWindowRect(IntPtr h,out R r);
 [DllImport("user32.dll")]public static extern bool PrintWindow(IntPtr h,IntPtr dc,uint flags);
 public struct R {public int L,T,Right,B;}
}
"@
function Assert($condition,$message){if(-not $condition){throw $message}}
$prefs='HKCU:\Software\TaskVitals'
Assert ([NetworkTest]::FindWindow('TaskVitals.Window.1','TaskVitals') -eq [IntPtr]::Zero) 'Close TaskVitals before this test.'
$before=Get-ItemProperty $prefs -ErrorAction SilentlyContinue
New-Item -Path $prefs -Force | Out-Null
$names=@('NetworkEnabled','LimitEnabled','TemperatureEnabled','GpuEnabled','Taskbar','TextScale','X','Y','Anchor','TaskbarOffset','RefreshMs','CpuColor','RamColor','ValueColor')
$process=$null
function Open-Widget {
 $script:process=Start-Process -FilePath $Executable -WindowStyle Hidden -PassThru
 Start-Sleep -Seconds 2
 $script:handle=[NetworkTest]::FindWindow('TaskVitals.Window.1','TaskVitals')
 Assert ($handle -ne [IntPtr]::Zero) 'Widget did not open'
}
function Command($id){[void][NetworkTest]::SendMessage($handle,0x111,[IntPtr]$id,[IntPtr]::Zero)}
function IpHelperLoaded {$process.Refresh();return ($process.Modules.ModuleName -contains "iphlpapi.dll")}
function Width {$r=New-Object NetworkTest+R;[void][NetworkTest]::GetWindowRect($handle,[ref]$r);return ($r.Right-$r.L)}
function Capture($name) {
 $r=New-Object NetworkTest+R;[void][NetworkTest]::GetWindowRect($handle,[ref]$r)
 $image=New-Object Drawing.Bitmap ($r.Right-$r.L),($r.B-$r.T)
 $graphics=[Drawing.Graphics]::FromImage($image);$dc=$graphics.GetHdc()
 try {Assert ([NetworkTest]::PrintWindow($handle,$dc,2)) 'Capture failed'} finally {$graphics.ReleaseHdc($dc);$graphics.Dispose()}
 try {$image.Save((Join-Path $evidence $name))} finally {$image.Dispose()}
}
$evidence=Join-Path $PSScriptRoot '..\build\verification';New-Item -Path $evidence -ItemType Directory -Force | Out-Null
try {
 # A missing preference must default to disabled; other optional columns stay off.
 Remove-ItemProperty $prefs NetworkEnabled -ErrorAction SilentlyContinue
 foreach($name in 'GpuEnabled','LimitEnabled','TemperatureEnabled'){New-ItemProperty $prefs $name -Value 0 -PropertyType DWord -Force | Out-Null}
 New-ItemProperty $prefs Taskbar -Value 1 -PropertyType DWord -Force | Out-Null
 New-ItemProperty $prefs TextScale -Value 100 -PropertyType DWord -Force | Out-Null
 New-ItemProperty $prefs RefreshMs -Value 1000 -PropertyType DWord -Force | Out-Null
 Open-Widget
 Assert ((Get-ItemPropertyValue $prefs NetworkEnabled) -eq 0) 'Network traffic is not disabled by default'
 # Another system DLL may pull IP Helper in; only assert unloading when it starts unloaded.
 $loadedByDefault=IpHelperLoaded
 $offWidth=Width
 Command 604
 Assert ((Get-ItemPropertyValue $prefs NetworkEnabled) -eq 1) 'Network choice was not saved'
 Assert (IpHelperLoaded) 'Enabling network traffic did not load IP Helper'
 $onWidth=Width
 # Two columns (download and upload): 98 logical pixels each on the taskbar.
 Assert ([Math]::Abs(($onWidth-$offWidth)-2*98*$offWidth/196) -le 2) 'Network columns did not add two columns'
 Command 103;Assert ($process.WaitForExit(3000)) 'Could not close widget'
 Open-Widget
 Assert ((Width) -eq $onWidth) 'Network columns did not persist'
 Start-Sleep -Seconds 3
 Capture 'network.png'
 Command 104
 Start-Sleep -Seconds 3
 Capture 'network-desktop.png'
 Command 104
 Command 604
 Assert ((Width) -eq $offWidth) 'Disabling network traffic did not restore strip size'
 if(-not $loadedByDefault){Assert (-not (IpHelperLoaded)) 'Disabling network traffic retained IP Helper'}
 for($i=0;$i -lt 3;$i++){Command 604;Command 604}
 if(-not $loadedByDefault){Assert (-not (IpHelperLoaded)) 'Repeated toggles retained IP Helper'}
 [ordered]@{defaultOff='passed';toggleLoadsIpHelper='passed';twoColumns='passed';restartPersistence='passed';disableRestoresWidth='passed';ipHelperLoadedByDefault=$loadedByDefault} | ConvertTo-Json | Tee-Object -FilePath (Join-Path $evidence 'network-results.json')
} finally {
 if($process -and -not $process.HasExited){Command 103;if(-not $process.WaitForExit(3000)){$process.Kill()}}
 foreach($name in $names){if($null -ne $before.$name){New-ItemProperty $prefs $name -Value $before.$name -PropertyType DWord -Force | Out-Null}else{Remove-ItemProperty $prefs $name -ErrorAction SilentlyContinue}}
}
