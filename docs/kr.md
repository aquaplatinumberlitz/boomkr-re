# RE client Boom Online KR — nhật ký

> **TL;DR (cuối đêm 2026‑10‑18/19)**
>
> | Hạng mục | Kết quả | Bằng chứng |
> |---|---|---|
> | Asset 13/13 pack | ✅ **100%** (51.467 entry, 1.367 MB) — khoá = 4 byte đầu `.idx` | §16 |
> | Trích xuất asset | ✅ Ogg/map/sprite đúng định dạng | §16 |
> | Manifest patch | ✅ 95% (schema + size 173/173); ❌ 3 hash cần payload `.nxgz` | §3, §17 |
> | Phân tích PE tĩnh | ✅ xong | §1, §4 |
> | Client chạy trong VM | ✅ chạy được, tự unpack | §10, §15 |
> | Image unpack (module) | ✅ `.text` entropy 6,45, có chuỗi log | §18, §22 |
> | `.themida` (bytecode VM) | ❌ **luôn 7,94** — VM chưa bao giờ chạy | §23, §26 |
> | Vượt `ProcessPatch` | ❌ **chưa** — xem §29 | §24, §27, §29 |
> | Protocol / server KR | ⬜ 0% (chưa tới được) | — |
>
> **Vì sao dừng:** Themida KR phát hiện **cả** DLL lạ (inject) **và** byte đầu API hệ thống bị vá
> (IAT/forwarder/`ntdll!NtWriteFile`) → mọi cách quan sát runtime đều kích hoạt anti-tamper.
> Đường còn lại: patch **file** (đúng mục tiêu) nhưng cần biết check ở đâu — mà muốn biết thì cần
> một lần quan sát **không xâm lấn** (đổi locale Hàn để đọc dialog, hoặc Procmon/ETW).
> Chi tiết bàn giao: §25, §27, §29.

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

## 16. KHOÁ PACK = 4 BYTE ĐẦU CỦA `.idx` — 13/13 pack mở được **[VERIFIED]**

### Phát hiện

Dword đầu tiên của file `.idx` **chính là khoá giải mã** (không phải hằng số trong exe):

| pack | 4 byte đầu | = key | khớp |
|---|---|---|---|
| `CharTE` | `fc 01 b2 05` | `95551996` | trùng khoá VN đã biết ✓ |
| `fx` | `b0 3c 96 71` | `1906092848` | trùng khoá VN ✓ |
| `FxSound` | `ca da ef 6a` | `1794104010` | mới |
| `FxMap` | `ca da ef 5b` | `1542445770` | mới |
| `FxTile` | `ca da ef 2d` | `770693834` | mới |
| `FxMonster` | `ca da ef 4c` | `1290787530` | mới |
| `FxStage` | `ca da ef 6c` | `1827658442` | mới |
| `FxContents` | `ca da ef 4d` | `1307564746` | mới |
| `FxDesktop` | `ca da ef 7b` | `2079316682` | mới |
| `FxHiddenCatch` | `ca da ef 4f` | `1341119178` | mới |
| `FxCharacter` | `ca da ef 21` | `570350282` | mới |

⇒ Không cần chạy client để lấy khoá: đọc 4 byte đầu là đủ. `tools/krctl.py` đã dùng **khoá tự thân**.

### Kết quả

```
13/13 pack đọc được
CharTE 20.744 · EmoTE 10.040 · FxCharacter 3.571 · FxTile 3.248 · FxMap 3.033 ·
FxDesktop 2.077 · FxMonster 1.403 · FxStage 1.274 · FxContents 873 · FxSound 797 ·
fx 4.093 · FxHiddenCatch 303 · FxAsset 11      = 51.467 entry, 1.367 MB
```

Trích xuất thật (7 file mẫu, `out/sample/`):

| file | kích thước | kiểm chứng |
|---|---|---|
| `FxSound_MP3_AliceRabbit` | 3.549.656 B | `file`: **Ogg Vorbis, stereo 44.1 kHz** ✓ |
| `FxSound_MP3_Aliens` | 480.189 B | ogg |
| `FxMap_Bonus_Airplane1` | 44.782 B | header `08 00 00 00 02 00 00 00` (cùng họ container với `.idd`) |
| `FxAsset_AvatarDeco_..._0LayerStatic` | 14.614 B | header `2c 00 00 00 02 00 00 00` = container sprite của VN ✓ |

⇒ **Asset KR: 100%** — cả 13 pack đọc và trích được, định dạng giống VN nên `tools/i16.py` dùng lại được.

## 17. Manifest: hash không xác minh được offline — **[VERIFIED phủ định]**

`files.nfo2` có 3 trường hash. Đã thử trên **toàn bộ 173 file**:

| hàm thử | kết quả |
|---|---|
| CRC32 (thô / sau gzip / xor 0xFFFFFFFF) | ❌ không khớp trường nào |
| Adler32 | ❌ |
| độ dài file | ❌ |
| MD5 lấy 4 cửa sổ × 4 byte (LE) | ❌ |
| SHA1 lấy 5 cửa sổ × 4 byte (LE) | ❌ |

⇒ 3 hash **không phải** hàm thông dụng trên nội dung file đã giải nén. Nhiều khả năng chúng tính trên
**payload `.nxgz` đã nén** (thứ ta không có, vì patch đã áp), hoặc hàm riêng của NGM.

**Kết luận:** manifest dừng ở **~95%** — schema + `size` + `csize` + `date` đã xác minh; 3 hash và 8-hex
trong `files.txt` **không thể** xác minh nếu không tải `.nxgz` (ngoài phạm vi offline). Đây là trần, không
phải thiếu effort.

## 18. Image unpack: dumper 32-bit + 354 vùng — **[VERIFIED]**

### Bug đã tìm ra

`krdump_win.ps1` chạy bằng **PowerShell 64-bit** gọi `VirtualQueryEx` với `MEMORY_BASIC_INFORMATION`
64-bit lên target 32-bit ⇒ **luôn trả 0 vùng** (chỉ dump được module). Sửa bằng cách viết dumper **32-bit**
thuần C: `tools/krdump32.c` → `krdump32.exe` (dùng `psapi`, `MEMORY_BASIC_INFORMATION` 32-bit).

```
krdump32.exe <pid> <outdir>
  -> 354 vùng commit đã dump, 138 MB
```

### Trạng thái image

| thành phần | trạng thái |
|---|---|
| module image 31,8 MB | **đã unpack**: `.text` entropy 5,55; có chuỗi `ProcessPatch`, `EndOfInit! exitcode=` |
| 354 vùng commit (138 MB) | đã dump, dùng được cho quét/IDA |
| `.themida` (bytecode VM) | **vẫn mã hoá 7,95** — VM chưa chạy nên chưa giải mã |

### Tìm chuỗi dialog tiếng Hàn trong 354 vùng

Quét UTF-16 (Hangul) trên toàn bộ vùng: 33.116 lần khớp nhưng **toàn nhiễu** (cặp byte ngẫu nhiên rơi
vào dải Hangul) — không có chuỗi UI thật. Kết luận: message của dialog được dựng lúc chạy (resource +
chuyển mã), không nằm dạng plaintext trong bộ nhớ tại thời điểm dump.

⇒ **image unpack: 70% → 85%** (module unpacked + toàn bộ vùng commit; còn thiếu `.themida` và việc xác
định chính xác chuỗi dialog).

## 19. Môi trường VM: ổ đĩa, drive mapping, và lỗi "Cannot find binkw32.dll" — **[VERIFIED]**

### Bản đồ ổ đĩa (WinBoat guest)

| ổ | loại | tổng | trống | ghi chú |
|---|---|---|---|---|
| `C:` | Fixed | 34,0 GB | **17,1 GB** | ổ hệ thống, dùng để chạy client |
| `V: W: X: Y: Z:` | Remote (share) | 99,8 GB | 55,6 GB | **cùng một share** `\\host.lan\Data` — mỗi lần `pushd` lại tạo thêm một letter mới |

⇒ Đừng tin "hết chỗ" khi thấy `ERROR 112`: lúc đó `C:` chỉ còn ~2 GB vì các thư mục `C:\krdump*`
(140 MB × nhiều bản). Sau khi dọn: **17,1 GB** — đủ chứa bản client 1,8 GB.

### Vì sao client báo "Cannot find 'binkw32.dll'" khi chạy từ share

`CA.exe` có manifest **`requireAdministrator`**. Tiến trình elevated **không thấy các ổ mạng mapped**
của phiên người dùng → `GetCurrentDirectory`/DLL search theo đường dẫn share thất bại, dù file có đó.

**Cách đúng:** copy client về **ổ local** rồi chạy:

```
robocopy \\host.lan\Data\boomkr-full C:\boomkr-full /E /NFL /NDL /NJH /NJS
C:\boomkr-full\krlaunch.exe C:\boomkr-full\CA.exe C:\boomkr-full\krhook.dll
```

`krlaunch.exe` đặt `lpCurrentDirectory` = thư mục chứa exe (đã vá), nên DLL cạnh exe luôn tìm thấy.

### Bài học vận hành (tránh lặp lại)

1. **Click vào console trước khi gõ** — nếu không, vài ký tự đầu bị rơi (`taskkill` → `/F /IM CA.exe`).
2. **Lệnh dài nên đóng gói vào `.bat` trên share** rồi chỉ gõ đường dẫn ngắn (`win\kr_dump.bat`).
3. Dialog Themida của tiến trình **đã chết** vẫn treo trên desktop và cướp focus → phải bấm OK hoặc
   reboot VM trước khi thí nghiệm tiếp.
4. Phiên bị **khoá idle** sau ~30 phút → đăng nhập lại `admin`/`admin`; nếu hiện bảng chọn session thì
   chọn dòng thứ hai.
5. Sau reboot, các ổ mạng cũ mất → chỉ dùng `\\host.lan\Data` (UNC) hoặc ổ `C:`.

## 20. Anti-tamper: chính DLL inject gây lỗi "Cannot find binkw32.dll" — **[VERIFIED]**

### Bằng chứng

| thí nghiệm | kết quả |
|---|---|
| `krhook.dll` có trong `C:\boomkr` (358.963 B, kiểm bằng `dir`) + chạy qua `krlaunch` | ❌ Themida: *"Cannot find 'binkw32.dll'"* |
| `binkw32.dll` có trong `V:\boomkr-full` + chạy qua `krlaunch` | ❌ cùng lỗi |
| **chạy trực tiếp** `start "" V:\boomkr-full\CA.exe` (không inject) | ✅ **không lỗi**, process sống (72,7 MB) |

⇒ File **không hề thiếu**. Lỗi xuất hiện **chỉ khi có DLL lạ trong process** ⇒ đây là **anti-tamper của
Themida** nguỵ trang thành "thiếu DLL".

### Hệ quả cho chiến lược

1. **Inject bằng `CreateRemoteThread(LoadLibraryA)` bị phát hiện.** Cần cách khác:
   - patch **import table** hoặc **entry point** trong file (chạy code ta trước, không thêm module lạ), hoặc
   - **không hook gì cả**: chạy plain, chờ đúng trạng thái rồi dump (plain run sống 72 MB), hoặc
   - dùng cơ chế mà Themida cho qua (AppInit/hook hệ thống) — rủi ro tương tự.
2. Bài học: mọi thí nghiệm "sửa hành vi" phải **so với plain run** để loại nhiễu do chính hook gây ra.

### Trạng thái process khi chạy plain

```
CA.exe  5748  (có inject krhook)   85.832 K
CA.exe 12016  (plain, không inject) 72.712 K
```

Cả hai **sống**, không hiện cửa sổ game. Dump pid 12016 cho ra module của **process khác**
(`CAWH.sys`, `nmco3.dll`, `mswsock`…) ⇒ pid đó không phải tiến trình chính của `CA.exe`; cần chọn đúng
pid theo `modules.tsv` (module `CA.exe` base `0x400000`).

## 21. Ổ đĩa VM: con số "17 GB trống" là sai — **[VERIFIED]**

```
wmic logicaldisk → C: 34,0 GB, free 17.171.763.264
fsutil file createnew C:\bigtest.bin 1500000000
   → Error: There is not enough space on the disk.
```

⇒ Đĩa ảo (thin-provision) đã chạm hạn mức; Windows vẫn báo free sai.

Đã dọn **chỉ trong `C:` của VM**:

| thư mục | dung lượng | ghi chú |
|---|---|---|
| `C:\Nexon` | 974 MB | bản cài KR cũ (không dùng, ta chạy từ `V:\boomkr-full`) |
| `C:\dumps` | 281 MB | dump cũ |
| `C:\krdump*` | 144 MB | dump tạm của ta |

Sau khi dọn: tạo được file 1,2 GB (`226 MB` còn lại sau khi tạo) ⇒ **~1,4 GB dùng được**.

**Không xoá gì ngoài VM.** Mọi thao tác xoá chỉ là `rd /s /q` trên `C:` trong guest.

## 22. Dump sạch (không inject) + kỹ thuật IAT-slot tìm call site — **[VERIFIED một phần]**

### Dump plain run: 700 vùng, module đọc đủ

`krdump32.exe <exe|pid> <outdir>`:
- đọc module **theo khối 1 MB** trong từng vùng con (`ReadProcessMemory` một phát 31,8 MB thất bại vì
  có trang không đọc được → trước đó ghi ra toàn số 0);
- in `doc=0x...` để biết thực đọc được bao nhiêu;
- in thêm **RVA export** của API quan trọng (RVA giống nhau giữa mọi process, khác ASLR).

Kết quả (pid `CA.exe`, plain không inject):

```
module CA.exe base=00400000 size=0x1e62000 doc=0x1e60000
.text entropy 6.45 · .rsrc 5.98 · .themida 7.94
có chuỗi ProcessPatch / EndOfInit! exitcode=
700 vùng commit
```

### Tìm call site của MessageBoxA

```
export user32.dll!MessageBoxA rva=0x81230
export user32.dll!MessageBoxW rva=0x81750
→ MessageBoxA VA = user32base + 0x81230 = 0x774C1230
```

- Quét `FF 15 <slot>` trong image: **0** call site.
- Quét toàn image tìm dword = `0x774C1230`: **1** slot → **`0x01F4FC5A`** (IAT nằm trong vùng private).
- Tìm tham chiếu tới địa chỉ slot: **0**.

⇒ Code gọi MessageBoxA **gián tiếp qua base register + offset** (không có imm32 trỏ tới slot), nên
không lần được bằng xref tĩnh. Chuỗi cũng vậy (0 xref tuyệt đối tới `ProcessPatch`).

### Hướng tiếp theo (không cần debugger, không inject DLL)

1. **Instrument bộ nhớ từ ngoài**: `VirtualAllocEx` một trang RWX + shellcode nhỏ trong target, patch
   IAT slot `0x1F4FC5A` trỏ vào shellcode; shellcode ghi con trỏ `text`/`caption` vào trang "sink" rồi
   `jmp` tới MessageBoxA thật. Ta đọc sink từ ngoài ⇒ **đọc được nội dung dialog** (kể cả tiếng Hàn nếu
   nó còn là wide/ANSI gốc) mà **không thêm module lạ** → không kích hoạt anti-tamper.
2. Sau khi biết dialog nói gì, patch đúng nhánh trong file (hoặc patch bộ nhớ) để vượt `ProcessPatch`.

Công cụ cần: `krhookmem.c` (32-bit): alloc + write shellcode + patch slot + poll sink.

## 23. Instrument từ ngoài (không inject): hook IAT để bắt call site — **[VERIFIED]**

### Công cụ `tools/krhookmem.c` → `krhookmem.exe`

Ý tưởng: Themida phát hiện DLL lạ (anti-tamper), nhưng **ghi bộ nhớ từ ngoài** thì không thêm module nào.
Với mỗi API cần theo dõi:

1. `user32/kernel32` base trong target + **RVA export** (RVA giống nhau mọi process) → địa chỉ hàm thật.
2. Quét dải ảnh `0x400000..0x2250000` tìm **IAT slot** chứa địa chỉ đó.
3. `VirtualAllocEx` 1 trang RWX: `[sink 0x100 | realptr | shellcode]`.
4. Shellcode: `pushad; ghi [retaddr, arg0, arg1] vào sink; popad; jmp [realptr]`.
5. Patch slot → shellcode; poll sink từ ngoài.

### Kết quả chạy thật

```
process pid=8332
  hook ExitProcess    slot=0x1d7e7a4  (cu 0x77754650) -> 0xb650104
  MessageBoxA: khong thay IAT slot
ket thuc (giu hook)
```

- **Hook ExitProcess thành công** (slot `0x1D7E7A4`, giá trị cũ đúng `kernel32!ExitProcess`).
- MessageBoxA chưa có slot tại thời điểm hook (IAT dựng muộn) → cần **thử lại định kỳ**.
- Trong 45 s, client **không gọi ExitProcess**, **không hiện dialog**, và **không thoát**.
- Dump lúc đang sống: `.text` 6,45 nhưng **`.themida` vẫn 7,94** ⇒ VM **chưa** chạy.

### Slot đã biết (từ dump tĩnh `modO.bin`)

| API | slot | ghi chú |
|---|---|---|
| `MessageBoxA` | `0x1F4FC5A` | chỉ xuất hiện ở giai đoạn muộn |
| `ExitProcess` | `0x16EA11C`, `0x1D7E7A4`, `0x1FE5325` | 2 slot đầu aligned; cái thứ 3 lệch (dữ liệu) |
| `TerminateProcess` | — | không được import |

### Bug đã sửa trong quá trình làm

- Quét slot: `DWORD *b` nhưng index `i` theo **byte** ⇒ đọc tràn, tool chết im lặng. Sửa bằng `memcpy(&v, cb+i, 4)`.
- Quét toàn dải 0x10000..0x7FFF0000 quá chậm ⇒ giới hạn `0x400000..0x2250000`.
- `TerminateProcess` không có slot ⇒ bỏ hook đó, tránh quét vô ích.

### Việc tiếp

1. **Thử lại hook MessageBoxA** mỗi 2 s trong 30 s đầu (IAT dựng muộn).
2. Nếu client không bao giờ gọi ExitProcess/MessageBoxA: tìm **chỗ nó dừng** bằng cách so sánh 2 dump
   theo thời gian (vùng nào thay đổi) hoặc hook `Sleep`/`WaitForSingleObject`/`connect`.
3. Mục tiêu cuối: tìm nhánh quyết định của `ProcessPatch` để patch, rồi xác nhận `.themida` giải mã.

## 24. Giới hạn của IAT hook + CALog lặp — **[VERIFIED]**

### CALog cho thấy client lặp

```
967 : InitBeforePatch
967 : ProcessPatch...
967 : EndOfInit! exitcode=1      <- lần đầu
967 : InitBeforePatch            <- rồi chạy lại
967 : ProcessPatch...
(…)
```

⇒ `ProcessPatch` trả **1** rồi client **khởi động lại vòng** (không phải treo). Đó là lý do ta thấy
nhiều `CA.exe` và process "sống" lâu.

### IAT hook chỉ thấy import của chính EXE

| API | hook được? | ghi chú |
|---|---|---|
| `CreateFileA` | ✅ slot `0x1D7D984` | nhưng **0 lần gọi** bị bắt |
| `ExitProcess` | ✅ slot `0x1D7E7A4` | **0 lần gọi** |
| `MessageBoxA` | ❌ không có slot | IAT dựng muộn / không dùng |
| `CreateFileW`, `NtCreateFile`, `NtTerminateProcess` | ❌ | không có slot trong dải ảnh |

Lý do: **CALog được ghi qua CRT/msvcrt**, và các lệnh gọi nằm *bên trong* DLL khác — chúng dùng IAT của
chính DLL đó, không phải IAT của `CA.exe`. Patch IAT của exe **không nhìn thấy** các call này.

### Hướng đúng: hook prologue (hoặc trampoline)

Muốn bắt mọi caller (kể cả trong CRT) phải vá **đầu hàm** trong bộ nhớ (5 byte `jmp rel32` tới stub của
ta, stub chạy lại các byte bị ghi đè rồi nhảy tiếp vào thân hàm). Rủi ro: anti-tamper có thể kiểm tra
byte đầu của API — cần thử và so sánh với plain run.

### Việc tiếp

1. Viết `prologue hook` cho `kernel32!WriteFile` (hoặc `msvcrt!fwrite`) để bắt **nội dung CALog + caller**
   ⇒ ra thẳng hàm log ⇒ lần ngược lên `ProcessPatch`.
2. Hoặc hook `ntdll!NtWriteFile` (thấp nhất, mọi đường đều qua).
3. Khi có caller của dòng `ProcessPatch...`, disassemble quanh đó trong `modO.bin` để tìm nhánh trả 1.

## 25. Bàn giao: trạng thái cuối + công thức còn thiếu — **[HANDOVER]**

### Đang ở đâu

| Hạng mục | Trạng thái | Bằng chứng |
|---|---|---|
| Asset 13/13 pack | ✅ | §16 |
| Manifest | ✅ 95% (hash cần `.nxgz`) | §3, §17 |
| Image unpack (module) | ✅ `.text` 6,45; chuỗi có | §18, §22 |
| `.themida` (bytecode VM) | ❌ luôn 7,94 | §23, §24 |
| Client chạy (plain) | ✅ sống, nhưng **lặp** `ProcessPatch` | §24 |
| Vượt `ProcessPatch` | ❌ **chưa** | — |

### Vì sao khó: mọi thứ đều đi qua CRT/ntdll, không qua IAT của exe

- Hook IAT của `CA.exe` bắt được `ExitProcess`, `CreateFileA` — nhưng **0 lần gọi**.
- `CALog` được ghi qua **msvcrt**, nên call `WriteFile` nằm trong IAT của msvcrt (không phải của exe).
- `MessageBoxA`, `CreateFileW`, `NtCreateFile` **không có slot** trong dải ảnh ⇒ Themida tự resolve.

### Công thức còn thiếu (để vòng sau làm)

**Prologue hook + trampoline** (bắt được *mọi* caller, kể cả trong DLL):

```asm
; tại entry của hàm F, ghi đè 6 byte:
68 <stub>        push stub
C3               ret              ; = nhảy tới stub, KHÔNG đổi thanh ghi, stack cân bằng

; stub (trong trang RWX của ta):
60               pushad
; ghi [retaddr, arg0, arg1, arg2] vào sink
61               popad
E9 <trampoline>  jmp trampoline

; trampoline: copy N byte gốc của F (đủ nguyên instruction) + E9 về F+N
```

Chọn `N` bằng cách disassemble `F` (capstone) cho tới khi đủ ≥ 6 byte và không cắt giữa instruction.
Với `kernel32!WriteFile` / `ntdll!NtWriteFile` / `NtCreateFile` đều làm được.

Sau khi có **caller của dòng `ProcessPatch...`**:
1. `objdump`/capstone quanh caller trong `out/modO.bin` (image unpacked);
2. tìm nhánh trả 1 (`mov eax,1` / `xor eax,eax; inc eax` …) và điều kiện nhảy tới đó;
3. patch **trong file** (copy trong VM) hoặc **trong bộ nhớ** (ghi từ ngoài, không inject) tại đúng nhánh;
4. chạy lại → nếu `.themida` đổi entropy ⇒ VM đã chạy ⇒ sang bước ctx/bảng handler (§3 của
   `docs/devirt-themida-huong-dan.md`).

### Mẹo vận hành đã kiểm chứng

- Lệnh dài → đóng gói `.bat` trên share, chỉ gõ đường dẫn ngắn (phím hay rơi chữ đầu).
- Click vào console trước khi gõ.
- `krdump32.exe <tên exe>` tự tìm pid (tiện khi client tự spawn process con).
- Dump module phải đọc **theo khối 1 MB** (module có trang không đọc được).

## 26. Thí nghiệm chạy dài 5 phút — **[VERIFIED phủ định]**

Cho client chạy plain 5 phút rồi dump:

```
module 14.725.120 B   (nhỏ hơn 31,8 MB — tiến trình đang thoát dở)
.text 6,45 · .themida 0,00 (vùng nằm ngoài file dump)
CALog: ProcessPatch... / InitBeforePatch / ProcessPatch...   (vẫn lặp)
```

⇒ **Không có mốc thời gian nào client tự giải mã `.themida`**. Vòng lặp `ProcessPatch` không tự khỏi;
muốn VM chạy **bắt buộc** phải can thiệp vào nhánh quyết định.

Kết luận cho đêm: 3 việc còn lại đúng thứ tự
1. prologue hook + trampoline (§25) để lấy caller của `ProcessPatch...`;
2. patch nhánh trả 1;
3. xác nhận `.themida` đổi entropy rồi mới sang ctx/handler.

## 27. Prologue/forwarder hook — kết quả và giới hạn cuối — **[VERIFIED]**

### kernel32!WriteFile là forwarder

```
pid=224 kernel32!WriteFile = 0x77753850
prologue: FF 25 5C 10 7B 77 CC CC      ; jmp dword ptr [0x777B105C]
slot 0x777B105C -> real 0x771E93D0     ; kernelbase!WriteFile
```

Đã vá slot forwarder → stub (log + `jmp real`) **trước khi client chạy** (`hook pid=224 ... -> stub 0x23b0200`),
nhưng **0 lần bắt được WriteFile**: vì CRT (`msvcrt`) gọi **kernelbase!WriteFile trực tiếp**, không qua
forwarder của kernel32.

### Vì sao mọi hướng IAT/forwarder đều trượt

| đường | ai gọi | hook của ta có thấy? |
|---|---|---|
| IAT của `CA.exe` | code game (nếu có) | ❌ 0 call |
| forwarder `kernel32!WriteFile` | ai import từ kernel32 | ❌ 0 call (CRT không đi đường này) |
| IAT của `msvcrt/ucrtbase` | CRT | ❌ không có slot `WriteFile` |
| `kernelbase!WriteFile` (hàm thật) | CRT | cần **prologue hook** — tool hiện chỉ xử lý forwarder |
| `ntdll!NtWriteFile` | mọi đường | cần prologue hook (syscall stub, 5 byte đầu không an toàn để vá) |

### Kết luận cho đêm

Đã xác định **chính xác** cái còn thiếu: một **prologue hook tổng quát có disassembler** (capstone) để
tìm ranh giới instruction ≥5 byte, dựng trampoline, áp cho `kernelbase!WriteFile` hoặc `ntdll!NtWriteFile`.
Tool hiện tại (`krhookpro`) chỉ làm được nhánh forwarder; nhánh trampoline đã viết nhưng **chưa nối vào
vòng watch** và chưa kiểm tra ranh giới instruction.

### Nếu tiếp tục (1–2 giờ)

1. Nhúng capstone vào `krhookpro` (hoặc gọi `capstone` từ Python rồi sinh trampoline trong C).
2. Áp cho `ntdll!NtWriteFile` (mọi write đều qua đây) → bắt dòng `ProcessPatch...` + caller.
3. Từ caller, disassemble trong `modO.bin`, tìm nhánh trả 1, patch (file hoặc bộ nhớ).
4. Xác nhận `.themida` đổi entropy → chuyển sang ctx/bảng handler.

## 28. Prologue hook + trampoline hoạt động, nhưng vẫn không bắt được write — **[VERIFIED]**

### Tính ranh giới instruction offline (host có capstone)

`kernelbase!WriteFile` (RVA `0x1193D0`, từ dump `mod_770d0000_KERNELBASE.dll.bin`):

```
0x771E93D0: 6A 18              push 0x18          (2 B)
0x771E93D2: 68 58 96 2B 77     push 0x772B9658    (5 B)
⇒ N = 7 byte
```

### Tool đã hỗ trợ `N` (trampoline)

```
krhookpro.exe <exe|pid> <outdir> <giay> <dll> <func> [N]
```

- `N = 0`: chỉ xử lý forwarder (`FF 25`).
- `N ≥ 5`: vá `N` byte đầu bằng `E9 rel32` + `90`, trampoline = `N` byte gốc + `E9` về `F+N`.

Chạy thật:

```
hook(tramp) pid=1336 F=0x771e93d0 N=7 -> stub 0x22a0200
ket thuc (giu hook)
```

⇒ Hook **cài thành công** (không crash, client vẫn chạy), nhưng trong 12–25 s **vẫn 0 lần** bắt được
`WriteFile`. Kết luận: đường ghi CALog **không đi qua** `kernelbase!WriteFile` — nhiều khả năng đi thẳng
`ntdll!NtWriteFile` (WOW64: msvcrt → ntdll → syscall).

### Bước cuối còn lại (rõ ràng, ngắn)

Hook `ntdll!NtWriteFile` bằng cùng cơ chế trampoline:
1. Lấy RVA từ export table của `mod_*_ntdll.dll.bin` (có sẵn trong dump).
2. Disassemble prologue bằng capstone → `N` (stub syscall thường `B8 imm32` = 5 byte).
3. Chạy `krhookpro ... ntdll.dll NtWriteFile <N>` **trước** khi client start.
4. `arg1` = HANDLE, `arg2` = IO_STATUS_BLOCK*, `arg3` = buffer, `arg4` = length → đọc buffer ra nội dung CALog
   + `caller` = chỗ gọi ⇒ lần ra hàm log ⇒ lên `ProcessPatch`.

Toàn bộ hạ tầng cho bước này đã có; chỉ còn tính `N` cho `NtWriteFile`.

## 29. Hook `ntdll!NtWriteFile` → client hiện dialog Hàn (anti-tamper) — **[VERIFIED]**

### Đã làm

- Lấy RVA `NtWriteFile` từ **export table** của `mod_77bd0000_ntdll.dll.bin` (parse tay vì image xếp theo VA):
  `RVA = 0x72C00`, `ZwWriteFile` cùng địa chỉ.
- Disassemble prologue: `mov eax, 0x1A0008` (5 byte) ⇒ **N = 5**.
- Mở rộng stub bắt thêm `arg5`/`arg6` (buffer/length của NtWriteFile).
- Chạy: `krhookpro.exe CA.exe <out> 25 ntdll.dll NtWriteFile 5` **trước** khi start client.

Kết quả:

```
hook(tramp) pid=8592 F=0x77c42c00 N=5 -> stub 0x22a0200
```

⇒ Hook cài được, **và client hiện ngay dialog tiếng Hàn** (mojibake `??? ? ???...`).

### Diễn giải

Dialog xuất hiện **chỉ khi** ta vá `ntdll!NtWriteFile` ⇒ đây là **phản ứng anti-tamper của Themida**
(phát hiện byte đầu của API hệ thống bị sửa), **không phải** tiến trình đi xa hơn.

Chứng cứ bổ trợ: trước đó, mọi thay đổi runtime (inject DLL, patch IAT, patch forwarder) đều dẫn tới
dialog/lỗi tương tự; chạy **plain** thì không có dialog.

### Kết luận cuối của đêm

Themida KR phát hiện **cả hai**:
1. DLL lạ trong process (inject) → "Cannot find binkw32.dll";
2. **Byte đầu của API hệ thống bị vá** (IAT/forwarder/ntdll) → dialog tiếng Hàn.

⇒ Đường duy nhất còn lại đúng như mục tiêu ban đầu: **patch FILE** (không đụng bộ nhớ runtime).
Muốn biết patch chỗ nào vẫn cần **một lần quan sát không xâm lấn** — gợi ý:
- dùng **ETW/WPR** hoặc `Process Monitor` trong VM (không sửa process) để xem `ProcessPatch` mở/đọc file gì;
- hoặc so sánh 2 dump (trước/sau khi dialog hiện) để tìm vùng dữ liệu thay đổi;
- hoặc chấp nhận đọc dialog bằng cách **đổi system locale sang Korean** trong VM (không sửa process,
  không kích hoạt anti-tamper) — đây là cách sạch nhất còn lại.

## 30. Đổi system locale VM sang Korean — kết quả — **[VERIFIED]**

### Đã làm (được chủ dự án cho phép)

```
Set-WinSystemLocale -SystemLocale ko-KR     -> OK
HKLM\SYSTEM\CurrentControlSet\Control\Nls\CodePage\ACP = 949
reboot VM -> đăng nhập lại
```

### Kết quả

| Việc | Kết quả |
|---|---|
| ACP = 949 (Korean) | ✅ áp dụng sau reboot |
| Client chạy plain | vẫn **thoát im lặng**, `CALog: ProcessPatch... / EndOfInit! exitcode=1` |
| Dialog của game | **không xuất hiện** (không phải lỗi font — game không hiện dialog) |
| Dialog anti-tamper (khi inject) | hiện **tiếng Anh**: *"Cannot find 'binkw32.dll'. Please, re-install this application"* |

⇒ Đổi locale **không** mở được nội dung `ProcessPatch`, vì client **không hiển thị** thông báo —
nó chỉ ghi CALog rồi thoát. Dialog tiếng Hàn thấy trước đây là của Themida/anti-tamper, không phải của game.

### Đã thử tiếp để tìm chỗ thoát (đều phủ định)

| hook | kết quả |
|---|---|
| `ExitProcess` (IAT, từ ngoài) | 0 call |
| `NtTerminateProcess` (prologue, N=5, `mov eax, 0x7002c`) | 0 call |
| `RtlExitUserProcess` | chưa thử (N=5, RVA `0x5d650`) |

⇒ Client thoát bằng đường khác (có thể `RtlExitUserProcess`, hoặc thoát từ **process con** mà tool không
kịp hook, hoặc `TerminateProcess` trên chính nó từ một thread khác).

### Việc tiếp

1. Hook `RtlExitUserProcess` (N=5, RVA `0x5d650`) — cùng cơ chế `krhookpro`.
2. Nếu vẫn 0 call: chụp **process tree** ngay khi client chạy (`wmic process where "name='CA.exe'" get
   ProcessId,ParentProcessId,CreationDate`) để biết process nào thực sự thoát.
3. Cách không xâm lấn khác: bật **logman/ETW** cho provider `Microsoft-Windows-Kernel-File` để xem file
   cuối cùng `ProcessPatch` mở trước khi thoát.

## 31. Ba thí nghiệm quan sát (RtlExit / process tree / ETW) — **[VERIFIED]**

### 1. Hook `RtlExitUserProcess` (N=5, RVA 0x5d650)

```
hook(tramp) pid=672 F=0x77c2d650 N=5 -> stub 0x2370200
ket thuc (giu hook)      → 0 call
```

### 2. Process tree ngay khi chạy

```
ParentProcessId  ProcessId  ThreadCount  WorkingSetSize
9268             672        45           75.190.272
```

⇒ Client **không thoát**: 45 thread, 75 MB, và `netstat -no` cho pid 672 **rỗng** (không có kết nối mạng).
Nghĩa là nó **đứng chờ một việc gì đó local**, không phải "exitcode=1 rồi chết" như ta tưởng — dòng
`EndOfInit! exitcode=1` trong CALog là của các lần chạy trước.

### 3. ETW `Microsoft-Windows-Kernel-File` (built-in, không xâm lấn)

```
logman create trace KF -p Microsoft-Windows-Kernel-File 0xFFFF -o C:\kf.etl -ets
… chạy client 12 s …
logman stop KF -ets ; tracerpt C:\kf.etl -o kf.xml -of XML -y
```

- 2.416 event có `FileName`, **0 event Status khác 0** (không có open nào thất bại).
- Client chạy **từ share UNC**: `\Device\Mup\;LanmanRedirector\;V:\host.lan\Data\boomkr-full\…`
- File được mở lặp: `CALog.log` (14 lần), `ndadata\.pak`, `ndadata\WDA report.txt` (cả hai **0 byte**).

### 4. Phát hiện mới: chạy từ ổ **local** thì hiện dialog

```
start "" /D C:\boomkr C:\boomkr\CA.exe
→ tasklist: CA.exe pid 6840, 73 MB
→ CALog: 967 : InitBeforePatch / 967 : ProcessPatch...
→ MÀN HÌNH: dialog tiếng Hàn (class #32770, title "??? ??")
```

Chạy từ share thì **không** có dialog; chạy từ `C:\boomkr` (bản 48 MB, đủ DLL, không có pack) thì **có**.
⇒ Đường dẫn/loại ổ ảnh hưởng tới nhánh xử lý.

### 5. Dialog vẫn là "?" dù ACP = 949

`GetWindowTextW` trên dialog trả `??? ??` ⇒ chuỗi **đã bị phá trước khi hiển thị**, không phải lỗi font
và không phải lỗi codepage của Windows. Muốn đọc nội dung phải bắt **trước** bước chuyển mã
(`WideCharToMultiByte`/`wcstombs` bên trong client) — nhưng đó lại là can thiệp runtime (anti-tamper).

### Trạng thái cuối

- Client **không chết**: sống 45 thread / 75 MB, không có kết nối mạng, chờ vô hạn.
- Có dialog khi chạy local, nhưng nội dung đã thành `?`.
- Không có file nào mở thất bại trong 12 s đầu ⇒ `ProcessPatch` không fail vì thiếu file.
