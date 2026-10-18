"""Dựng image phẳng (offset = VA - 0x400000) từ file unpack + các vùng runtime, rồi soi stub head.

Vì sao cần: Themida cấp vùng private 0xB66000 (18,9 MB) nằm trong *khoảng trống địa chỉ* của image —
file PE không chứa nó, nên capstone/IDA không thấy code unpack. Image phẳng cho phép đọc byte theo VA
mà không cần PE hợp lệ.

Dùng:
  python3 tools/krflat.py build          # -> out/kr-flat.bin
  python3 tools/krflat.py heads          # disasm 26 stub head (cần out/kr-flat.bin)
"""
import argparse
import pathlib
import struct
import sys

HERE = pathlib.Path(__file__).resolve().parent.parent
OUT = HERE / "out"
BASE = 0x400000
SPAN = 0x2300000          # phủ tới hết image + vùng private


def pe_sections(path):
    d = pathlib.Path(path).read_bytes()
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
    nso = pe + 24 + struct.unpack_from("<H", d, pe + 20)[0]
    secs = []
    for i in range(nsec):
        o = nso + 40 * i
        name = d[o:o + 8].rstrip(b"\0").decode(errors="replace")
        vs, va, rs, rp = struct.unpack_from("<IIII", d, o + 8)
        secs.append((name, va, vs, rs, rp))
    return d, base, secs


def cmd_build(a):
    src = OUT / "kr-unpacked-text.exe"
    d, base, secs = pe_sections(src)
    assert base == BASE, f"base lạ: {base:#x}"
    flat = bytearray(SPAN)
    filled = bytearray(SPAN)
    for name, va, vs, rs, rp in secs:
        n = min(rs, vs)
        if n:
            flat[va:va + n] = d[rp:rp + n]
            filled[va:va + n] = b"\x01" * n
    # chồng các vùng runtime lên (trạng thái sau unpack thắng)
    for f in sorted((OUT / "regions_3.0").glob("*.bin")):
        lo = int(f.name.split("_")[0], 16) - base
        b = f.read_bytes()
        if lo < 0 or lo + len(b) > SPAN:
            continue
        flat[lo:lo + len(b)] = b
        filled[lo:lo + len(b)] = b"\x01" * len(b)
    (OUT / "kr-flat.bin").write_bytes(bytes(flat))
    print(f"kr-flat.bin {len(flat):,} B  ({filled.count(1):,} B có dữ liệu, "
          f"{SPAN - filled.count(1):,} B trống)")


def cmd_heads(a):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    flat = (OUT / "kr-flat.bin").read_bytes()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    # lấy danh sách head bằng tool của boom-re
    import subprocess, json as js
    out = subprocess.run([sys.executable, "/home/admin/boom-re/tools/vmdevirt.py", "discover",
                          str(OUT / "kr-unpacked-text.exe")], capture_output=True, text=True)
    heads = []
    for line in out.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[0] == "head":
            heads.append(int(parts[1], 16))
    print(f"{len(heads)} stub head\n")
    prologues = {}
    for h in heads:
        off = h - BASE
        code = flat[off:off + 40]
        ins = list(md.disasm(code, h))[:6]
        txt = " | ".join(f"{i.mnemonic} {i.op_str}" for i in ins)
        prologues.setdefault(ins[0].mnemonic if ins else "?", 0)
        prologues[ins[0].mnemonic if ins else "?"] += 1
        print(f"{h:#010x}  {txt}")
    print("\nlệnh đầu tiên:", prologues)


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("build").set_defaults(func=cmd_build)
    sub.add_parser("heads").set_defaults(func=cmd_heads)
    a = ap.parse_args()
    a.func(a)


if __name__ == "__main__":
    main()
