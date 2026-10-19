# Nexon KR — trang chính thức (chưa cào được, cần mở tay)

Tool không truy cập được vì `ca.nexon.com` phân giải ra IP nội bộ (bị chặn ở tầng DNS).
Mở bằng trình duyệt bình thường rồi copy nội dung cần dùng vào đây hoặc vào `raw/`.

## Danh sách cần lấy

| Trang | URL | Nội dung dự kiến | Giá trị cho engine |
|---|---|---|---|
| Nhân vật & chỉ số | https://ca.nexon.com/Info/Character/1 | bảng nhân vật chính thức: số bóng / độ dài / tốc độ (min–max) | đối chiếu `Attr/BomberAttr` (đã khớp 8/8 với ja.wikipedia) |
| Guide Object | https://ca.nexon.com/CACreators/Guide/Object/2 | hướng dẫn vật thể/mode do Nexon viết | xác nhận luật chính thức, khác wiki cộng đồng |
| 크아 잘하는 방법 | https://ca.nexon.com/Free/Content/416902 | bài hướng dẫn chơi | chiến thuật, có thể có mốc thời gian |
| 도감 vật phẩm | https://ca.nexon.com/Info/ItemGachapon/99/ | danh mục vật phẩm/gacha | tên + phân loại item |

## Đã có nguồn thay thế (dùng tạm)

- Chỉ số nhân vật: `docs/game-spec-web.md` §1 (ja.wikipedia, khớp 8/8 với `data/kr/bomber_attr.json`).
- Luật cơ bản + item: `docs/web/namu-digest.md` (namu, build KR cuối).
- Trọng số rơi item: `data/kr/*_ItemAttr.bin` (dữ liệu client thật, tốt hơn web).

## Khi lấy được, ghi theo mẫu

```
Nguồn: <URL>
Tiêu đề: <tiêu đề trang>
Ngày truy cập: <YYYY-MM-DD>
Kết luận được hỗ trợ: <...>
Giới hạn: <...>
```
