# (by PID: two copies) Bring the game window to the front, then press keys (SendInput scan codes) only if it is in front.
param([int]$GamePid, [string[]]$Keys, [int]$HoldMs = 80, [int]$GapMs = 400)
Add-Type @'
using System; using System.Runtime.InteropServices;
public static class K {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint type);
}
'@
$p = Get-Process -Id $GamePid -ErrorAction Stop | Where-Object { $_.MainWindowHandle -ne 0 }
if (-not $p) { throw 'no game window' }
$h = $p.MainWindowHandle
if ([K]::GetForegroundWindow() -ne $h) { [K]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero); [K]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero) }; [K]::SetForegroundWindow($h) | Out-Null; Start-Sleep -Milliseconds 300
if ([K]::GetForegroundWindow() -ne $h) { throw 'game window is not in front; no keys sent' }
$vk = @{ ENTER=0x0D; ESC=0x1B; SPACE=0x20; UP=0x26; DOWN=0x28; LEFT=0x25; RIGHT=0x27; F=0x46; W=0x57; S=0x53; A=0x41; D=0x44; E=0x45; F8=0x77; F9=0x78; F10=0x79; CTRL=0x11; TAB=0x09; ALT=0x12; SHIFT=0x10; F6=0x75; F7=0x76; HOME=0x24; F12=0x7B; F1=0x70; F2=0x71; F3=0x72; F4=0x73; F5=0x74; F11=0x7A; R=0x52; Q=0x51; U=0x55; H=0x48 }
foreach ($k in $Keys) {
    if ([K]::GetForegroundWindow() -ne $h) { throw "game lost focus before $k" }
    $parts = $k.Split('+')
    $codes = $parts | ForEach-Object { if ($vk.ContainsKey($_)) { [byte]$vk[$_] } else { [byte][char]$_ } }
    foreach ($c in $codes) { [K]::keybd_event($c, [byte][K]::MapVirtualKey($c, 0), 0, [UIntPtr]::Zero) }
    Start-Sleep -Milliseconds $HoldMs
    [array]::Reverse($codes)
    foreach ($c in $codes) { [K]::keybd_event($c, [byte][K]::MapVirtualKey($c, 0), 2, [UIntPtr]::Zero) }
    Start-Sleep -Milliseconds $GapMs
}
"sent: $($Keys -join ' ')"
