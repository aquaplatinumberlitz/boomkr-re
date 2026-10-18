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

## 11. Image unpacked + nhận dạng VM KR — **[VERIFIED]**

### Cách lấy được image

`tools/krdump.py --delay 3` (Wine bị làm chậm bởi `WINEDEBUG=+file` ghi ra file) → process sống đủ lâu
→ dump `/proc/<pid>/mem` → **6 section đổi**:

| section | va | size | entropy sau dump | nghĩa |
|---|---|---|---|---|
| (không tên → `.text`) | `0x00401000` | `0x764c00` | **5,92** | code game **đã giải mã** |
| `.rsrc` | `0x01698000` | `0x59c00` | 4,82 | resource |
| `.idata` | `0x0197a000` | `0x200` | 1,47 | vẫn nhỏ (IAT dựng động) |
| (trống) | `0x0197b000` | `0x200` | 0,84 | |
| `gulxdsye` | `0x01c76000` | `0x1e6c00` | **7,95** | bytecode VM (vẫn mã hoá) |
| `gsowkarq` | `0x01e5d000` | `0x200` | 4,42 | stub |

`out/kr-unpacked.exe` (10.141.152 B) — **đúng bằng kích thước file gốc**, chỉ nội dung section đổi.
`out/kr-unpacked-text.exe` = bản đã đặt tên section đầu thành `.text` và `gulxdsye` → `.themida`
(tool `vmdevirt` cần tên `.text` để không nhầm toàn bộ image là vùng VM).

### Nhận dạng VM — khác VN rõ rệt

```
python3 vmdevirt.py discover out/kr-unpacked-text.exe
  sites 102  distinct stub heads 26
```

| | VN | **KR** |
|---|---|---|
| marker `EB 10 57 4C 20` | 73 | **0** |
| site gọi vào VM | 607 | **102** |
| stub head | 390 (1 head dùng 73 lần) | **26** (mỗi head ~4 lần) |
| mẫu dispatch trong `.text` | `jmp [edi+eax*4]` | **457** chỗ `jmp [eax/ecx/edx*4+disp]` |
| vùng VM | 5 section Themida | 1 section `.themida` 1,9 MB + 2 section nhỏ |

⇒ **KR là đời VM khác**: không dùng marker, không có một entry dùng chung, và dispatch rải rác
(`eax/ecx/edx`). Theo phân loại trong `docs/vmdevirt.md`, đây nhiều khả năng là kiểu **chained/FALCON**
hơn là threaded của VN.

### Việc tiếp (G3 sâu hơn)

1. `profile` cần **snapshot tại một site** — chưa có. Có thể lấy bằng cách cho client chạy tới site
   (khó, vì nó chết ở `ProcessPatch`) hoặc emulate từ image unpack với ngữ cảnh dựng tay.
2. Trước đó: xác định **bảng handler** của KR bằng cách soi 457 chỗ `jmp [reg*4+disp]` — bảng nằm ở
   đâu (`.text`? vùng ẩn danh?), bao nhiêu entry.

## 12. Tìm bảng handler — **KHÔNG có bảng tĩnh** (kết quả quyết định) — **[VERIFIED]**

### Nơi code unpack thực sự nằm

Chuỗi log (`InitBeforePatch`, `ProcessPatch`, `EndOfInit! exitcode=`) **không có trong file dump theo
section**, mà nằm ở vùng private `0x00B66000` (18.956.288 B) — một vùng Themida cấp trong **khoảng trống
địa chỉ của image** (giữa `.text` kết thúc `0xA65C00` và `.rsrc` `0x1698000`).

| chuỗi | VA |
|---|---|
| `InitBeforePatch` | `0x017913C9` |
| `ProcessPatch` | `0x0179179C` |
| `EndOfInit! exitcode=` | `0x01792249` |

⇒ Khi dump "theo section" ta chỉ lấy được phần bị ghi đè, còn **code/string unpack thật nằm ở vùng
private**. Đây là lý do `doctor` vẫn báo PACKED.

### Quét dispatch — hai nơi, cùng một kết luận

| nơi | `jmp [reg*4+disp]` | bảng lớn nhất | đích |
|---|---|---|---|
| `.text` (image) | 457 | 164 entry | `.text` |
| vùng private `0xB66000` | **1.105** | 17 entry | cùng vùng |

- **Tất cả** bảng đều nhỏ (2–164 entry, phần lớn 2–8) và **không có bảng nào được nhiều điểm dispatch
  dùng chung** (`dùng bởi 1 điểm` cho gần như 100%).
- Không bảng nào trỏ vào `.themida` (vùng bytecode 1,9 MB).

⇒ Đây là **jump table C++ bình thường** (switch/case), **không phải bảng handler của VM**.
KR **không dùng bảng handler tĩnh theo opcode** như VN (178 entry trong ctx).

### Hệ quả

KR nhiều khả năng là kiểu **chained**: mỗi handler tự tính handler kế tiếp rồi nhảy thẳng (không qua bảng).
Điều này khớp với 3 dữ kiện khác: **0 marker**, mỗi stub head dùng ~4 lần (không có entry chung), và
bytecode vẫn mã hoá 7,95 trong `.themida`.

⇒ Pipeline `vmdevirt` của VN (`handlers` → `opcodes` → `profile`) **không dùng trực tiếp được** cho KR.
Cần backend **chained**: đi theo cạnh handler→handler, đọc "đích kế tiếp" từ thân handler.

### Việc tiếp

1. Theo 26 stub head vào `.themida` để xem mẫu vào VM (có prologue chung không).
2. Dựng **image lai** có map vùng `0xB66000` vào đúng VA (thêm section) để `capstone`/IDA nhìn thấy code
   unpack — hiện nó nằm ngoài file.
3. Muốn có runtime: phải qua `ProcessPatch` (patch nhánh thoát) — hiện là chặn duy nhất.

## 13. Wine: dừng ở privileged instruction (giống VN) — **[VERIFIED]**

### Bằng chứng

`WINEDEBUG=+relay` (dòng cuối trước khi chết):

```
Call KERNEL32.ExitProcess(c0000096) ret=140003800
```

`+seh`:

```
Exception 0xc0000096 (EXCEPTION_PRIV_INSTRUCTION) addr=0x020760B9
wow64cpu.dll + 0x1110        <- chạy qua WoW64 của Wine
```

`0x020760B9` = **byte đầu section `.themida`** (`0x02076000 + 0xB9`). Byte ở đó là `1E` = `push ds` —
hợp lệ trong 32-bit thật, **không hợp lệ trong chế độ 64-bit của WoW64** không dịch được
⇒ Wine (CachyOS, WoW64-only) không chạy nổi, **đúng như bản VN** (`docs/runtime.md`: `0xC0000096`).

### Hệ quả

| Môi trường | Kết quả |
|---|---|
| Wine (local) | chết `0xC0000096` tại `0x20760B9`; chỉ dump được image **một phần** (6 section, `.themida` vẫn mã hoá 7,95) |
| WinBoat (Windows thật) | đi qua điểm đó, tới `ProcessPatch`, thoát `exitcode=1` |

⇒ Muốn chạy KR **phải dùng Windows thật** (WinBoat), không phải Wine. Giá trị Wine mang lại: image
unpack một phần + xác nhận cơ chế chống VM nằm ở `.themida`.

### Debugger cũng bị chặn

`winedbg --gdb` + `break ExitProcess`: process **SIGSEGV tại `0x020760B9`** (cùng địa chỉ) trước khi tới
`ExitProcess` ⇒ có anti-debug/anti-VM ở đúng chỗ đó; `catch syscall exit_group` của gdb cũng không bắt được.

### Việc tiếp (đúng đường)

1. Trong **WinBoat**, dump bộ nhớ `CA.exe` (sửa `live_dump.ps1` bị `OverflowException` — nó cast
   `IntPtr.ToInt32()`; với process 32-bit dưới PS 64-bit phải dùng `ToInt64`).
2. Tìm xref tới chuỗi `ProcessPatch` (`0x17917xx` trong vùng private) ⇒ tìm hàm kiểm tra.
3. Patch nhánh thoát (`exitcode=1`) trên **bản copy** trong VM, chạy lại ⇒ nếu qua được thì VM KR mới
   bắt đầu giải mã `.themida`, lúc đó dump lại.

## 14. Dump trong Windows: image vẫn packed ở 1,1 s — **[VERIFIED]**

`tools/krdump_win.ps1` (bản 64-bit-safe; bản VN lỗi `OverflowException` vì `BaseAddress.ToInt32()`).
Chụp module `CA.exe` tại 4 mốc 300/600/900/1100 ms:

| mốc | kích thước | chuỗi `ProcessPatch` | entropy `.text` |
|---|---|---|---|
| 300 ms | 31.842.304 B | 0/2 | 7,90 |
| 600 ms | 31.842.304 B | 0/2 | 7,90 |
| 900 ms | 31.842.304 B | 0/2 | 7,90 |
| 1100 ms | 31.842.304 B | 0/2 | 7,90 |

⇒ **Trong Windows, image không hề unpack** trước khi client thoát: `ProcessPatch` thất bại **trước**
bước giải nén. (Chuỗi log nằm ở vùng private chỉ xuất hiện khi unpack; ở Wine ta bắt được vì client
treo trong `WinTrust` đủ lâu.)

### Hệ quả — chặn duy nhất hiện tại

| Hướng | Trạng thái |
|---|---|
| Wine | chết `0xC0000096` tại `0x20760B9` (`push ds` trong WoW64) |
| winedbg | anti-debug: SIGSEGV cùng địa chỉ, không tới `ExitProcess` |
| Windows, dump theo thời gian | image chưa unpack (client thoát ở ~1 s) |
| Windows, dump sau khi unpack | **không thể** vì unpack xảy ra *sau* `ProcessPatch` |

⇒ Muốn có image unpack của KR **bắt buộc phải qua `ProcessPatch`**. Ba cách, theo thứ tự khả thi:

1. **Đọc dialog tiếng Hàn** để biết nó đòi gì. Cách sạch: bật locale Hàn cho **riêng phiên** —
   đã có `wine-kr` nhưng Wine chết trước dialog; trong Windows phải đổi system locale (ảnh hưởng VN).
   Cách không ảnh hưởng VN: chạy client KR trong **một VM Windows thứ hai** hoặc snapshot VM rồi đổi
   locale, đọc xong khôi phục.
2. **Patch mù**: hook `ExitProcess` trong WinBoat bằng một DLL thay thế (đã có `tools/launcher/inject.c`
   làm mẫu) → khi client gọi `ExitProcess(1)`, log backtrace/return address → tìm ra chỗ thoát, rồi patch.
3. **Chặn `ProcessPatch`**: nếu nó gọi `WinTrust`/`WinINet` (đã thấy load các DLL đó), hook các API này
   để luôn trả "thành công" — có thể đủ để client đi tiếp mà không cần biết nội dung dialog.

## 15. Hook DLL + chạy được client ở trạng thái SỐNG — **[VERIFIED]**

### Công cụ

| File | Vai trò |
|---|---|
| `tools/krhook.c` → `krhook.dll` (32-bit) | inline hook: `WinVerifyTrust(Ex)`, `InternetOpen/Connect`, `HttpOpenRequest/SendRequest`, `InternetReadFile`, `ExitProcess`, `TerminateProcess`, `RtlExitUserProcess`, `MessageBoxA/W`, `WideCharToMultiByte` |
| `tools/krlaunch.c` → `krlaunch.exe` | `CreateProcess` suspended → `VirtualAllocEx`+`WriteProcessMemory`+`CreateRemoteThread(LoadLibraryA)` → `ResumeThread` |
| `tools/krdump_win.ps1` | dump theo mốc thời gian (`-Shots`) hoặc theo pid đang sống (`-Target`) |

### Kết quả

1. Inject OK (`LoadLibraryA -> OK (0x6e7e0000)`), hook cài đủ 12 hàm.
2. **Client không còn thoát ngay**: sống > 20 s, RSS 68–71 MB (so với 21 MB khi chết sớm).
3. **Dump theo pid sống → image ĐÃ UNPACK**:
   - có chuỗi `ProcessPatch`, `EndOfInit! exitcode=`
   - entropy giảm: `7,90 → 5,55` (vùng `.text`), `6,37`/`6,75` các vùng khác
   - `out/windump5/mod_00400000_CA.exe.bin` (31.842.304 B) — **ảnh module đã unpack thật**
4. Dialog tiếng Hàn vẫn hiện; bấm OK → `ExitProcess(0)` **từ `0x70E51811`** (CRT, không phải code game).

### Vì sao vẫn chưa đọc được nội dung dialog

`MessageBoxA` nhận được **bytes `3f` (`?`)** — tức chuỗi đã bị đổi wide→ANSI bằng ACP=1252 **trước**
MessageBox. Hook `WideCharToMultiByte` **không bắt được** ⇒ app dùng đường chuyển khác (resource string,
hoặc hàm tự viết). Muốn đọc phải: (a) hook `MultiByteToWideChar`/`LoadString*`, hoặc (b) đổi ACP của
**process** (không được trên Windows), hoặc (c) đọc thẳng resource trong `.rsrc` của image unpacked.

### Điểm mấu chốt cho bước sau

- Đã có **image unpacked** ⇒ có thể tìm resource/dialog text trực tiếp trong `.rsrc`.
- Đã có **process sống** ⇒ có thể dump bất cứ lúc nào, thậm chí sau khi bấm OK (nếu hook chặn `ExitProcess`
  bằng cách không gọi hàm thật — hiện hook chỉ log rồi gọi thật).
- `.themida` vẫn 7,95 ⇒ VM chưa chạy; phải qua `ProcessPatch` mới giải mã.
