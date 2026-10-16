$ErrorActionPreference = "Stop"
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type @"
using System; using System.Runtime.InteropServices;
public static class PlacementProbe {
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string c,string n);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h,out R r);
 [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h,IntPtr after,int x,int y,int w,int height,uint flags);
 public struct R {public int L,T,Right,B;}
}
"@
$h=[PlacementProbe]::FindWindow('TaskVitals.Window.1','TaskVitals')
$bar=[PlacementProbe]::FindWindow('Shell_TrayWnd',$null)
if($h -eq [IntPtr]::Zero -or $bar -eq [IntPtr]::Zero) { throw "Widget and taskbar must be running" }
$prefs=Get-ItemProperty HKCU:\Software\TaskVitals
if($prefs.Taskbar -ne 1) { throw "Enable Taskbar view before this test" }
$original=New-Object PlacementProbe+R; [void][PlacementProbe]::GetWindowRect($h,[ref]$original)
try {
foreach($size in 301,302,303) {
 [void][PlacementProbe]::SendMessage($h,0x111,[IntPtr]$size,[IntPtr]::Zero)
 foreach($position in 401,403) {
  [void][PlacementProbe]::SendMessage($h,0x111,[IntPtr]$position,[IntPtr]::Zero)
  $r=New-Object PlacementProbe+R; [void][PlacementProbe]::GetWindowRect($h,[ref]$r)
  $root=[Windows.Automation.AutomationElement]::FromHandle($bar)
  $buttons=$root.FindAll([Windows.Automation.TreeScope]::Descendants,(New-Object Windows.Automation.PropertyCondition ([Windows.Automation.AutomationElement]::ControlTypeProperty),([Windows.Automation.ControlType]::Button)))
  foreach($button in $buttons){ $c=$button.Current; $b=$c.BoundingRectangle
   if(-not $c.IsOffscreen -and $b.Width -gt 0 -and $r.L -lt $b.Right -and $r.Right -gt $b.Left -and $r.T -lt $b.Bottom -and $r.B -gt $b.Top){ throw "Overlapping $($c.Name) size=$size position=$position" }
  }
  Write-Output "No button overlap: size=$size position=$position bounds=$($r.L),$($r.T),$($r.Right),$($r.B) buttons=$($buttons.Count)"
 }
}
} finally {
 $scaleCommand=switch($prefs.TextScale) {85 {301}; 120 {303}; default {302}}
 [void][PlacementProbe]::SendMessage($h,0x111,[IntPtr]$scaleCommand,[IntPtr]::Zero)
 if($prefs.Anchor -eq 3) {
  [void][PlacementProbe]::SetWindowPos($h,[IntPtr]::Zero,$original.L,$original.T,0,0,0x15)
 }
 [void][PlacementProbe]::SendMessage($h,0x111,[IntPtr](401+$prefs.Anchor),[IntPtr]::Zero)
}
