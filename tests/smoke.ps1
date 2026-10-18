param([string]$Executable = (Join-Path $PSScriptRoot '..\release\TaskVitals.exe'), [string]$EvidenceDirectory = (Join-Path $PSScriptRoot '..\build\verification'))
$ErrorActionPreference = 'Stop'
$Executable = (Resolve-Path -LiteralPath $Executable).Path
New-Item -ItemType Directory -Force -Path $EvidenceDirectory | Out-Null
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class MeterTest {
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string c, string t);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll")] public static extern int GetWindowLong(IntPtr h, int n);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out Rect r);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
 [DllImport("user32.dll")] public static extern uint GetGuiResources(IntPtr h, uint flags);
 public struct Rect { public int Left,Top,Right,Bottom; }
}
'@
function Assert($condition, $message) { if (-not $condition) { throw $message } }
function Capture($handle, $name) {
    $r = New-Object MeterTest+Rect
    [void][MeterTest]::GetWindowRect($handle,[ref]$r)
    $image = New-Object Drawing.Bitmap ($r.Right-$r.Left),($r.Bottom-$r.Top)
    $graphics = [Drawing.Graphics]::FromImage($image)
    $dc = $graphics.GetHdc()
    try { Assert ([MeterTest]::PrintWindow($handle,$dc,2)) 'Window capture failed' }
    finally { $graphics.ReleaseHdc($dc); $graphics.Dispose() }
    try { $image.Save((Join-Path $EvidenceDirectory $name),[Drawing.Imaging.ImageFormat]::Png) }
    finally { $image.Dispose() }
}
Assert ([MeterTest]::FindWindow('TaskVitals.Window.1','TaskVitals') -eq [IntPtr]::Zero) 'Close TaskVitals before running this test.'
$self = Start-Process -FilePath $Executable -ArgumentList '--self-test' -WindowStyle Hidden -PassThru -Wait
Assert ($self.ExitCode -eq 0) "Sampling self-test failed: $($self.ExitCode)"
$prefs = 'HKCU:\Software\TaskVitals'
$run = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$hadPrefs = Test-Path $prefs
$prefSnapshot = if ($hadPrefs) { Get-ItemProperty $prefs }
$oldRun = (Get-ItemProperty $run -ErrorAction SilentlyContinue).TaskVitals
$process = $null
try {
    $process = Start-Process -FilePath $Executable -WindowStyle Hidden -PassThru
    Start-Sleep -Seconds 3
    $handle = [MeterTest]::FindWindow('TaskVitals.Window.1','TaskVitals')
    Assert ($handle -ne [IntPtr]::Zero) 'Widget did not open'
    Assert (([MeterTest]::GetWindowLong($handle,-20) -band 8) -eq 8) 'Window is not topmost'
    # Optional columns widen the strip; turn them off for the compact-size check.
    foreach ($option in @(@('GpuEnabled',601),@('LimitEnabled',602),@('TemperatureEnabled',603))) {
        if ((Get-ItemPropertyValue $prefs $option[0]) -eq 1) {
            [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]$option[1],[IntPtr]::Zero)
        }
    }
    # Force compact mode for this check regardless of the user's saved mode.
    if ((Get-ItemPropertyValue $prefs Taskbar) -ne 1) {
        [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]104,[IntPtr]::Zero)
    }
    $compactRect = New-Object MeterTest+Rect
    [void][MeterTest]::GetWindowRect($handle,[ref]$compactRect)
    Assert (($compactRect.Right-$compactRect.Left) -lt 320) 'Compact mode did not shrink the window'
    # Exercise actual geometry and font replacement, not just preference writes.
    $widths=@()
    for($i=0;$i -lt 3;$i++) {
        [void][MeterTest]::SendMessage($handle,0x111,[IntPtr](301+$i),[IntPtr]::Zero)
        $sizeRect=New-Object MeterTest+Rect
        [void][MeterTest]::GetWindowRect($handle,[ref]$sizeRect)
        $widths+=($sizeRect.Right-$sizeRect.Left)
        Capture $handle "size-$i.png"
    }
    Assert ($widths[0] -lt $widths[1] -and $widths[1] -lt $widths[2]) 'Text sizes did not change widget geometry'
    Start-Sleep -Milliseconds 500
    $fontGdiBefore=[MeterTest]::GetGuiResources($process.Handle,0)
    for($i=0;$i -lt 30;$i++) {
        [void][MeterTest]::SendMessage($handle,0x111,[IntPtr](301+($i%3)),[IntPtr]::Zero)
    }
    Start-Sleep -Milliseconds 500
    $fontGdiAfter=[MeterTest]::GetGuiResources($process.Handle,0)
    Assert ($fontGdiAfter -le $fontGdiBefore+1) "Repeated text-size changes grew settled GDI resources: $fontGdiBefore to $fontGdiAfter"
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]401,[IntPtr]::Zero)
    $rightRect=New-Object MeterTest+Rect
    [void][MeterTest]::GetWindowRect($handle,[ref]$rightRect)
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]405,[IntPtr]::Zero)
    $movedRect=New-Object MeterTest+Rect
    [void][MeterTest]::GetWindowRect($handle,[ref]$movedRect)
    Assert ($movedRect.Left -lt $rightRect.Left) 'Move-left command did not move the strip'
    Assert ((Get-ItemPropertyValue $prefs Anchor) -eq 3) 'Custom position was not recorded'
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]402,[IntPtr]::Zero)
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]403,[IntPtr]::Zero)
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]503,[IntPtr]::Zero)
    # Verify both new preferences survive a real process restart.
    [void][MeterTest]::SendMessage($handle,0x10,[IntPtr]::Zero,[IntPtr]::Zero)
    Assert ($process.WaitForExit(3000)) 'Could not stop the first test instance'
    $process=Start-Process -FilePath $Executable -WindowStyle Hidden -PassThru
    Start-Sleep -Seconds 1
    $handle=[MeterTest]::FindWindow('TaskVitals.Window.1','TaskVitals')
    Assert ($handle -ne [IntPtr]::Zero) 'Restart failed'
    Assert ((Get-ItemPropertyValue $prefs TextScale) -eq 120) 'Text size was not restored on startup'
    Assert ((Get-ItemPropertyValue $prefs Anchor) -eq 2) 'Position was not restored on startup'
    Assert ((Get-ItemPropertyValue $prefs CpuColor) -eq 0xFFBE6E) 'CPU color was not restored on startup'
    Assert ((Get-ItemPropertyValue $prefs RamColor) -eq 0xD2DC50) 'RAM color was not restored on startup'
    Assert ((Get-ItemPropertyValue $prefs ValueColor) -eq 0xFFF4E6) 'Percentage color was not restored on startup'
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]502,[IntPtr]::Zero)
    Assert ((Get-ItemPropertyValue $prefs CpuColor) -eq 0xF5F5F5 -and (Get-ItemPropertyValue $prefs RamColor) -eq 0xF5F5F5 -and (Get-ItemPropertyValue $prefs ValueColor) -eq 0xF5F5F5) 'White palette was not applied'
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]501,[IntPtr]::Zero)
    $restoredRect=New-Object MeterTest+Rect
    [void][MeterTest]::GetWindowRect($handle,[ref]$restoredRect)
    Assert (($restoredRect.Right-$restoredRect.Left) -eq $widths[2]) 'Restored size is incorrect'
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]302,[IntPtr]::Zero)
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]401,[IntPtr]::Zero)
    Capture $handle 'taskbar.png'
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]104,[IntPtr]::Zero)
    Assert ((Get-ItemPropertyValue $prefs Taskbar) -eq 0) 'Desktop mode toggle failed'
    $second = Start-Process -FilePath $Executable -WindowStyle Hidden -PassThru -Wait
    Assert ($second.ExitCode -eq 0) 'Second instance failed'
    Start-Sleep -Seconds 5
    Capture $handle 'desktop.png'
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]503,[IntPtr]::Zero)
    Capture $handle 'colors.png'
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]501,[IntPtr]::Zero)
    # Exercise every user-facing cadence and leave the default selected.
    $intervals=@(1000,2000,5000,10000)
    for($i=0;$i -lt $intervals.Count;$i++) {
        [void][MeterTest]::SendMessage($handle,0x111,[IntPtr](201+$i),[IntPtr]::Zero)
        Assert ((Get-ItemPropertyValue $prefs RefreshMs) -eq $intervals[$i]) 'Refresh interval was not saved'
    }
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]203,[IntPtr]::Zero)
    # A temporary startup test is reverted in finally, including an existing entry.
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]102,[IntPtr]::Zero)
    $current = (Get-ItemProperty $run -ErrorAction SilentlyContinue).TaskVitals
    if ($oldRun -eq ('"' + $Executable + '"')) {
        Assert ($null -eq $current) 'Startup disable failed'
    } else {
        Assert ($current -eq ('"' + $Executable + '"')) 'Startup command is not correctly quoted'
        [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]102,[IntPtr]::Zero)
        Assert ($null -eq (Get-ItemProperty $run -ErrorAction SilentlyContinue).TaskVitals) 'Startup disable failed'
    }
    # Let deferred GDI cleanup settle after the menu/view changes.
    Start-Sleep -Milliseconds 500
    $process.Refresh()
    $cpuBefore = $process.TotalProcessorTime.TotalMilliseconds
    $gdiBefore = [MeterTest]::GetGuiResources($process.Handle,0)
    $userBefore = [MeterTest]::GetGuiResources($process.Handle,1)
    Start-Sleep -Seconds 12
    $process.Refresh()
    $gdiAfter = [MeterTest]::GetGuiResources($process.Handle,0)
    $userAfter = [MeterTest]::GetGuiResources($process.Handle,1)
    Assert ($gdiAfter -le $gdiBefore+1) 'GDI resources are growing'
    Assert ($userAfter -le $userBefore+1) 'USER resources are growing'
    $report = [ordered]@{
        selfTest='passed'; singleInstance='passed'; alwaysOnTop='passed'; startupToggle='passed'; compactToggle='passed'; refreshChoices='passed'
        textSizes='passed'; repeatedFontReplacement='passed'; positioning='passed'; sizeAndPositionPersistence='passed'
        colorPresetsAndPersistence='passed'
        workingSetMiB=[Math]::Round($process.WorkingSet64/1MB,2)
        privateMemoryMiB=[Math]::Round($process.PrivateMemorySize64/1MB,2)
        cpuMillisecondsOver12Seconds=[Math]::Round($process.TotalProcessorTime.TotalMilliseconds-$cpuBefore,2)
        gdiBefore=$gdiBefore; gdiAfter=$gdiAfter; userBefore=$userBefore; userAfter=$userAfter
    }
    $report | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $EvidenceDirectory 'results.json')
    $report | ConvertTo-Json
    [void][MeterTest]::SendMessage($handle,0x111,[IntPtr]103,[IntPtr]::Zero)
    Assert ($process.WaitForExit(3000)) 'Widget did not exit'
} finally {
    if ($process -and -not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    if ($null -eq $oldRun) { Remove-ItemProperty $run TaskVitals -ErrorAction SilentlyContinue }
    else { New-ItemProperty $run TaskVitals -Value $oldRun -PropertyType String -Force | Out-Null }
    if (-not $hadPrefs) { Remove-Item $prefs -ErrorAction SilentlyContinue }
    else {
        foreach ($name in @('X','Y','Taskbar','RefreshMs','TextScale','Anchor','TaskbarOffset','CpuColor','RamColor','ValueColor','GpuEnabled','LimitEnabled','TemperatureEnabled')) {
            if ($null -ne $prefSnapshot.$name) { New-ItemProperty $prefs $name -Value $prefSnapshot.$name -PropertyType DWord -Force | Out-Null }
            else { Remove-ItemProperty $prefs $name -ErrorAction SilentlyContinue }
        }
    }
}
