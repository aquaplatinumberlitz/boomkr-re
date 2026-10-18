# boomkr-re — RE client Boom Online **KR** (project con)

Project con của [`boom-re`](../boom-re) (bản VN). Tách riêng để **không trùng lặp/giẫm chân** với
project VN đang được một session khác làm song song.

- Build KR: `/home/admin/boom-re/boomKR/` (1,8 GB, git-ignored trong repo VN)
- Nguồn gốc build: patch Nexon `00967`, `release_date.txt` = `2026-06-10 13:30:30`
- Master plan & phân vai: [`docs/MASTER_PLAN.md`](../boom-re/docs/MASTER_PLAN.md)
- Kế hoạch KR: [`docs/kr-re-plan.md`](../boom-re/docs/kr-re-plan.md)

## Trạng thái (cập nhật 2026-10-18, cuối ngày)

| Việc | Trạng thái | Bằng chứng |
|---|---|---|
| KR-002 — quyết định phạm vi/pháp lý | ✅ **đã chốt**: offline-preservation, chạy trong VM cô lập, gỡ anti-cheat, **không phân phối** | `docs/kr.md` §7 |
| KR-ASSET-001 — pack đọc được | ✅ **13/13** (51.467 entry, 1.367 MB) | `docs/kr.md` §16, `out/packs.json` |
| KR-ASSET-002 — khoá các pack `Fx*` | ✅ **giải xong**: khoá = **4 byte đầu của `.idx`** (không cần chạy client) | `docs/kr.md` §16 |
| Trích xuất asset | ✅ kiểm chứng: Ogg/map/sprite lấy ra đúng định dạng | `docs/kr.md` §16 |
| KR-001 — manifest patch | ✅ schema + `size` khớp **173/173**; ❌ 3 hash không xác minh được (tính trên payload `.nxgz` đã nén) | `docs/kr.md` §3, §17 |
| PRES-002 (KR) — SHA-256 build | ✅ 177 file | `out/boomKR.sha256` |
| KR-003 — phân tích PE tĩnh | ✅ xong (section/entry/import/manifest/chữ ký/CEF) | `docs/kr.md` §1, §4 |
| **Chạy client KR** | 🟡 **chạy được trong VM** qua hook+inject, sống >20 s (RSS 68–85 MB), **tự unpack**; ❌ **dừng ở `ProcessPatch`** rồi thoát | `docs/kr.md` §8, §15 |
| **Image unpack** | 🟡 **85%**: module image 31,8 MB **đã giải mã** (`.text` entropy 5,55) + **359 vùng** commit (138 MB); ❌ `.themida` vẫn 7,95 | `docs/kr.md` §10, §18 |
| Devirt VM KR | 🟡 **10%**: xác định **khác đời VN** — 0 marker, 102 site/26 head, **không bảng handler tĩnh** (1.562 điểm `jmp [reg*4+disp]` đều là jump table C++) ⇒ nghi kiểu **chained/FALCON** | `docs/kr.md` §11, §12 |
| Protocol KR | ⬜ **0%** — client chưa từng mở kết nối | — |
| Server KR | ⬜ **0%** | — |

Tổng: **asset + hiểu build + manifest ≈ 95%**; **chơi offline được ≈ 25%**.

### "Trạng thái client" nghĩa là gì

1. **Chạy được**: `CA.exe` khởi động trong WinBoat (Windows 10), không crash ngay.
2. **Tự unpack**: Themida giải mã code vào bộ nhớ; ta dump được module image 31,8 MB đã giải mã
   (`.text` entropy 5,55, có chuỗi `ProcessPatch`, `EndOfInit! exitcode=`).
3. **Dừng ở bước kiểm tra khởi động** (`ProcessPatch`): hiện dialog **tiếng Hàn** rồi thoát
   `exitcode=1`. Nội dung dialog **không đọc được** vì app đổi wide→ANSI bằng ACP=1252 trước khi hiện
   (`MessageBoxA` nhận toàn byte `?`).
4. **Vì vậy VM chưa chạy**: section `.themida` (1,9 MB bytecode) **vẫn mã hoá 7,95** — đây là lý do
   protocol/VM/server đều 0%: không phải khó, mà **chưa tới được**.
5. **Đã có đồ nghề để phá**: hook DLL (`krhook.dll`) log được địa chỉ gọi `MessageBoxA` và **chặn
   `ExitProcess(≠0)`**; launcher inject `krlaunch.exe`; dumper 32-bit `krdump32.exe` (359 vùng); image
   unpacked để disassemble.

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
