Add-Type @"
using System; using System.Runtime.InteropServices;
public class W3 {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr p, EnumProc cb, IntPtr l);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowTextW(IntPtr h, [Out] char[] s, int n);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassNameW(IntPtr h, [Out] char[] s, int n);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
}
"@
$script:out = New-Object System.Collections.ArrayList
$target = [IntPtr]::Zero
[W3]::EnumWindows({
  param($h,$l)
  $pid2 = 0; [void][W3]::GetWindowThreadProcessId($h,[ref]$pid2)
  $p = (Get-Process -Id $pid2 -ErrorAction SilentlyContinue).ProcessName
  if ($p -eq "NewPatcher") { $script:target = $h }
  return $true
}, [IntPtr]::Zero) | Out-Null
[void]$script:out.Add("dialog hwnd=$($script:target)")
[W3]::EnumChildWindows($script:target, {
  param($h,$l)
  $t = New-Object char[] 1024; $n1 = [W3]::GetWindowTextW($h,$t,1024)
  $c = New-Object char[] 256; $n2 = [W3]::GetClassNameW($h,$c,256)
  if ($n1 -gt 0) { [void]$script:out.Add("  CLASS=$(-join $c[0..($n2-1)]) TEXT=$(-join $t[0..($n1-1)])") }
  return $true
}, [IntPtr]::Zero) | Out-Null
$script:out | Out-File -Encoding UTF8 C:\childwin.txt
Get-Content C:\childwin.txt
