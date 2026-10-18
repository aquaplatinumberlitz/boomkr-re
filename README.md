# boomkr-re — RE client Boom Online **KR** (project con)

Project con của [`boom-re`](../boom-re) (bản VN). Tách riêng để **không trùng lặp/giẫm chân** với
project VN đang được một session khác làm song song.

- Build KR: `/home/admin/boom-re/boomKR/` (1,8 GB, git-ignored trong repo VN)
- Nguồn gốc build: patch Nexon `00967`, `release_date.txt` = `2026-06-10 13:30:30`
- Master plan & phân vai: [`docs/MASTER_PLAN.md`](../boom-re/docs/MASTER_PLAN.md)
- Kế hoạch KR: [`docs/kr-re-plan.md`](../boom-re/docs/kr-re-plan.md)

## Trạng thái

| Việc | Trạng thái | Bằng chứng |
|---|---|---|
| KR-ASSET-001 — pack nào đọc được | ✅ **4/13** | `out/packs.json`, `docs/kr.md` §2 |
| KR-001 — giải manifest patch | ✅ schema + kiểm size 173/173 | `out/manifest.json`, `docs/kr.md` §3 |
| PRES-002 (phần KR) — SHA-256 build | ✅ 177 file | `out/boomKR.sha256` |
| KR-002 — quyết định phạm vi/pháp lý | ⏸ **chờ chủ dự án** | — |
| KR-ASSET-002 — khoá 9 pack `Fx*` | ⛔ cần KR-002 (phải chạy client) | — |
| KR-003 — so sánh PE tĩnh | ⛔ cần KR-002 | — |
| Chạy client / unpack / protocol | ⛔ **không làm** khi chưa có KR-002 | `MASTER_PLAN` §6.3 |

> Theo `MASTER_PLAN` §6.3: mọi việc **runtime** của KR bị chặn bởi **KR-002** (quyết định phạm vi +
> pháp lý). Project này chỉ làm phần **tĩnh/asset**, đúng những việc không cần gate.

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
