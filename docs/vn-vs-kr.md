# So sánh VN vs KR — cái gì dùng lại được, cái gì mới

Số liệu đo bằng `rust/krpack` (cùng một code đọc cả hai bản) và script đối chiếu tên entry.

## 1. Pack & nội dung

| | VN (`boom-re/wine/Boom`) | KR (`boom-re/boomKR`) |
|---|---|---|
| Số pack | 4 | **13** |
| Số entry | 22.612 | **51.467** |
| Pack | charte, emote, fxasset, fx | CharTE, EmoTE, FxAsset, fx, **FxCharacter, FxContents, FxDesktop, FxHiddenCatch, FxMap, FxMonster, FxSound, FxStage, FxTile** |
| idd lớn nhất | fx 408 MB | CharTE 369 MB, EmoTE 275 MB |

## 2. Trùng lặp nội dung (theo tên entry)

| cặp pack | VN | KR | chung | cùng kích thước | KR mới |
|---|---|---|---|---|---|
| `charte` ↔ `CharTE` | 3.695 | 20.744 | 3.640 | 3.615 | **17.104** |
| `emote` ↔ `EmoTE` | 5.459 | 10.040 | 3.696 | 3.684 | **6.344** |
| `fx` ↔ `fx` | 13.447 | 4.093 | 1.505 | 1.356 | **2.588** |

- 9 pack mới của KR: **16.579 entry** (toàn bộ là mới).
- Tổng KR-only ≈ **42.615 entry** (83% nội dung KR không có ở VN).
- VN-only ≈ 13.760 entry (chủ yếu trong `fx`).

**Bằng chứng mạnh nhất về "cùng dòng code":** entry đầu của `charte` (VN) và `CharTE` (KR) **giống nhau
tuyệt đối**, kể cả offset và size:

```
AvatarDeco/A02_10_0001_0LayerAdjust   off=0x00000000 size=0xd83
AvatarDeco/A02_10_0001_0LayerStatic   off=0x00000d83 size=0x5523
```

⇒ Cùng format, cùng nguồn asset, KR là bản mở rộng mới hơn.

## 3. KR có thêm gì (từ tên entry)

| Pack | Nội dung | Ví dụ |
|---|---|---|
| `FxStage` | định nghĩa **chặng/chế độ** | `Game/AIBattle_SuperStar`, `Game/AirplaneBigWave`, `Game/Alchemist_Start`, `Game/Baseball_Image` |
| `FxContents` | UI/nội dung theo chế độ | `AIBattle/Dlg_Result_*`, `AIBattle/Effect_Hurrah_*` |
| `FxHiddenCatch` | **chế độ Hidden Catch** | `HiddenCatch/Bt_Close`, `Bt_Map`, `Bt_Newstart` |
| `FxMap` / `FxTile` | map & tile (3.033 + 3.248 entry) | `Bonus/Airplane1`, `Tile00/BoxGray` |
| `FxMonster` | monster (1.403) | `Monster/AquaMon00_Appear` |
| `FxCharacter` | nhân vật/phụ kiện (3.571) | `Character/B02-AttackB_Down_ALayerStatic` |
| `FxDesktop` | UI desktop/lobby (2.077) | `AIaider/RedZone` |
| `FxSound` | âm thanh (797, Ogg) | `MP3/AliceRabbit` |

⇒ KR có ít nhất **5 nhóm chế độ mới** so với VN: AIBattle (đấu AI), Airplane (bắn máy bay), Alchemist,
Baseball, HiddenCatch.

## 4. Áp dụng RE của VN sang KR — theo từng lớp

| Lớp | Dùng lại được? | Ghi chú |
|---|---|---|
| Đọc pack `.idx/.idd` | ✅ **100%** | cùng khoá tự thân (4 byte đầu), cùng XOR-chain, cùng CRC |
| Sprite/alpha/RGB565 (`i16.py`) | ✅ **100%** | cùng container |
| Map/tile, âm thanh | ✅ **100%** | cùng định dạng |
| Luật chơi lõi (bom, item, di chuyển, va chạm) | ✅ **~80%** | cùng engine; KR thêm chế độ |
| Kiến trúc server Rust | ✅ **100%** | đã có ở `boom-re/server` |
| Protocol | ⚠️ 0% trực tiếp | KR chưa biết; **nhưng rewrite thì tự định nghĩa** |
| Themida / VM | ❌ **0%** | KR là đời khác: 0 marker, không bảng handler tĩnh, 102 site/26 head |
| Anti-cheat | ❌ 0% | VN HackShield (gỡ được) vs KR GameGuard kernel + launcher Nexon |

## 5. Chi phí (ước lượng)

| Hạng mục | VN-derived | KR-specific |
|---|---|---|
| Asset pipeline | 100% | 0% (chỉ thêm 9 pack) |
| Engine core (vật lý, bom, item) | ~80% | ~20% |
| Chế độ mới (AIBattle, HiddenCatch, Airplane, Alchemist, Baseball) | 0% | **mới hoàn toàn** |
| Server | ~90% | ~10% |
| Client render/UI | ~70% | ~30% (nhiều UI mới) |

**Kết luận:** đi theo hướng **rewrite** thì RE của VN dùng thẳng được cho phần lõi; KR chỉ cộng thêm
nội dung + 5 chế độ. Không cần đụng Themida/GameGuard của KR.
