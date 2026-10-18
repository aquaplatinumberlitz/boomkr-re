"""Tìm bảng handler của VM KR từ các điểm dispatch `jmp [reg*4+disp32]`.

Ý tưởng: trong image đã unpack, mỗi điểm dispatch là `FF 24 <modrm> <disp32>` với
modrm ∈ {85 = [eax*4+d], 8D = [ecx*4+d], 95 = [edx*4+d], 9D = [ebx*4+d], A5 = [ebp*4+d]…}.
`disp32` (địa chỉ tuyệt đối trong image phẳng, base 0x400000) thường là **bảng con trỏ**.
Đọc bảng đó, đếm entry liên tiếp là con trỏ code → đó là bảng handler.

Dùng:
  python3 tools/krdispatch.py [--exe out/kr-unpacked-text.exe] [--json out/kr_tables.json]
"""
import argparse
import json
import pathlib
import struct

HERE = pathlib.Path(__file__).resolve().parent.parent
OUT = HERE / "out"

# modrm của `jmp [reg*4+disp32]` (mod=00, rm=100 SIB, scale=2)
SIB_MODRM = {0x85: "eax", 0x8D: "ecx", 0x95: "edx", 0x9D: "ebx", 0xA5: "ebp", 0xAD: "esi", 0xB5: "edi"}


class Img:
    def __init__(self, path):
        self.raw = pathlib.Path(path).read_bytes()
        pe = struct.unpack_from("<I", self.raw, 0x3C)[0]
        nsec = struct.unpack_from("<H", self.raw, pe + 6)[0]
        opt = pe + 24
        self.base = struct.unpack_from("<I", self.raw, opt + 28)[0]
        nso = opt + struct.unpack_from("<H", self.raw, pe + 20)[0]
        self.secs = []
        for i in range(nsec):
            o = nso + 40 * i
            name = self.raw[o:o + 8].rstrip(b"\0").decode(errors="replace")
            vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", self.raw, o + 8)
            self.secs.append({"name": name, "va": vaddr, "vsize": vsize,
                              "raw": rawptr, "rawsize": rawsize})

    def sec_of(self, va):
        rva = va - self.base
        for s in self.secs:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rawsize"]):
                return s
        return None

    def read(self, va, n):
        s = self.sec_of(va)
        if not s:
            return None
        off = va - self.base - s["va"]
        if off < 0 or off + n > s["rawsize"]:
            return None
        return self.raw[s["raw"] + off: s["raw"] + off + n]

    def u32(self, va):
        b = self.read(va, 4)
        return struct.unpack("<I", b)[0] if b else None


def find_dispatch(img):
    """Quét toàn bộ section thực thi tìm `jmp [reg*4+disp32]`."""
    out = []
    for s in img.secs:
        if s["name"] not in (".text", ".themida"):
            continue
        d = img.raw[s["raw"]:s["raw"] + s["rawsize"]]
        for i in range(len(d) - 7):
            if d[i] != 0xFF or d[i + 1] != 0x24:
                continue
            reg = SIB_MODRM.get(d[i + 2])
            if not reg:
                continue
            disp = struct.unpack_from("<I", d, i + 3)[0]
            out.append({"at": img.base + s["va"] + i, "reg": reg, "table": disp,
                        "section": s["name"]})
    return out


def read_table(img, va, max_entries=1024):
    """Đọc các con trỏ liên tiếp cho tới khi giá trị không trỏ vào section code."""
    ents = []
    for k in range(max_entries):
        v = img.u32(va + 4 * k)
        if v is None:
            break
        s = img.sec_of(v)
        if not s or s["name"] not in (".text", ".themida"):
            break
        ents.append(v)
    return ents


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=str(OUT / "kr-unpacked-text.exe"))
    ap.add_argument("--json", default=str(OUT / "kr_tables.json"))
    a = ap.parse_args()
    img = Img(a.exe)
    print(f"image {a.exe}  base {img.base:#x}  {len(img.secs)} section")
    for s in img.secs:
        print(f"  {s['name']:<12} va={img.base + s['va']:#010x} vsize={s['vsize']:#x} raw={s['rawsize']:#x}")

    disps = find_dispatch(img)
    print(f"\ndispatch `jmp [reg*4+disp]`: {len(disps)}")
    from collections import Counter
    print("  theo thanh ghi:", dict(Counter(d["reg"] for d in disps)))
    print("  theo section:", dict(Counter(d["section"] for d in disps)))

    tables = {}
    for d in disps:
        t = d["table"]
        if t not in tables:
            s = img.sec_of(t)
            tables[t] = {"table": t, "section": s["name"] if s else None,
                         "entries": read_table(img, t), "used_by": []}
        tables[t]["used_by"].append(d["at"])

    print(f"\nbảng ứng viên: {len(tables)}")
    rows = []
    for t, info in sorted(tables.items(), key=lambda kv: -len(kv[1]["entries"])):
        n = len(info["entries"])
        if n == 0:
            continue
        tgt = Counter(img.sec_of(v)["name"] for v in info["entries"])
        rows.append({"table": hex(t), "section": info["section"], "entries": n,
                     "targets": dict(tgt), "used_by": len(info["used_by"])})
        print(f"  bảng {t:#010x} ({info['section']}): {n:>4} entry, "
              f"đích={dict(tgt)}, dùng bởi {len(info['used_by'])} điểm dispatch")
    pathlib.Path(a.json).write_text(json.dumps({"dispatch": len(disps), "tables": rows}, indent=1))
    print(f"-> {a.json}")


if __name__ == "__main__":
    main()
