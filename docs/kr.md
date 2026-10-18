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

## 8. Gate G1 — client có chạy được? **KẾT QUẢ: KHÔNG (một phần)**

Thử trong WinBoat (Windows 10 19045, admin), thư mục riêng, **không** đụng `C:\boom` của VN.

### Chuỗi thử

| # | Cấu hình | Kết quả |
|---|---|---|
| 1 | bản sạch anti-cheat (`boomkr-run`: bỏ `BlackCipher/`, `Dwarf/`, `CAWH.sys`, `GameGuard.des`, `NGM*`, `nmco*`) | Themida: **"Cannot find 'nmco3.dll'"** → client không vào được |
| 2 | + `nmco3.dll`, `nmcogame.dll`, `NGMDll.dll`, `NGMResource.dll`, `NGClient.aes` | Themida: **"Cannot find 'CAWH.sys'"** |
| 3 | + `CAWH.sys`, `GameGuard.des` | dialog **tiếng Hàn, hiện `???`** rồi thoát |
| 4 | bản **đầy đủ** (`boomkr-full`, có `BlackCipher/`) | `CALog.log`: `InitBeforePatch` → `ProcessPatch...` → **`EndOfInit! exitcode=1`** → thoát |
| 5 | `CALauncher.exe` | thoát ngay, không log, không cửa sổ |

### Bằng chứng

`boomkr-run/CALog.log` (client tự ghi):
```
967 : InitBeforePatch
967 : ProcessPatch...
967 : EndOfInit! exitcode=1
```
- `tasklist` sau 15 s: **không còn** `CA.exe`/`NGM*`/`GameGuard*` → client thoát.
- Client **đã tự unpack xong** trước khi tới `ProcessPatch` (log do code game ghi, không phải stub).
- Dialog tiếng Hàn **không đọc được**: app dùng `MessageBoxA`, text cp949 đã bị thay bằng `?` trước khi
  hiển thị. `GetWindowTextW` cũng chỉ trả `???` (`tools/grab_dialog.ps1`) ⇒ không lấy được nội dung.
- Đổi codepage hệ thống sang 949 (Korean) sẽ đọc được, nhưng **ảnh hưởng client VN (cp1258)** đang dùng
  chung VM ⇒ chưa làm.

### Kết luận G1

- Client **không chạy được** ở dạng nguyên bản: dừng ở `ProcessPatch` với `exitcode=1`.
- **Không phải do anti-cheat bị gỡ** (bản đầy đủ cũng dừng cùng chỗ, cùng exit code).
- Nghi vấn chính **[HYPOTHESIS]**: `ProcessPatch` cần **patch server / NGM** (mạng) hoặc token do
  launcher ghi vào `CAKR.ini`; launcher cũng thoát vì không có server Nexon.
- Hệ quả: muốn qua G1 phải **patch bỏ `ProcessPatch`** hoặc **giả lập NGM/patch server**.

### Việc tiếp theo (G1)

1. Viết **dumper riêng cho KR** (tool `live_dump.ps1` của VN lỗi `OverflowException` với process này);
   dump ngay ~0,9 s sau khi start để bắt image đã unpack ⇒ có thể qua **G2 mà không cần qua G1**.
2. Nếu dump được: `doctor` + `discover` trên image unpack ⇒ trả lời G3 (VM đời nào).
3. Nếu không dump được: đọc `CAKR.ini` (mã hoá) / hook `ProcessPatch` — cần quyết định thêm.

## 9. Môi trường Wine cô lập (không đụng bản VN) — **[VERIFIED]**

| Thành phần | Đường dẫn | Ghi chú |
|---|---|---|
| Wine prefix | `/home/admin/wine-kr/prefix` | tạo bằng `wineboot`, **không** dùng `~/.wine` |
| Fontconfig riêng | `/home/admin/wine-kr/fonts.conf` | chỉ trỏ `/home/admin/wine-kr/fonts` + `noto-cjk` (có **Noto Sans CJK KR**) |
| Locale Hàn riêng | `/home/admin/wine-kr/locale/ko_KR.UTF-8` | `localedef` vào thư mục riêng, dùng qua `LOCPATH` — **không cần root, không đổi locale hệ thống** |
| Bản chạy | `/home/admin/boomkr-run` (1,4 GB, hardlink) | đã bỏ `BlackCipher/`, `Dwarf/`, `CAWH.sys`… |

Chạy:

```bash
WINEPREFIX=/home/admin/wine-kr/prefix FONTCONFIG_FILE=/home/admin/wine-kr/fonts.conf \
LOCPATH=/home/admin/wine-kr/locale LC_ALL=ko_KR.UTF-8 wine CA.exe
```

Kết quả: client **chạy được dưới Wine** (khác bản VN: VN chết ở anti-VM `0xC0000096` trong Wine).
Dưới Wine client đi tới `ProcessPatch` rồi tự thoát sau **~0,8 s** (`CALog.log` không kịp ghi `EndOfInit`).

> Đổi codepage/ngôn ngữ hệ thống **không cần** nữa: locale Hàn chỉ tồn tại trong `LOCPATH` của prefix KR.

## 10. Dump image đã unpack từ Wine — **[VERIFIED một phần]**

`tools/krdump.py`: chạy `wine CA.exe`, tìm pid, chờ ~0,5 s (image đã unpack), đọc
`/proc/<pid>/maps` + `/proc/<pid>/mem`, ghi đè section của file gốc, và dump mọi vùng exec.

Kết quả lần chạy tốt nhất (`--delay 0.5`):

```
.idata     va=0x0197a000 size=0x200    <- mem
           va=0x0197b000 size=0x200    <- mem
gulxdsye   va=0x01c76000 size=0x1e6c00 <- mem   (vùng VM 1,99 MB)
gsowkarq   va=0x01e5d000 size=0x200    <- mem   (stub)
4 section đổi -> out/kr-unpacked.exe
384 vùng exec -> out/regions/
```

Hạn chế:
- `doctor` trên `out/kr-unpacked.exe` vẫn báo `PACKED`: **marker = 0** (KR vốn không có marker VN) và
  `.idata` vẫn `0x200` (chưa thấy IAT mới ở thời điểm dump).
- 384 vùng exec đã dump **không chứa marker**; code đã unpack có thể nằm ở vùng xuất hiện **sau** 0,5 s.
- Process chỉ sống ~0,8 s → cần dump muộn hơn (0,65–0,75 s) hoặc bắt thời điểm chuyển sang code unpack
  (ptrace/last-branch), rồi quét theo **mẫu dispatch** thay vì marker.

Việc tiếp: (a) dump ở 0,65 s và so sánh vùng exec mới; (b) hoặc tìm `jmp [reg*4]`/bảng handler trong
các vùng đã dump để nhận dạng VM KR.
