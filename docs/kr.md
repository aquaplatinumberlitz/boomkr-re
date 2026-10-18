# Build KR — phát hiện tĩnh

Tác giả: dsh · 2026-10-18 · build `00967` (2026-06-10) tại `/home/admin/boom-re/boomKR/`.
Mọi mục có tag bằng chứng theo `MASTER_PLAN` §7.

## 1. `CA.exe` — cấu trúc **[VERIFIED]**

| Thông số | Giá trị | Lệnh |
|---|---|---|
| Kích thước | 10.141.152 B | `ls -l` |
| PE | PE32, machine `0x14c`, imagebase `0x400000` | `pefile` |
| Entry | RVA `0x1E5D000`, section `gsowkarq` (raw `0x200`) | `pefile` |
| Section VM | `gulxdsye` 1,99 MB, entropy **7,95** | `tools/vmdevirt.py doctor` |
| `.idata` | raw `0x200` | `doctor` |
| Import thật | `kernel32` (1 hàm) + `comctl32` (1 hàm) | `pefile` |
| Marker `EB 10 57 4C 20` | **0** (VN: 73) | `doctor` |
| Manifest | `GameGuardVista Program`, `requireAdministrator` | `strings` |
| Chữ ký | NEXON Korea (DigiCert EV) | `strings` (chain DigiCert) |
| `CAKR.ini` | **không phải text** (nhị phân) | `cat` → rác |

Kết luận: file **packed hoàn toàn** (2 import, IAT xoá). `doctor` → `PACKED`.

## 2. Pack `.idx/.idd` — **4/13 đọc được** **[VERIFIED]**

Lệnh: `python3 tools/krctl.py packs` (dùng `tools/idf.py` của boom-re).

| pack | khoá | ver | entries | idd |
|---|---|---|---|---|
| `CharTE` | `95551996` | 15 | **20.744** | 368.661.975 |
| `EmoTE` | `95551996` | 15 | **10.040** | 275.290.407 |
| `FxAsset` | `95551996` | 15 | 11 | 155.592 |
| `fx` | `1906092848` | 15 | 4.093 | 87.822.170 |
| `FxCharacter`, `FxContents`, `FxDesktop`, `FxHiddenCatch`, `FxMap`, `FxMonster`, `FxSound`, `FxStage`, `FxTile` | **chưa biết** | — | — | — |

- Header `.idd` KR giống VN (4 dword đầu), nên reader VN dùng được.
- Khoá KR **trùng khoá VN**: `95551996` (pack nội dung) và `1906092848` (`fx`).
- Tên trong `idx` là **cp949** (Hàn), `parse_idx` đã xử lý; ví dụ `AvatarDeco/A02_10_0001_0LayerAdjust`,
  `CAMission/Bt_CaMission_Quest`.
- 9 pack `Fx*` là **pack mới của KR** (VN chỉ có `fxasset`). Cần khoá riêng — **chỉ lấy được khi chạy
  client** (hook `PackFS__DecryptIdx`), tức bị chặn bởi **KR-002**.
- Đã thử 3 khoá VN + `0` + `0x2C` trên cả 9 pack: không khớp. Không brute-force (2^32).

## 3. Manifest patch — schema giải được **[VERIFIED]**

### `files.txt` — 173 URL

```
http://crazy.dn.nexoncdn.co.kr/NGMpatch/00967/PatchData/<path>.<8hex>.nxgz
```

- `<8hex>` **không phải** CRC32 của file đã giải nén: kiểm 173/173 đều khác **[VERIFIED]**.
- Cũng không phải adler32 / md5[:4] / sha1[:4] / crc32 của gzip (thử trên `binkw32.dll`).
- Giả thuyết còn lại **[HYPOTHESIS]**: là CRC32/hash của **payload `.nxgz` đã nén** (thứ ta không có,
  vì patch đã áp). Muốn xác minh phải tải một `.nxgz` từ CDN — **không làm** (ngoài phạm vi offline).

### `files.nfo2` — 173 dòng, schema **[VERIFIED]**

```
NFO400 ; DO NOT edit this line manually
"<path>","<date>","<size>","<csize>","<hash_a>","<hash_b>","<hash_c>","0","0"
```

| Trường | Nghĩa | Kiểm chứng |
|---|---|---|
| `path` | đường dẫn tương đối (`\` → `/`) | 173/173 file tồn tại |
| `date` | `YYYYMMDDHH` (vd `2006120520`) | **[HYPOTHESIS]** theo giá trị quan sát |
| `size` | kích thước **đã giải nén** | ✅ **173/173 khớp `stat`** |
| `csize` | kích thước nén | tỉ lệ `csize/size` trung bình **0,501** |
| `hash_a`,`hash_b`,`hash_c` | 3 hash (int32 có dấu) | **[UNKNOWN]** thuật toán |
| 2 số `0` cuối | chưa rõ | **[UNKNOWN]** |

Lệnh: `python3 tools/krctl.py manifest` → `out/manifest.json`.

## 4. Thành phần đi kèm

| File/thư mục | Nhận xét |
|---|---|
| `BlackCipher/*.aes` (`BlackCall`, `BlackCipher`, `BlackMon`, `BlackXchg`) | payload INCA mã hoá, **không đụng** |
| `Dwarf/` | **CEF (Chromium)**: `chrome_elf.dll`, `*.pak` → **browser nhúng cho login web**, không phải anti-cheat |
| `CALauncher.exe` (3,5 MB) | launcher Nexon (auth + patch) |
| `CAWH.sys`, `nmcosrv.exe`, `ngclient.aes`, `gameguard.des` | anti-cheat/driver — **không đụng** |
| `NexonAnalytics32/64.dll`, `NGMDll.dll` | telemetry/launcher, 64-bit có mặt |
| 14 file **byte-identical** với VN | `binkw32.dll`, `cxdefault.fft`, `fxasset.idd/.idx`, `d3dx9_30.dll`, … (xem `MASTER_PLAN` §4) |

## 5. Câu hỏi mở **[UNKNOWN]**

1. Nguồn gốc `CA.exe.i64` (40,7 MB, 2026-10-17) — ai tạo, bằng IDA nào, có đáng tin?
2. Thuật toán 3 hash trong `files.nfo2`.
3. Khoá 9 pack `Fx*` (cần chạy client).
4. Framing protocol KR có giống VN không.
5. KR có chạy offline được không (GameGuard + launcher).
6. Có tài khoản KR hợp lệ không.

## 6. Việc bị chặn

Mọi việc **runtime** (chạy client, unpack, protocol, khoá `Fx*`) đều chờ **KR-002** — quyết định phạm vi
và pháp lý của chủ dự án (`MASTER_PLAN` §6.3, D-01/D-03). Project này **không** chạy `CA.exe`.

## 7. KR-002 — QUYẾT ĐỊNH (2026-10-18, chủ dự án)

| Trục | Quyết định |
|---|---|
| Mục tiêu | **(A) offline-preservation** — giữ/khôi phục game chạy được **không cần server Nexon**; client + server giả chạy LAN/offline (hoặc remaster) |
| Chạy client KR | **Có** — cần chạy để lấy dữ liệu (unpack, khoá pack, protocol) trong môi trường cô lập |
| Anti-cheat | **Gỡ** — loại bỏ GameGuard/BlackCipher khỏi bản chạy cục bộ để client khởi động offline |
| Phân phối | **Không** phân phối binary/asset gốc; không đưa thành phần anti-cheat vào bất kỳ bản build nào |

### Hệ quả kỹ thuật (ràng buộc tự đặt)

1. Mọi thử nghiệm chạy client **chỉ trong WinBoat**, thư mục riêng `C:\boomkr` — không đụng `C:\boom` (VN),
   không sửa `boomKR/` gốc trên host.
2. **Cắt mạng ra ngoài** khi chạy (không gửi dữ liệu tới Nexon). Chỉ giữ host-share để lấy file.
3. Gỡ anti-cheat bằng cách **loại bỏ/đổi tên thành phần** trong bản copy (không viết patch phát hành):
   `BlackCipher/`, `CAWH.sys`, `nmcosrv.exe`, `gameguard.des`, `ngclient.aes`.
4. Nếu client thoát vì thiếu anti-cheat: chuyển sang **patch điểm gọi khởi tạo AC** trong `CA.exe`
   (chỉ trên bản copy cục bộ) — ghi rõ offset vào tài liệu.
5. Không login tài khoản thật, không capture traffic server thật.
6. Mọi artefact nặng nằm ở `out/` (git-ignored) hoặc ngoài repo.
