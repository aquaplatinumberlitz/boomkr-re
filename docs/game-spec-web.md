# Tổng hợp luật chơi & thông số Crazy Arcade từ web (KR/CN/JP/EN/VN)

> Mục đích: bổ sung cho phần RE — những gì web nói được thì không phải RE lại.
> Mọi số liệu dưới đây đều ghi nguồn; chỗ không có nguồn thì ghi rõ **chưa có**.

## 0. Phương pháp & giới hạn truy cập

Đã thử truy cập (2026-10-19):

| Nhóm trang | Kết quả |
|---|---|
| `*.wikipedia.org` (en/ja/zh/vi/ru) | ✅ truy cập được — **nguồn chính của tài liệu này** |
| `ca.nexon.com` (guide chính thức KR), `namu.wiki`, `readonly.wiki`, `ja.namu.wiki` | ❌ DNS trả IP nội bộ → bị chặn |
| `baike.so.com`, `bnb.web.sdo.com` (DB vật phẩm chính thức TQ), `web.archive.org` | ❌ chặn cùng lý do |
| `*fandom*`, `wikiwand` | ❌ Cloudflare 403 |

⇒ Phần lớn wiki Hàn/Trung **không đọc trực tiếp được**; chỉ lấy được **đoạn trích từ kết quả tìm kiếm**
(ghi rõ ở §8), còn lại dựa trên Wikipedia 5 ngôn ngữ.

## 1. Nhân vật và 3 chỉ số (nguồn: ja.wikipedia + vi.wikipedia)

Mỗi nhân vật có 3 chỉ số, đều là **cấp số (level)**, không phải đơn vị thời gian/ô:

- **水風船の量 / Số bom / Count** — số bóng mang tối đa
- **水風船の長さ / Độ dài / Power** — tầm nổ (đơn vị ô)
- **速度 / Tốc độ / Speed** — tốc độ di chuyển

Bảng gốc đầy đủ (min–max) — [ja.wikipedia クレイジーアーケード](https://ja.wikipedia.org/wiki/%E3%82%AF%E3%83%AC%E3%82%A4%E3%82%B8%E3%83%BC%E3%82%A2%E3%83%BC%E3%82%B1%E3%83%BC%E3%83%89):

| Nhân vật | Tên VN (vi.wiki) | Đặc trưng | Số bom | Độ dài | Tốc độ |
|---|---|---|---|---|---|
| Bazzi (배찌) | Khò Khò | tốc độ | 1–6 | 1–7 | **5–9** |
| Dao (다오) | Nhanh Nhẩu | đều | 1–10 | 1–7 | 5–7 |
| Dizni (디지니) | Mật Ngọt | đều | 2–7 | 1–9 | 4–8 |
| Mos (모스) | Đầu Đinh | tốc độ | 1–8 | 1–5 | 5–8 |
| Uni (우니) | Bé Bỏng | độ dài | 1–6 | 2–7 | 5–8 |
| Ethi (에띠) | Mọt Sách | số bom | 1–10 | 1–8 | 4–8 |
| Marid (마리드) | Mắt Xanh | số bom | 2–9 | 1–6 | 4–8 |
| Kephi (케피) | Thịt Mỡ | độ dài | 1–9 | 2–8 | 4–8 |
| Sue (수) | Thiên Sứ | tốc độ | 2–9 | 1–7 | **6–10** |
| Hoou (후우) | Tí Điệu | số bom | 3–9 | 1–7 | 5–10 |
| Lei (레이) | — | tốc độ | 2–9 | 1–7 | 6–10 |
| Lucy (루시) | — | tốc độ | 2–9 | 1–7 | 6–10 |
| Santa / Lodumani | — | (random) | — chỉ số cao hơn bình thường — | | |

Bản nâng cấp (Luxury / Super) có bảng riêng trong cùng trang ja.wikipedia (ví dụ Luxury Dao 2–8 / 1–8 / 6–10,
Super Luxury Ethi 2–10 / 1–7 / 7–10).

**Đối chiếu với dữ liệu trong pack KR** (`data/kr/bomber_attr.json`): khớp **8/8** → xác nhận
`Attr/BomberAttr` field 2-3 = tốc độ, 4-5 = số bom, 6-7 = độ dài (xem `docs/kr-data.md` §23).

## 2. Luật cơ bản (en + ja + vi wikipedia)

- Bóng nước nổ theo **hàng dọc và hàng ngang**.
- Trúng nước → **bị nhốt trong bong bóng**, chưa chết.
  - Đồng đội chạm → **hồi sinh**.
  - Địch chạm → **chết**.
  - Không ai chạm trong một khoảng ngắn → **chết**.
  - Dùng **kim** (`ItemHPNeedle`) → **tự thoát**.
- Đá/ném bóng bằng **giày đá** (kick) và **găng tay** (glove): bóng **nổ sau 3 giây kể từ lúc ném**;
  không có "giữ ném"/"giữ đá" như Bomberman; **ném ngược được**.
- Thắng: loại hết đối thủ (hoặc theo luật từng mode).
- Phòng tối đa **8 người**; **Manner** chia đội theo màu cân bằng, **Free** tự do (1v7, 4v4…).

## 3. Chế độ chơi

Nguồn: [vi.wikipedia Crazy Arcade](https://vi.wikipedia.org/wiki/Crazy_Arcade), [en.wikipedia](https://en.wikipedia.org/wiki/Crazy_Arcade), [zh.wikipedia 爆爆王](https://zh.wikipedia.org/wiki/%E7%88%86%E7%88%86%E7%8E%8B).

| Mode | Luật / số liệu |
|---|---|
| **Cơ bản (BnB)** | nhốt & phá bóng; hết đối thủ thì thắng |
| **Hồi sinh (Respawn)** | chết **sống lại sau 3 giây**; thắng theo số mạng hạ được |
| **Hóa nhím (Hedgehog)** | bị sặc → thành nhím: phá bóng khi chạm, không bị nhốt, **không di chuyển được một lúc**, cứu được người trong bong bóng, diệt ma ở map nghĩa trang (rồi biến mất), không dùng được bom |
| **Săn thủ lĩnh (Boss hunt)** | mỗi đội 1 người mang cờ; thủ lĩnh chết → **cả đội bị sặc** |
| **Bom hẹn giờ (Time bomb)** | 1 người mang bom trên đầu, chạm người khác để truyền; hết giờ → nổ, người đó bị nhốt |
| **Zombie** | người phải sống **1 phút 30 giây**; bị lây thì có **4,5 giây** uống thuốc giải |
| **Cỏ 4 lá (Clover)** | ăn cỏ nhiều hơn thì thắng; ăn cỏ có xác suất nhận vàng / "Superman" |
| **Nông trại xanh (Green farm)** | ăn **20 điểm** thì thắng: trắng **+1**, vàng **+3**, đen **+5** |
| **Đua xe (Racing)** | hoàn thành **3 vòng** qua mốc 1→4 |
| **Kho báu (Treasure)** | chiếm ô cùng màu để tích điểm |
| **Quái vật xanh / Sâu** | hạ nhiều sâu hơn đối thủ; combo 5 gặp **sâu hoàng kim** |
| **Thỏ (Rabbit)** | hạ **20 con thỏ** đầu tiên thắng |
| **Đấu trường quái vật (Monster)** | 4 người 1 đội, diệt quái di động rồi diệt **boss** |
| **Super Fighter** | mode đối kháng riêng (vi.wikipedia) |
| **(KR-only) Alchemist = 마법 마켓** | chế đồ trong `FxContents/MagicMarket/*` — phân tích ở `docs/kr-content.md` |
| **(KR-only) Baseball** | map `Map/Baseball01..10`, đội Đỏ/Xanh tính điểm — `docs/kr-content.md` |

## 4. Bản đồ theo chủ đề (nguồn: vi.wikipedia)

| Chủ đề | Số map | Đặc điểm |
|---|---|---|
| Bánh ngọt | 1→5 | |
| Khu rừng | 0→21 | nhiều quỷ đỏ |
| Băng tuyết | 0→17 | xuất hiện **giày tốc độ tối đa** |
| Đại dương | 0→26 | mực ống |
| Thuyền chiến | 0→19 | |
| Ngôi làng | 0→17 | nhiều bụi rậm để ẩn nấp |
| Khoáng sản | 0→23 | **xe đường ray** |
| Kim tự tháp | 0→22 | hồn ma, mũi khoan, gai |
| Doanh trại | 0→15 | xe tăng, gai nhọn, bụi gai |
| Mưa bom | 0→15 | bom rơi từ hệ thống |
| Nhà máy | 0→23 | lỗ truyền bom, bẫy tia nước |
| Nghĩa trang | 0→20 | hồn ma |
| Sa mạc | 0→15 | tua trượt, túp lều |
| Biển cả | 1→9 | cá mập, sứa nổ bom, san hô |

## 5. Cơ chế đặc biệt trên map (số liệu)

### 5.1 Phương tiện / vật thể có tốc độ riêng
Đoạn trích từ [baike.so.com – 泡泡堂](https://baike.so.com/doc/5351493-5586951.html) (trang bị chặn, chỉ đọc được snippet):

| Vật thể | Tốc độ |
|---|---|
| Cú mèo | 5 |
| Rùa | 2 |
| Rùa cướp biển | 9 |
| Bạch tuộc | 2 |
| Bạch tuộc cướp biển | 9 |
| Đĩa bay (UFO) | 10 |
| Đĩa hỏng | 2 |
| Xe tăng | 5 |
| Xe goòng (có ray / không ray / có động cơ) | **10 / 2 / 9** |
| Xe đua (đầy xăng / cạn xăng) | **10 / 5** |

Ghi chú kèm snippet: "ngồi đĩa bay không nhặt được item nhưng đi xuyên vật cản".

### 5.2 Zombie (vi.wikipedia)
- Người sống sót: **90 giây**
- Bị lây: **4,5 giây** để uống thuốc giải

### 5.3 Hồi sinh: **3 giây** · Ném bóng bằng giày/găng: **3 giây** rồi nổ

## 6. Tiến trình & hệ thống (en.wikipedia)

- **720 cấp độ**, chia theo hình/ màu huy hiệu + số sọc.
- Có hệ thống **ladder** (xếp hạng) ưu tiên hơn level.
- **Kênh** chia theo nhóm level; một số map/vật phẩm yêu cầu level.
- Điểm kinh nghiệm: thắng > hòa > thua.
- Việt Nam: VinaGame, tên **Boom Online**, đóng 10/05/2017.
- Hàn/Tài: đóng **13/08/2026**; Trung Quốc vẫn chạy (đổi nhà phát hành 2023).

## 7. Đối chiếu nhanh: web ↔ dữ liệu trong pack KR

| Thông số | Web | Pack KR (`data/kr/…`) | Kết luận |
|---|---|---|---|
| Số bom / độ dài / tốc độ theo nhân vật | ja.wikipedia (min–max) | `bomber_attr.json` | ✅ khớp 8/8 |
| Trọng số rơi item | không có | `ItemAttr` (Bomb 3650, Fire 2800, Roller 2000, Punch 300, Shoe/MaxPower 150) | ✅ dùng pack |
| Fuse bom thường | ja: ném xong **3 s** nổ; VN server đang 2.5 s | — | ⚠️ cần chốt 3.0 vs 2.5 |
| Thời gian bị nhốt trước khi chết | không nêu số | — | ❌ chưa có |
| Quy đổi cấp tốc độ → ô/giây | không có | — | ❌ chưa có |
| Monster drop/reward | không có | `monster_drop.json` 760 bản ghi | ✅ dùng pack |
| Monster physics | không có | `monster_physics.json` 45 con | ✅ dùng pack |
| Map & script | danh sách theme + số map | 770 script XML trong `out/scripts/` | ✅ dùng pack |

## 8. Những gì **chưa** tìm được (cần RE hoặc tự thiết kế)

1. **Thời gian bị nhốt trong bong bóng** trước khi chết (giây).
2. **Quy đổi cấp tốc độ (4..10) → ô/giây** — chỉ có thứ hạng tương đối.
3. **Fuse chính xác** của bóng đặt thường (2.5 s theo VN server vs 3 s theo ja.wikipedia cho bóng ném).
4. Bảng **kinh nghiệm/Lucci** theo level (720 cấp) — web không có số.
5. Công thức **chế đồ Magic Market** (KR-only) — không có trên web, không có trong pack.
6. Tỉ lệ **rơi đồ theo map** ngoài `prob%` đã đọc từ file map.

> Cách lấp 1–3: đo trong client (đã có hạ tầng: prefix `.wine-vn`, `tools/duo_measure.sh`) hoặc RE tĩnh
> hàm cập nhật di chuyển (`0x708CC8` → strategy `vtbl+32`).
