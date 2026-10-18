# boomkr-re — RE client Boom Online **KR** (project con)

Project con của [`boom-re`](../boom-re) (bản VN). Tách riêng để **không trùng lặp/giẫm chân** với
project VN đang được một session khác làm song song.

- Build KR: `/home/admin/boom-re/boomKR/` (1,8 GB, git-ignored trong repo VN)
- Nguồn gốc build: patch Nexon `00967`, `release_date.txt` = `2026-06-10 13:30:30`
- Master plan & phân vai: [`docs/MASTER_PLAN.md`](../boom-re/docs/MASTER_PLAN.md)
- Kế hoạch KR: [`docs/kr-re-plan.md`](../boom-re/docs/kr-re-plan.md)

## Trạng thái (chốt cuối đêm 2026-10-18/19)

| Việc | Trạng thái | Bằng chứng |
|---|---|---|
| KR-002 — quyết định phạm vi | ✅ đã chốt (offline-preservation, VM cô lập, gỡ AC, không phân phối) | `docs/kr.md` §7 |
| Asset pack | ✅ **13/13** — 51.467 entry, 1.367 MB; khoá = 4 byte đầu `.idx` | §16 |
| Trích xuất asset | ✅ Ogg/map/sprite đúng định dạng | §16 |
| Manifest patch | ✅ 95% (schema + size 173/173); ❌ 3 hash cần `.nxgz` | §3, §17 |
| PE tĩnh (section/entry/import/chữ ký/CEF) | ✅ xong | §1, §4 |
| Client chạy trong VM | ✅ chạy, tự unpack | §10, §15 |
| Image unpack (module) | ✅ `.text` 6,45 + 700 vùng/138 MB | §18, §22 |
| `.themida` (VM bytecode) | ❌ luôn 7,94 — VM chưa chạy | §23, §26 |
| Vượt `ProcessPatch` | ❌ chưa; anti-tamper chặn mọi instrumentation | §24, §27, §29 |
| Protocol / server | ⬜ 0% | — |

**Kết luận:** phần **asset + hiểu build + manifest ≈ 95%**; phần **chạy được game ≈ 25%**.

### "Trạng thái client" nghĩa là gì

1. `CA.exe` khởi động trong Windows VM (WinBoat), **tự unpack** — dump được module 31,8 MB đã giải mã.
2. Dừng ở bước kiểm tra khởi động `ProcessPatch` (CALog: `InitBeforePatch → ProcessPatch...`), rồi
   **lặp lại vòng khởi động**; `exitcode=1` ở lần đầu.
3. Vì chưa qua bước này nên VM bytecode (`.themida`) **chưa được giải mã** — kéo theo protocol/VM/server
   đều 0%: *không phải khó, mà chưa tới được*.
4. **Mọi cách quan sát runtime đều kích hoạt anti-tamper**: inject DLL → "Cannot find binkw32.dll";
   vá API hệ thống (`IAT`, forwarder, `ntdll!NtWriteFile`) → dialog tiếng Hàn. Chạy plain thì im lặng.
5. Đường còn lại: patch **file** (đúng mục tiêu) — cần một lần quan sát **không xâm lấn**
   (đổi locale VM sang Korean để đọc dialog, hoặc Procmon/ETW).

### Công cụ trong repo

| File | Việc |
|---|---|
| `tools/krctl.py` | manifest/pack: `packs`, `manifest`, `doctor` |
| `tools/krdump.py` | dump image từ process Wine (`/proc/pid/mem`) |
| `tools/krdump32.c/.exe` | dump module + mọi vùng commit trong Windows (32-bit, đọc theo khối) |
| `tools/krflat.py` | image phẳng VA→offset |
| `tools/krdispatch.py` | tìm bảng handler từ các điểm `jmp [reg*4+disp]` |
| `tools/krhook.c/.dll`, `tools/krlaunch.c/.exe` | inject DLL (⚠ kích hoạt anti-tamper) |
| `tools/krhookmem.c` | hook IAT từ ngoài (không inject) |
| `tools/krhookpro.c` | prologue/forwarder hook + trampoline N byte |
| `win/kr_dump.bat`, `win/kr_hookmem.bat` | wrapper chạy trong VM (gõ ngắn) |

## Nguyên tắc: không copy code

Tool của `boom-re` là **một bản duy nhất**. Project này import khi cần:

```bash
export BOOM_RE=/home/admin/boom-re      # mặc định
export BOOMKR_DIR=/home/admin/boom-re/boomKR
```

`tools/krctl.py` gọi `tools/idf.py` (pack) và `tools/vmdevirt.py` (doctor) của `boom-re` qua `sys.path`
và `subprocess` — không có bản sao nào trong repo này. Xem `SYNC.md`.

## Dùng

```bash
python3 tools/krctl.py packs            # 13 pack: khoá nào biết, bao nhiêu entry
python3 tools/krctl.py manifest         # files.txt + files.nfo2, kiểm size với file thật
python3 tools/krctl.py sha256           # manifest SHA-256 của build KR
python3 tools/krctl.py doctor           # doctor trên CA.exe (qua boom-re)
```

Kết quả ghi vào `out/` (git-ignored).

## Luật bằng chứng (theo MASTER_PLAN §7)

Mọi khẳng định trong `docs/` phải có: (1) tag `[VERIFIED|DOC|HYPOTHESIS|UNKNOWN]`;
(2) đường dẫn tương đối + địa chỉ/dòng hoặc lệnh; (3) tác giả, ngày.
Địa chỉ phải nêu rõ binary (`CA.exe`), và phân biệt thập phân/hex.
