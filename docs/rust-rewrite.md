# Rewrite engine Boom KR bằng Rust — kế hoạch

> Bối cảnh: source engine cũ và asset gốc **đã thất lạc**. Toàn bộ hiểu biết phải dựng lại từ
> RE client (repo `boom-re` cho VN, `boomkr-re` cho KR) + asset decode từ pack trong client.

## 1. Vì sao hướng này đúng

Nếu **tự viết cả client lẫn server** thì không cần protocol gốc của Nexon, cũng không cần vượt
`ProcessPatch`/Themida — hai thứ đang chặn RE client KR. Asset thì đã mở 100% (13/13 pack).

## 2. Cái đã có (dùng lại được)

| Thành phần | Trạng thái | Nơi |
|---|---|---|
| Đọc pack `.idx/.idd` (khoá = 4 byte đầu `.idx`) | ✅ **Rust, byte-identical với Python** | `rust/krpack` |
| Format sprite/ảnh | ✅ có `tools/i16.py` | port sang Rust |
| Map/tile | ✅ cùng họ container | port sang Rust |
| Âm thanh | ✅ Ogg Vorbis | dùng trực tiếp |
| Server Rust (lobby/phòng/map/di chuyển) | ✅ có sẵn | `boom-re/server` |
| Luật chơi đã RE (item, map, bomb một phần) | 🟡 | `boom-re/docs/items.md`, `map.md` |

## 3. Cái **phải** RE thêm từ client (không có source)

| Hệ thống | Cần biết gì | Lấy từ đâu |
|---|---|---|
| Chuyển động nhân vật | tốc độ, gia tốc, 4 hướng, trượt băng (bubble) | trace toạ độ theo tick ở client VN + `server` hiện có |
| Bomb | thời gian nổ, lan theo ô, sức mạnh, chain | `docs/items.md` + quan sát |
| Item | danh sách, hiệu ứng, tỉ lệ rơi | pack `FxContents`/`FxTile` + docs |
| Map | ý nghĩa tile, va chạm, spawn | decode map + so với hành vi |
| Animation | thứ tự frame, thời lượng | sprite container (i16) + timing quan sát |
| UI/lobby | luồng màn hình, danh sách phòng | đã RE phần lớn ở VN (`docs/lobby.md`) |

## 4. Kiến trúc

```
rust/
├── krpack      # ✅ đọc .idx/.idd (đã xong, byte-identical)
├── assets      # sprite/map/ogg loader trên krpack
├── protocol    # protocol MỚI (của mình), không phải của Nexon
├── server      # tokio: lobby, room, match, physics
└── client      # wgpu/macroquad: render, input, netcode
```

## 5. Mốc

| # | Mốc | Tiêu chí xong |
|---|---|---|
| 1 | `krpack` | ✅ 13/13 pack, 51.467 entry, extract byte-identical |
| 2 | `assets` | load + vẽ 1 sprite, 1 map, phát 1 ogg; CLI dump PNG |
| 3 | `protocol` + `server` | 2 client vào 1 phòng, di chuyển đồng bộ |
| 4 | bomb + item + va chạm | nổ lan đúng, item nhặt được, thắng/thua |
| 5 | đóng gói | build Windows, chạy offline không cần server Nexon |

## 6. Ghi chú kỹ thuật

- `krpack` **không dùng crate ngoài** ⇒ build offline được.
- Entry có `key != 0`: giải mã bằng XOR-chain + CRC32 kiểm tra (đã verify).
- Entry `_mm`: zlib — **chưa làm** trong Rust (cần inflate; TODO mốc 2).
- Tên file trong pack: cp949; phần lớn là ASCII nên hiện đọc lossy, mốc 2 sẽ decode chuẩn.

## 7. Rủi ro

| Rủi ro | Giảm thiểu |
|---|---|
| Asset KR thuộc Nexon, không thuộc công ty | dùng asset VN (của công ty) cho build phát hành; KR chỉ để đối chiếu |
| Luật chơi không đoán hết từ quan sát | ưu tiên cảm giác chơi "đủ tốt", không cần giống 100% |
| Pack format có thể khác ở bản VN | `krpack` đọc được cả VN (cùng format) — kiểm tra ở mốc 2 |
