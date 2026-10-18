"""Dump image KR đã unpack từ tiến trình Wine (không cần debugger, không cần Windows VM).

Lý do: `CA.exe` (Themida) tự unpack rồi dừng ở `ProcessPatch` (treo trong WinTrust). Lúc đó image đã
giải nén trong bộ nhớ. Script này:

  1. chạy `wine CA.exe` với prefix/locale/font riêng của KR;
  2. chờ tới khi tiến trình vào `ProcessPatch` (đọc CALog.log) hoặc hết thời gian chờ;
  3. đọc `/proc/<pid>/maps` + `/proc/<pid>/mem`, lấy toàn bộ vùng của image CA.exe;
  4. ghi đè lên bản file gốc theo section table -> `out/kr-unpacked.exe`;
  5. kill tiến trình.

Dùng:
  python3 tools/krdump.py [--src DIR] [--out FILE] [--wait 6]
"""
import argparse
import os
import pathlib
import re
import shutil
import signal
import struct
import subprocess
import sys
import time

KR_ROOT = pathlib.Path(os.environ.get("BOOMKR_RUN", "/home/admin/boomkr-run"))
WINE = pathlib.Path(os.environ.get("WINE_KR", "/home/admin/wine-kr"))
HERE = pathlib.Path(__file__).resolve().parent.parent
OUT = HERE / "out"


def wine_env():
    env = dict(os.environ)
    env.update({
        "WINEPREFIX": str(WINE / "prefix"),
        "FONTCONFIG_FILE": str(WINE / "fonts.conf"),
        "LOCPATH": str(WINE / "locale"),
        "LC_ALL": "ko_KR.UTF-8",
        "LANG": "ko_KR.UTF-8",
        "WINEDEBUG": "+file",   # làm chậm có chủ ý: process sống lâu hơn để kịp dump
        "DISPLAY": env.get("DISPLAY", ":0"),
    })
    return env


def find_pid(exe_name="CA.exe", timeout=20):
    """Tìm pid của tiến trình đích (wine chạy exe như một process Linux)."""
    end = time.time() + timeout
    while time.time() < end:
        for p in pathlib.Path("/proc").iterdir():
            if not p.name.isdigit():
                continue
            try:
                cmd = (p / "cmdline").read_bytes().replace(b"\0", b" ").decode(errors="replace")
                maps = (p / "maps").read_text(errors="replace")
            except Exception:
                continue
            if exe_name.lower() in cmd.lower() and "CA.exe" in maps:
                return int(p.name)
        time.sleep(0.05)
    return None


def wait_patch_stage(log, timeout=15):
    """Chờ tới khi CALog ghi 'ProcessPatch' (image đã unpack) hoặc timeout."""
    end = time.time() + timeout
    while time.time() < end:
        try:
            if "ProcessPatch" in log.read_text(errors="replace"):
                return True
        except Exception:
            pass
        time.sleep(0.05)
    return False


def dump_image(pid, src: pathlib.Path, dst: pathlib.Path):
    """Ghi đè các section của file gốc bằng nội dung đã unpack trong bộ nhớ."""
    maps = pathlib.Path(f"/proc/{pid}/maps").read_text(errors="replace")
    segs = []
    for line in maps.splitlines():
        m = re.match(r"([0-9a-f]+)-([0-9a-f]+) (\S{4}) \S+ \S+ \S+\s*(.*)", line)
        if not m:
            continue
        lo, hi, perm, path = int(m.group(1), 16), int(m.group(2), 16), m.group(3), m.group(4).strip()
        if path.lower().endswith("ca.exe"):
            segs.append((lo, hi, perm, path))
    if not segs:
        raise SystemExit("không thấy vùng CA.exe trong maps")
    base = min(s[0] for s in segs)
    print(f"pid {pid}  base {base:#x}  {len(segs)} vùng")

    data = bytearray(src.read_bytes())
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    opt = pe + 24
    nsec_off = opt + struct.unpack_from("<H", data, pe + 20)[0]
    mem = open(f"/proc/{pid}/mem", "rb", 0)
    patched = 0
    for i in range(nsec):
        o = nsec_off + 40 * i
        name = data[o:o + 8].rstrip(b"\0").decode(errors="replace")
        vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, o + 8)
        if vaddr == 0 or rawsize == 0:
            continue
        chunk = min(rawsize, vsize)
        try:
            mem.seek(base + vaddr)
            buf = mem.read(chunk)
        except Exception:
            continue
        if len(buf) != chunk:
            continue
        if bytes(buf) == bytes(data[rawptr:rawptr + chunk]):
            continue
        data[rawptr:rawptr + chunk] = buf
        patched += 1
        print(f"  {name:<10} va={vaddr:#010x} size={chunk:#x} <- mem (đã unpack)")
    mem.close()
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data)
    print(f"{patched} section đổi -> {dst} ({len(data):,} B)")
    return patched


def dump_regions(pid, outdir: pathlib.Path):
    """Dump mọi vùng thực thi (đặc biệt vùng ẩn danh: code Themida đã unpack)."""
    outdir.mkdir(parents=True, exist_ok=True)
    maps = pathlib.Path(f"/proc/{pid}/maps").read_text(errors="replace")
    mem = open(f"/proc/{pid}/mem", "rb", 0)
    n = 0
    for line in maps.splitlines():
        m = re.match(r"([0-9a-f]+)-([0-9a-f]+) (\S{4}) \S+ \S+ \S+\s*(.*)", line)
        if not m:
            continue
        lo, hi, perm, path = int(m.group(1), 16), int(m.group(2), 16), m.group(3), m.group(4).strip()
        if "x" not in perm or hi - lo > 0x4000000:
            continue
        try:
            mem.seek(lo)
            buf = mem.read(hi - lo)
        except Exception:
            continue
        if not buf:
            continue
        name = f"{lo:08x}_{len(buf):x}.bin"
        (outdir / name).write_bytes(buf)
        n += 1
    mem.close()
    print(f"{n} vùng exec -> {outdir}")
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default=str(KR_ROOT))
    ap.add_argument("--out", default=str(OUT / "kr-unpacked.exe"))
    ap.add_argument("--wait", type=float, default=15)
    ap.add_argument("--delay", type=float, default=3.0, help="chờ bao lâu sau khi thấy pid rồi dump")
    a = ap.parse_args()
    src = pathlib.Path(a.src)
    exe = src / "CA.exe"
    log = src / "CALog.log"
    if log.exists():
        log.unlink()
    out = pathlib.Path(a.out)
    for attempt in range(1, 4):
        if log.exists():
            log.unlink()
        proc = subprocess.Popen(["wine", "CA.exe"], cwd=str(src), env=wine_env(),
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                start_new_session=True)
        pid = find_pid(timeout=a.wait)
        if not pid:
            proc.kill()
            print(f"lần {attempt}: không thấy pid, thử lại")
            continue
        # chờ một khoảng cố định: image đã được Themida unpack trong ~1 s đầu
        hit = False
        end = time.time() + a.delay
        while time.time() < end:
            if not pathlib.Path(f"/proc/{pid}").exists():
                break
            time.sleep(0.02)
        if not hit and not pathlib.Path(f"/proc/{pid}").exists():
            proc.kill()
            print(f"lần {attempt}: client thoát trước khi dump, thử lại")
            continue
        print(f"lần {attempt}: ProcessPatch={'đã tới' if hit else 'chưa (dump sớm)'}")
        try:
            ok = dump_image(pid, exe, out)
            dump_regions(pid, out.parent / "regions")
            if ok:
                proc.kill()
                return
        except SystemExit as ex:
            print(f"  {ex}")
        except FileNotFoundError:
            print("  pid biến mất khi dump")
        finally:
            try:
                os.killpg(os.getpgid(pid), signal.SIGKILL)
            except Exception:
                pass
            proc.kill()
    raise SystemExit("3 lần đều không dump được")


if __name__ == "__main__":
    main()
