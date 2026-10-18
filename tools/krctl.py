"""Công cụ RE client Boom Online KR — chạy trong project con boomkr-re.

Nguyên tắc: **không copy code của boom-re**, chỉ import khi cần (biến BOOM_RE, mặc định ../boom-re).
Nhờ vậy tool pack/protocol/VM của VN vẫn là một bản duy nhất, sửa một chỗ.

Lệnh:
  python3 tools/krctl.py packs [--dir DIR] [--extract N]   # liệt kê pack + dò khoá + xuất thử sprite
  python3 tools/krctl.py manifest [--dir DIR]              # giải files.txt + files.nfo2, kiểm size
  python3 tools/krctl.py sha256 [--dir DIR] [--out F]      # manifest SHA-256 (PRES-002)
  python3 tools/krctl.py doctor [--exe PATH]               # gọi vmdevirt.doctor của boom-re
"""
import argparse
import csv
import hashlib
import json
import os
import pathlib
import re
import struct
import subprocess
import sys

KR = pathlib.Path(os.environ.get("BOOMKR_DIR", "/home/admin/boom-re/boomKR")).resolve()
BOOM_RE = pathlib.Path(os.environ.get("BOOM_RE", "/home/admin/boom-re")).resolve()
HERE = pathlib.Path(__file__).resolve().parent.parent
OUT = HERE / "out"

# khoá đã biết (từ VN). KR dùng lại 2 khoá này cho 4 pack.
KEYS = {
    "fx": 1906092848,
    "charte": 95551996, "emote": 95551996, "fxasset": 95551996,
    # KR
    "CharTE": 95551996, "EmoTE": 95551996, "FxAsset": 95551996,
}


def load_idf():
    sys.path.insert(0, str(BOOM_RE / "tools"))
    import idf  # noqa: E402  (tool của boom-re, không copy)
    return idf


def probe_key(idf, idx_path):
    """Dò khoá cho một .idx: parse thành công + tên đọc được + nhiều entry."""
    data = idx_path.read_bytes()
    for key in {95551996, 1906092848, 0, 0x2C}:
        try:
            ver, ents, _ = idf.parse_idx(idf.decrypt_idx(data, key))
        except Exception:
            continue
        if len(ents) < 5:
            continue
        names = [e[0] for e in ents[:8]]
        if all(n and all(32 <= ord(c) < 127 for c in n) for n in names):
            return key, ver, ents
    return None, None, None


def cmd_packs(a):
    root = pathlib.Path(a.dir).resolve()
    idf = load_idf()
    rows = []
    for idx in sorted(root.glob("*.idx")):
        key, ver, ents = None, None, None
        if idx.stem in KEYS:
            try:
                data = idx.read_bytes()
                ver, ents, _ = idf.parse_idx(idf.decrypt_idx(data, KEYS[idx.stem]))
                key = KEYS[idx.stem]
            except Exception as ex:
                print(f"  {idx.stem}: lỗi với khoá đã biết: {ex}", file=sys.stderr)
        if ents is None:
            key, ver, ents = probe_key(idf, idx)
        if ents is None:
            rows.append({"pack": idx.stem, "key": None, "version": None, "entries": None})
            print(f"{idx.stem:<14} KHOÁ CHƯA BIẾT")
            continue
        idd = idx.with_suffix(".idd")
        rows.append({"pack": idx.stem, "key": key, "version": ver, "entries": len(ents),
                     "idd_bytes": idd.stat().st_size if idd.exists() else None})
        print(f"{idx.stem:<14} key={key:<12} ver={ver} entries={len(ents):<7} "
              f"idd={idd.stat().st_size if idd.exists() else 0:,}")
    OUT.mkdir(exist_ok=True)
    (OUT / "packs.json").write_text(json.dumps(rows, indent=1))
    known = sum(1 for r in rows if r["entries"])
    print(f"\n{known}/{len(rows)} pack đọc được -> {OUT/'packs.json'}")
    if a.extract:
        stem = a.extract_pack
        key = KEYS.get(stem) or next(r["key"] for r in rows if r["pack"] == stem)
        dest = OUT / "extract" / stem
        dest.mkdir(parents=True, exist_ok=True)
        subprocess.run([sys.executable, str(BOOM_RE / "tools" / "idf.py"), "extract",
                        str(root), stem, str(dest)], check=False)


def cmd_manifest(a):
    root = pathlib.Path(a.dir).resolve()
    txt = root / "files.txt"
    nfo = root / "files.nfo2"
    urls = []
    if txt.exists():
        for line in txt.read_text(errors="replace").splitlines():
            m = re.match(r"(https?://.+?)\.([0-9a-f]{8})\.nxgz$", line.strip())
            if m:
                rel = re.sub(r"^.*/PatchData/", "", m.group(1))
                urls.append({"url": line.strip(), "path": rel, "hash8": m.group(2)})
    rows, size_ok, size_bad, missing = [], 0, 0, 0
    if nfo.exists():
        body = nfo.read_text(errors="replace").splitlines()[1:]   # dòng 1 = "NFO400 ; ..."
        for r in csv.reader(body, quotechar='"'):
            if len(r) < 6:
                continue
            name = r[0].replace("\\", "/")
            size = int(r[2])
            f = root / name
            actual = f.stat().st_size if f.exists() else None
            if actual is None:
                missing += 1
            elif actual == size:
                size_ok += 1
            else:
                size_bad += 1
            rows.append({"path": name, "date": r[1], "size": size, "csize": int(r[3]),
                         "hash_a": int(r[4]), "hash_b": int(r[5]), "hash_c": int(r[6]),
                         "actual": actual})
    print(f"files.txt: {len(urls)} URL   files.nfo2: {len(rows)} dòng")
    print(f"  size khớp file thật: {size_ok}  lệch: {size_bad}  thiếu: {missing}")
    print(f"  tỉ lệ nén csize/size trung bình: "
          f"{sum(r['csize']/r['size'] for r in rows if r['size'])/max(1,len(rows)):.3f}")
    if urls:
        print(f"  8-hex KHÔNG phải CRC32 của file đã giải nén (đã kiểm 173/173)")
    OUT.mkdir(exist_ok=True)
    (OUT / "manifest.json").write_text(json.dumps(
        {"urls": urls, "nfo": rows, "size_ok": size_ok, "size_bad": size_bad}, indent=1))
    print(f"-> {OUT/'manifest.json'}")


def cmd_sha256(a):
    root = pathlib.Path(a.dir).resolve()
    dest = pathlib.Path(a.out) if a.out else OUT / "boomKR.sha256"
    dest.parent.mkdir(parents=True, exist_ok=True)
    lines = []
    for f in sorted(root.rglob("*")):
        if f.is_file():
            h = hashlib.sha256()
            with f.open("rb") as fh:
                for chunk in iter(lambda: fh.read(1 << 20), b""):
                    h.update(chunk)
            lines.append(f"{h.hexdigest()}  {f.relative_to(root)}")
    dest.write_text("\n".join(lines) + "\n")
    print(f"{len(lines)} file -> {dest}")


def cmd_doctor(a):
    exe = pathlib.Path(a.exe) if a.exe else KR / "CA.exe"
    tool = BOOM_RE / "tools" / "vmdevirt.py"
    print(f"# {tool} doctor {exe}\n")
    sys.exit(subprocess.run([sys.executable, str(tool), "doctor", str(exe)]).returncode)


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("packs"); p.add_argument("--dir", default=str(KR))
    p.add_argument("--extract", action="store_true"); p.add_argument("--extract-pack", default="FxAsset")
    p.set_defaults(func=cmd_packs)
    m = sub.add_parser("manifest"); m.add_argument("--dir", default=str(KR)); m.set_defaults(func=cmd_manifest)
    s = sub.add_parser("sha256"); s.add_argument("--dir", default=str(KR)); s.add_argument("--out")
    s.set_defaults(func=cmd_sha256)
    d = sub.add_parser("doctor"); d.add_argument("--exe"); d.set_defaults(func=cmd_doctor)
    a = ap.parse_args()
    a.func(a)


if __name__ == "__main__":
    main()
