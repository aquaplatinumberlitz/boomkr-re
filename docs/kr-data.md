# Dữ liệu game KR: cái gì nằm trong pack, cái gì nằm trong code/server

Trả lời câu hỏi: **công thức Magic Market, tỉ lệ rơi, thông số physics** có RE được từ client không.

## 1. Kiểm kê toàn bộ bảng dữ liệu trong 13 pack

Quét cả 13 pack, tìm entry không phải sprite/ogg/map:

| Entry | Kích thước | Định dạng | Nội dung |
|---|---|---|---|
| `fx/TileInfo/TileInfoList` | 1,18 MB | XML + hex | **5.929 tile**, mỗi tile 1 khối `Data` 72 byte |
| `fx/ObjectTable/ObjectTable` | 2,39 MB | nhị phân | **8.185 object**: bảng tên→offset + blob định nghĩa animation |
| `fx/Pet/PetTable`, `PetRandomTable` | 104 KB + 184 KB | Excel XML | thuộc tính pet, bảng random |
| `fx/MotionBlur/MotionBlurTable` | 59 KB | Excel XML | bảng motion blur |
| `FxDesktop/ClientData/ClientString` | 892 KB | XML cp949 | **6.993 chuỗi** UI (LangID 1042) |
| `fx/ObjectTable/NameTable`, `SpeakerTable` | 3 KB / 174 KB | XML / nhị phân | tên script object, câu thoại |
| `FxMap/Map/*` | 1.151 map | nhị phân | lưới + `<mapscript>` (770 map có script) |
| `CharTE/ObjectTable/*`, `EmoTE/*`, `FxAsset/*` | 1,2 MB + 916 KB + 70 KB | nhị phân | bảng object theo pack |

### Định dạng đã giải

**ObjectTable** (`8185` entry):

```
u32 count
lặp count lần: u32 nameLen, name[cp949], u32 offset   (offset = 0xFFFFFFFF với entry thư mục)
... blob 2,13 MB: mỗi object = danh sách state (Default/Flame/Timer/User/Pang/Broken/On…)
    mỗi state = u32 nameLen, name, u32 nframes, nframes × u32 frameId
```

Kiểm chứng: đọc đúng 8.185 entry, offset đầu tiên `BnbNavi/NewIcon → 264991`, `Bomb/BasicBomb → 265154`.

**TileInfoList**: `<Tile Name="Tile00_Box/BoxGray" Data="0200000000…"/>`, `Data` = 72 byte.
Các tile khác nhau ở vài word đầu (ví dụ `Ground/Cross00` có thêm `ffffffff`), phần còn lại là
danh sách frame/state giống ObjectTable ⇒ **đây là định nghĩa hiển thị/animation của tile, không phải
ma trận vật lý**.

## 2. Công thức Magic Market — **KHÔNG có trong client** [VERIFIED phủ định]

Bằng chứng:

1. Không có bảng recipe/item nào trong 13 pack (bảng đầy đủ ở §1).
2. `ClientString` (6.993 chuỗi) **không có** chuỗi nào chứa 합성/제작/분해/확률/drop;
   chỉ có 2 chuỗi chứa "조합" và cả hai đều không liên quan (quy tắc mật khẩu, lời thoại monster).
3. Chuỗi UI chế tạo (`Synthesis`, `ProbabilityStatus`, `ProtectionStatus`, `MixItem`…) chỉ nằm trong
   **exe** (ASCII), tức code UI — không kèm số liệu.
4. UI có sẵn ô `ProbabilityStatus` / `ProtectionStatus` ⇒ **xác suất do server gửi xuống**, client chỉ hiển thị.

⇒ Kết luận: công thức + xác suất là **server-side**. Muốn có thì hoặc (a) có server KR, hoặc (b) tự định
nghĩa lại cho bản rewrite.

## 3. Tỉ lệ rơi item — **KHÔNG có trong client** [VERIFIED phủ định]

Cùng lý do §2: không có bảng rate/drop nào trong pack; `ClientString` không có chuỗi 확률/drop.
(`PetRandomTable` là random **kỹ năng pet**, không phải drop.)

## 4. Thông số physics

| Loại | Ở đâu | Trạng thái |
|---|---|---|
| Thuộc tính tile (va chạm, loại ô) | `TileInfoList` | ✅ có dữ liệu, **cần decode 72 byte** (mới biết cấu trúc ngoài) |
| Hằng số chuyển động (tốc độ, gia tốc, trượt) | trong code | ❌ chưa lấy; đã có bản VN chạy được trong `boom-re/server` |
| Bomb: thời gian nổ, bán kính, lan | trong code | ❌ chưa lấy; VN đã có hành vi quan sát |
| Script bomb theo thời gian | `<mapscript>` (770 map) | ✅ đọc được đầy đủ (`time ms`, `power`, `targetcol/row`) |

## 5. Kết luận & đề xuất cho bản rewrite

1. **Recipe Magic Market**: tự thiết kế (A + B → C, kèm xác suất và item bảo vệ) — dữ liệu gốc không có.
2. **Tỉ lệ rơi**: tự thiết kế theo cảm giác chơi; có thể tham chiếu tỉ lệ trong `boom-re` (VN đã tune).
3. **Physics**: lấy từ bản VN đang chạy (`boom-re/server`) rồi tinh chỉnh; hằng số gốc của KR phải RE
   bằng debugger — mà debugger đang bị anti-tamper chặn (xem `docs/kr.md` §29).
4. **Dữ liệu dùng được ngay**: `ObjectTable` (8.185 object + animation), `TileInfoList` (5.929 tile),
   `ClientString` (6.993 chuỗi, cần decode cp949), 770 map script.
