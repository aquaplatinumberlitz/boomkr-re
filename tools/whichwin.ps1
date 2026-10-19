Add-Type @"
using System; using System.Text; using System.Runtime.InteropServices;
public class W2 {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowTextW(IntPtr h, [Out] char[] s, int n);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassNameW(IntPtr h, [Out] char[] s, int n);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
}
"@
$script:lines = New-Object System.Collections.ArrayList
[W2]::EnumWindows({
  param($h,$l)
  if ([W2]::IsWindowVisible($h)) {
    $t = New-Object char[] 512; $n1 = [W2]::GetWindowTextW($h,$t,512)
    $c = New-Object char[] 256; $n2 = [W2]::GetClassNameW($h,$c,256)
    $pid2 = 0; [void][W2]::GetWindowThreadProcessId($h,[ref]$pid2)
    if ($n1 -gt 0) {
      $p = (Get-Process -Id $pid2 -ErrorAction SilentlyContinue).ProcessName
      [void]$script:lines.Add("PID=$pid2 PROC=$p CLASS=$(-join $c[0..($n2-1)]) TEXT=$(-join $t[0..($n1-1)])")
    }
  }
  return $true
}, [IntPtr]::Zero) | Out-Null
$script:lines | Out-File -Encoding UTF8 C:\whichwin.txt
Get-Content C:\whichwin.txt
