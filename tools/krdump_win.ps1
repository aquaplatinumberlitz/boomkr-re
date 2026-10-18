# Dump bộ nhớ CA.exe (KR) trong Windows — bản 64-bit-safe.
# Bản VN (tools/live_dump.ps1) lỗi OverflowException vì cast BaseAddress.ToInt32(); ở đây dùng Int64.
# Chạy trong guest:
#   powershell -ExecutionPolicy Bypass -File \\host.lan\Data\boomkr-re\tools\krdump_win.ps1 `
#       -Exe C:\boomkr\CA.exe -OutDir C:\krdump2 -WaitMs 1200
param([string]$Exe, [string]$OutDir, [int]$WaitMs = 1200, [string]$Shots = "", [int]$Target = 0)

$ErrorActionPreference = "Stop"
$src = @"
using System; using System.Runtime.InteropServices; using System.IO; using System.Diagnostics; using System.Text;
public static class KrDump {
  [DllImport("kernel32")] static extern IntPtr OpenProcess(int a, bool i, int pid);
  [DllImport("kernel32")] static extern bool ReadProcessMemory(IntPtr h, IntPtr a, byte[] b, IntPtr n, out IntPtr r);
  [DllImport("kernel32")] static extern IntPtr VirtualQueryEx(IntPtr h, IntPtr a, out MBI32 m, IntPtr l);
  // Process 32-bit (WOW64) phải dùng MEMORY_BASIC_INFORMATION32: 28 byte
  [StructLayout(LayoutKind.Sequential)] public struct MBI32 { public uint Base, AllocBase, AllocProt, Size, State, Protect, Type; }
  [StructLayout(LayoutKind.Sequential)] public struct MBI64 { public IntPtr Base, AllocBase; public IntPtr Size; public int State, Protect, Type; }
  public static byte[] Read(IntPtr h, long a, int n) {
    var b = new byte[n]; IntPtr r;
    if (!ReadProcessMemory(h, new IntPtr(a), b, new IntPtr(n), out r)) return null;
    return b;
  }
  public static void Dump(int pid, string dir) {
    Directory.CreateDirectory(dir);
    IntPtr h = OpenProcess(0x0410, false, pid);
    if (h == IntPtr.Zero) throw new Exception("OpenProcess failed");
    var p = Process.GetProcessById(pid);
    var mods = new StringBuilder();
    foreach (ProcessModule m in p.Modules) {
      long b = m.BaseAddress.ToInt64(); int sz = m.ModuleMemorySize;
      mods.AppendLine(m.ModuleName + "\t" + b.ToString("x") + "\t" + sz.ToString("x"));
      try {
        if (m.ModuleName.ToLower() == "ca.exe" || m.ModuleName.ToLower().EndsWith(".dll")) {
          var img = Read(h, b, sz);
          if (img != null) File.WriteAllBytes(Path.Combine(dir, "mod_" + b.ToString("x8") + "_" + m.ModuleName + ".bin"), img);
        }
      } catch {}
    }
    File.WriteAllText(Path.Combine(dir, "modules.tsv"), mods.ToString());
    var rs = new StringBuilder();
    long a2 = 0x10000; MBI32 mbi; int len = Marshal.SizeOf(typeof(MBI32));
    while (a2 < 0x7FFF0000) {
      if (VirtualQueryEx(h, new IntPtr(a2), out mbi, new IntPtr(len)) == IntPtr.Zero) break;
      long b2 = mbi.Base; long sz2 = mbi.Size;
      if (sz2 <= 0) break;
      bool commit = (mbi.State & 0x1000) != 0;
      bool exec = (mbi.Protect & 0xF0) != 0;
      long image = mbi.Type & 0x1000000;
      if (commit && (exec || image != 0) && sz2 <= 0x4000000) {
        try {
          var buf = Read(h, b2, (int)sz2);
          if (buf != null) {
            File.WriteAllBytes(Path.Combine(dir, "reg_" + b2.ToString("x8") + ".bin"), buf);
            rs.AppendLine(b2.ToString("x") + "\t" + sz2.ToString("x") + "\t" + mbi.Protect.ToString("x"));
          }
        } catch {}
      }
      a2 = b2 + sz2;
    }
    File.WriteAllText(Path.Combine(dir, "regions.tsv"), rs.ToString());
  }
}
"@
Add-Type -TypeDefinition $src -ReferencedAssemblies System.Diagnostics.Process
if ($Target -ne 0) {
  try { [KrDump]::Dump($Target, $OutDir); Write-Host "dump pid $Target xong -> $OutDir" }
  catch { Write-Host "lỗi: $_" }
  Get-ChildItem $OutDir -Recurse -File | Measure-Object -Property Length -Sum | ForEach-Object {
    Write-Host ("tổng {0} file, {1:N0} B" -f $_.Count, $_.Sum) }
  return
}

# -Shots "300,600,900,1200": dump module ảnh nhiều mốc (giữ được trạng thái sau unpack trước khi thoát)
if ($Shots -ne "") {
  $times = $Shots.Split(",") | ForEach-Object { [int]$_ }
  $proc = Start-Process -FilePath $Exe -WorkingDirectory (Split-Path $Exe) -PassThru
  $t0 = Get-Date
  foreach ($t in $times) {
    $el = ((Get-Date) - $t0).TotalMilliseconds
    if ($t -gt $el) { Start-Sleep -Milliseconds ([int]($t - $el)) }
    if ($proc.HasExited) { Write-Host "shot ${t}: process đã thoát"; continue }
    try {
      [KrDump]::Dump($proc.Id, (Join-Path $OutDir ("shot" + $t)))
      Write-Host "shot ${t} ms: ok"
    } catch { Write-Host "shot ${t} ms: lỗi $_" }
  }
  if (-not $proc.HasExited) { $proc.Kill() }
  Get-ChildItem $OutDir -Recurse -File | Measure-Object -Property Length -Sum | ForEach-Object {
    Write-Host ("tổng {0} file, {1:N0} B" -f $_.Count, $_.Sum)
  }
  return
}
$p = Start-Process -FilePath $Exe -WorkingDirectory (Split-Path $Exe) -PassThru
Start-Sleep -Milliseconds $WaitMs
try {
  if ($p.HasExited) { Write-Host "process đã thoát trước khi dump (exitcode=$($p.ExitCode))" }
  [KrDump]::Dump($p.Id, $OutDir)
  Write-Host "dump xong -> $OutDir"
} catch {
  Write-Host "lỗi: $_"
} finally {
  if (-not $p.HasExited) { $p.Kill() }
}
Get-ChildItem $OutDir | Measure-Object -Property Length -Sum | ForEach-Object {
  Write-Host ("{0} file, {1:N0} B" -f $_.Count, $_.Sum)
}
