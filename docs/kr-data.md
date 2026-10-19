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

## 6. ĐÍNH CHÍNH quan trọng: bảng RƠI **CÓ** trong pack (MonInfo)

Sau khi quét cả entry **mã hoá** (lần đầu tôi bỏ qua `key != 0`), tìm thấy trong
`FxMap/MapScriptTemplate/MonInfo_*`:

```xml
<InitInfo ... dierewarditemprob="40" dierewardlucciprob="30" dierewardidx="1" ... />
...
<MonDieReward>
  <MonLevel value="1">
    <Difficulty value="0">
      <RewardLucci>
        <Lucci type="LucciBronze" weight="10"/>
        <Lucci type="LucciSilver" weight="0"/>
        ...
      </RewardLucci>
      <RewardItem itemidx="10"/>
    </Difficulty>
    <Difficulty value="1"> ... <RewardItem itemidx="11"/> ... </Difficulty>
  </MonLevel>
  <MonLevel value="2"> ... </MonLevel>
</MonDieReward>
```

### Số liệu đo được

| Chỉ số | Giá trị |
|---|---|
| File `MonInfo_*` | **45** (20 file có `<MonDieReward>`) |
| Bản ghi reward (MonLevel × Difficulty) | **760** |
| `<RewardItem itemidx>` | **760**, 38 index khác nhau, dải **10..113** |
| `dierewarditemprob` | 20 (19 file), 40 (4), 100 (1), 0 (1) |
| Lucci weight | `LucciBronze=10` phổ biến nhất; `LucciSilver` 10 hoặc 2; `LucciGold` 0 hoặc 10 |

⇒ **Tỉ lệ rơi và danh sách item rơi nằm trong client pack**, không phải server.
Đã trích ra: `data/kr/monster_drop.json` (760 bản ghi, kèm lucci weights).

### Physics của monster cũng nằm trong pack

`<InitInfo collisionAttr="user=1,bomb=1,flames=1" collisionBase="6,10" collisionSize="111x68"
firePos="left=-3,top=-2,right=3,bottom=2" passType="box=1,static=1,bomb=1"
movablearea="left=-5,top=-10,right=10,bottom=10" life="Max=2000,Anger=60,gageBar=1"
defence="25" exp="100" damage="100"/>`

Đã trích: `data/kr/monster_physics.json` (45 bản ghi).

## 7. Còn thiếu (đúng như kết luận cũ)

| Hạng mục | Trạng thái |
|---|---|
| Tỉ lệ rơi + item rơi | ✅ **có** (MonInfo, đã trích JSON) |
| Physics monster (collision, pass, movable area, fire offset) | ✅ **có** (MonInfo) |
| Physics nhân vật/bom (tốc độ, thời gian nổ, bán kính) | ❌ trong code; VN đã có hành vi trong `boom-re/server` |
| **Công thức Magic Market (A+B→C + xác suất)** | ❌ **không có trong client** — server-side |

## 8. Ghi chú: đã thử tìm trên mạng

| Nguồn | Kết quả |
|---|---|
| DuckDuckGo HTML | trang "anomaly" (chặn bot) |
| Bing | trả trang nhưng **"결과가 없음"** với mọi truy vấn (kể cả `크레이지아케이드`) |
| Mojeek | HTTP 403 |
| namu.wiki | Cloudflare "Just a moment" |
| ko.wikipedia | **0 kết quả** cho `크레이지 아케이드` (không có bài) |
| Google | trang consent, không có kết quả |

⇒ Không lấy được thông tin ngoài. Theo chỉ đạo: dùng dữ liệu VN + client KR; phần không có thì tự thiết kế.

## 9. Copy cấu hình VN sang dùng tạm (`config/gameplay.json`)

Chủ dự án chỉ ra: VN đã chạy đúng phần này thì lấy luôn, không cần RE lại. Đúng — nhưng cần phân biệt
**server giữ gì** và **client giữ gì**:

| Tham số | VN giữ ở đâu | Giá trị | Dùng cho KR |
|---|---|---|---|
| `round_time` | server (env `ROUND_TIME`) | 120 s | ✅ copy |
| `bomb_fuse` | server (env `BOMB_FUSE`) | 2.5 s | ✅ copy (giá trị gốc chưa biết, VN đã dùng ổn) |
| `match_end` | server | 3.0 s | ✅ copy |
| `turn_mode`, `start_op`, `begin_op`, `room_type` | server | 4, 117, 191, 19 | ✅ copy |
| Tile px / grid stride | client (RE ở `docs/map.md`) | 40 px / 101 | ✅ copy |
| Trọng số item rơi | client (placement trong map) | Shoe/Shark 100, MaxPower 10, Fire/Bomb/Roller 30 (%) | ✅ copy |
| **Tốc độ nhân vật** | **client** | ❌ VN server chỉ relay vị trí client gửi | ⚠️ phải tự chọn + tune |
| **Bán kính/lan của bom theo power** | **client** | ❌ power do client gửi, server không mô phỏng | ⚠️ phải tự chọn + tune |
| Hiệu ứng từng item kind | client | ❌ (`docs/oc_e-game.md` ghi "CHƯA KIỂM") | ⚠️ tune |
| Monster physics + reward | **pack KR** (MonInfo) | ✅ 45 + 760 bản ghi | ✅ dùng luôn |

⇒ Đã gom tất cả vào **`config/gameplay.json`** (JSON hợp lệ, có `_provenance` ghi nguồn từng nhóm).
Chỗ nào chưa biết để `null` + `note` để lúc code không đoán bừa.

### Vì sao "thiếu" dù VN chạy đúng

VN **server** không mô phỏng chuyển động/bom — nó nhận vị trí client gửi và hẹn giờ nổ. Vì vậy trong
`boom-re/server` không có hằng số tốc độ/bán kính để copy; những số đó nằm trong **client VN** và chưa
được trích ra thành bảng. Với bản rewrite (tự viết client) thì ta **buộc phải chọn số** — nên để ở
`config/gameplay.json` và tune dần, thay vì cố RE từ client KR (đang bị anti-tamper chặn).

## 10. Đo từ client VN: fuse đã đo được, tốc độ bị chặn bởi môi trường

### Đo được: thời gian nổ bom

Từ log server VN đã lưu (`boom-re/logs/xvfb/rust_boom/fakeserver.log`):

```
174.499 [6868] C->S opcode 111   (đặt bom)
174.499 [6868] S->C opcode 152   (gán id)
176.999 [6868] S->C opcode 151   (nổ)
```

⇒ **2.500 s** đúng bằng `BOMB_FUSE`, khớp `config/gameplay.json`.

### Chưa đo được: tốc độ chạy + bán kính nổ

Hai lý do:

1. **Di chuyển không đi qua TCP**: client gửi vị trí bằng engine UDP riêng
   (`MTUDPBase/CUDPPeer`, xem `boom-re/docs/oc_e-game.md` §12). Log TCP của server **không có**
   opcode 141 — đã kiểm 2 log lưu: `141=0`.
2. **Client VN hiện không qua được bước patch**: chạy lại (cả `fakeserver` và server Rust) đều dừng ở
   `116 : InitBeforePatch` rồi thoát. Log của lần chạy thành công trước đây (`logs/spawn1/CALog.log`)
   đi tiếp `[XRpt] Sg - INT(dne:1)...` ⇒ khác biệt nằm ở **bản patch/snapshot đang dùng**, không phải server.

### Đường còn lại để lấy tốc độ (khi cần)

1. Khôi phục client VN chạy được (dựng lại `ca_fixed_patched.exe` từ `unpacked_ca.exe` hiện có, hoặc
   dùng lại quy trình snapshot trong `boom-re/docs/devirt.md`), rồi **đọc bộ nhớ** toạ độ nhân vật theo
   thời gian (VN không bị anti-tamper như KR) ⇒ px/s ⇒ ô/s.
2. Hoặc RE tĩnh: tìm hằng số tốc độ trong `unpacked_ca.exe` (hàm queue di chuyển `sub_6ED146`,
   seed `sub_654153` đã có địa chỉ trong `docs/oc_e-game.md` §5h).

Cả hai đều chưa làm trong đêm nay; `config/gameplay.json` để `null` cho tới khi có số.

## 11. RE tĩnh client VN để tìm tốc độ — tiến độ

Dùng IDA MCP trên `boom-re/unpacked_ca.exe` (có sẵn `.i64`).

| Địa chỉ | Vai trò (đã xác nhận bằng decompile) |
|---|---|
| `0x71B521` | tick gửi gói: đọc stamp `+19676`, so `+19688`, ngưỡng `0x7D0` (2000 ms), `sub_654153(3) % 12 == 0` → `sub_70E9FC` (gửi 141) |
| `0x708CC8` | **input dispatch**: nhận `(a2, a3=phím, a4=slot, a5=down)`; với mỗi hướng gọi `sub_6FAF6B / sub_6FB00B / sub_6FB0AB / sub_6FB12D / sub_6FB91F`; nhánh còn lại dispatch qua `vtable+32` của strategy |
| `0x6FAF6B`, `0x6FB00B`, `0x6FB0AB`, `0x6FB12D`, `0x6FB91F` | handler theo hướng/khe (bomber hiện tại) |
| `0x6ED146` | tính vùng ảnh hưởng (không phải di chuyển) |
| `0x6EE7A1` | duyệt ô theo hàng (line-of-sight / vùng) |
| `0x654153` | PRNG `rand()` bọc theo seed (`dword_E643E4`) |

**Chưa tìm ra**: hằng số thời gian mỗi bước đi (ms/ô) hoặc trường speed của bomber.
Hướng tiếp: đi từ `0x708CC8` → nhánh strategy `vtbl+32`, hoặc tìm nơi ghi "số bước còn lại" của queue
(`this+a2+95` là counter đã thấy ở `sub_6FAF6B`).

`config/gameplay.json` vẫn để `null` cho `speed`/`radius`.

## 12. Tìm trong boom-re: những thông số DÙNG ĐƯỢC (đã có sẵn)

Rà toàn bộ `boom-re` (main + 7 branch + worktree agent `~/boom-re-oc/*`) — kết quả:

| Thông số | Có ở đâu | Giá trị | Dùng được? |
|---|---|---|---|
| Fuse bom | log server VN | **2.500 s** (§10) | ✅ |
| **Tầm nổ** | `boom-re/docs/oc_e-game.md` §5a-5d | field bomber `vf31`, mặc định **1**; mỗi `ItemFire` **+1** | ✅ (bán kính = tầm nổ, đơn vị ô) |
| Số bom | cùng chỗ | `vf30`, +1 mỗi `ItemBomb` | ✅ |
| **Trọng số rơi item** | `fx/Attr/ItemAttr` (bảng thật, `docs/items.md`) | Bomb 3650, Fire 2800, Roller 2000, Punch 300, Hollow/Skeleton 200, Shoe/MaxPower 150 | ✅ |
| Trọng số theo map | `docs/map.md` | Shoe/Shark 100, MaxPower 10, Fire/Bomb/Roller 30 (%) | ✅ |
| Tile px / stride | `docs/map.md` | 40 px / 101 | ✅ |
| **Tốc độ chạy** | **`fx/Attr/BomberAttr`** (mới tìm) | bảng 13 nhân vật, mỗi entry ~12 u32 — **chưa parse đúng layout** | ⏳ cần 1 vòng nữa |

### Bảng `Attr/` vừa tìm thấy (đã trích ra `data/kr/`)

| File | VN | KR |
|---|---|---|
| `Attr/BomberAttr` | 2.501 B (13 entry) | 4.087 B |
| `Attr/ItemAttr` | 20.140 B | 25.597 B |
| `Attr/MapGroup` | 663 B | 663 B (giống nhau) |

`BomberAttr` bắt đầu bằng `Character/C00`, `Character/C01`, `Character/C03`, `Character/C05`… mỗi entry là
một dãy số nguyên (ví dụ C01: `1, 0, 4, 8, 2, 7, 1, 9, 10, 100, 9, 13`) — gần như chắc chắn chứa **tốc độ,
số bom, tầm nổ, HP**. Layout chưa khớp (parse thử 11/12/13 giá trị đều lệch offset) nên **chưa kết luận**
trường nào là tốc độ.

⇒ Đã copy 6 file nhị phân vào `data/kr/*.bin` để vòng sau parse bằng script đối chiếu VN↔KR (bảng VN nhỏ hơn
nên dễ suy ra layout).

`config/gameplay.json`: `bomb.radius_cells_per_power` **đã có cơ sở** (= tầm nổ, +1 mỗi Fire) — sẽ cập nhật
khi chốt; `movement.speed_cells_per_s` vẫn chờ parse `BomberAttr`.

## 13. Giải xong `BomberAttr`: layout + ánh xạ field → struct

### Layout (đã kiểm khớp tuyệt đối)

Không có trường count. Mỗi entry: `u32 nameLen, name[nameLen], 11 × u32` = **61 byte** với tên 13 ký tự.

| File | Kích thước | Số entry | Kiểm |
|---|---|---|---|
| VN | 2.501 | **41** | 61 × 41 = 2.501 ✅ |
| KR | 4.087 | **67** | 61 × 67 = 4.087 ✅ |

Dữ liệu: `data/kr/bomber_attr.json` (`{vn:[...], kr:[...]}`).

### Ánh xạ trong client (`sub_97124A`, xref tới chuỗi `Attr/BomberAttr`)

| field | nhận vào | ghi tại | ghi chú từ code |
|---|---|---|---|
| 0 | chỉ số nhân vật | `this + 103*idx` | chọn slot |
| 1 | — | `+116` | `field[1]-1` quyết định **class/kiểu nhân vật** (0 = mặc định) |
| 2 | — | `+117` | |
| 3 | — | `+120` | |
| 4 | — | `+123` | |
| 5 | — | `+126` | **bị chặn ≤ 10** (`if >10 → 10`) |
| 6 | — | `+129` | |
| 7 | — | `+132` | |
| 8 | — | `+135` | gán trực tiếp (không qua setter) |
| 9 | — | `+136` | gán trực tiếp |
| 10 | — | `+137` | |

### Giá trị (VN = KR ở 41 entry đầu, KR có thêm 26 nhân vật)

```
C00 [0, 0, 5, 7, 1, 10, 1, 7, 10, 100, 9]
C01 [1, 0, 4, 8, 2, 7, 1, 9, 10, 100, 9]
C03 [2, 0, 5, 8, 1, 8, 1, 5, 10, 100, 7]
C05 [3, 0, 4, 8, 1, 10, 1, 8, 10, 100, 7]
C04 [4, 0, 4, 8, 2, 9, 1, 6, 10, 100, 7]
```

- field 9 = **100** ở mọi nhân vật; field 8 = 10; field 5 ∈ 7..10 (bị chặn 10).
- field 4 ∈ {1,2} — trùng với "tầm nổ 1 → 2" mà `docs/oc_e-game.md` mô tả ⇒ **nghi field 4 = tầm nổ ban đầu**.

### Còn lại để chốt

Cần đối chiếu 1 trong 2 cách:
1. Tìm getter `vf30/vf31` của bomber và xem chúng đọc offset nào (`+123` = field 4, `+126` = field 5, …);
2. Hoặc so sánh với hành vi: nhân vật C01 có field 4 = 2 → nếu vào game C01 nổ xa hơn C00 (field 4 = 1) thì xác nhận.

`config/gameplay.json` vẫn giữ `null` cho tốc độ; `bomb.radius_cells_per_power` sẽ lấy theo field tầm nổ khi chốt.
