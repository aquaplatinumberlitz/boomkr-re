# Rewrite engine Boom KR bằng Rust: dùng chung engine với VN

> Cập nhật 2026-10-19. Kế hoạch gốc của engine nằm ở `boom-re/docs/REWRITE_PLAN.md`
> (Fidelity Spec, Feel Oracle, kiến trúc crate, lộ trình R0–R7 và W1–W4).
> Tài liệu này chỉ ghi **phần riêng của KR**. Tag bằng chứng theo `boom-re/docs/MASTER_PLAN.md` §7.

## 1. Kết luận

**Áp dụng được, và nên dùng một engine duy nhất cho cả VN lẫn KR.** KR là dòng code cùng engine,
bản mở rộng mới hơn:
- entry đầu của `charte` (VN) và `CharTE` (KR) giống tuyệt đối, kể cả offset và size **[VERIFIED]** (`docs/vn-vs-kr.md` §2);
- cùng định dạng pack, sprite, map, âm thanh **[VERIFIED]** (`docs/kr.md` §16).

Mô hình: **một engine Rust, nhiều "content profile"**.
- `vn`: 4 pack, bộ chế độ của VN.
- `kr`: 13 pack, thêm nội dung và chế độ mới.

Không fork engine. Phần riêng của KR chỉ gồm: danh sách pack, dữ liệu nội dung, và các chế độ chỉ có ở KR.

## 2. Dùng lại từ `boom-re`, theo từng lớp

| Lớp | Dùng lại | Ghi chú |
|---|---|---|
| Đọc pack `.idx/.idd` | ✅ 100% | Khoá = 4 byte đầu `.idx` (`docs/kr.md` §16). Áp dụng luôn cho VN, bỏ được bảng `KEYS` cứng |
| Sprite `CDataI16Alpha`, map `.map`, Ogg | ✅ 100% | `boom-re/tools/i16.py`, `boom-re/docs/map.md` |
| Luật chơi lõi (di chuyển, bom, nổ, item, bẫy) | ✅ | Cùng engine. **Hằng số đo trên client VN** (§3) |
| Kiến trúc Rust (`boom-core`, `boom-render`, `boom-audio`, `boom-ui`, `boom-server`…) | ✅ 100% | `boom-re/docs/REWRITE_PLAN.md` §5 |
| Giao thức | — | Rewrite dùng giao thức v2 tự thiết kế; protocol KR của Nexon **không cần** |
| Themida / VM / anti-cheat KR | ❌ không cần | Rewrite không chạy `CA.exe` KR. Vượt `ProcessPatch` không còn là điều kiện bắt buộc |

## 3. Đo "cảm giác chơi" cho KR lấy từ đâu

Client KR **không vào được tới gameplay**: dừng ở `ProcessPatch`, và anti-tamper chặn mọi instrumentation
(`docs/kr.md` §24, §27, §29). Vì vậy **Feel Oracle của KR là client VN**. Client VN đã chạy trọn trận
dưới Wine + gdb (`boom-re/docs/progress.md`).

Tra theo tên class trong `boom-re/decomp/ida_names.json` và chuỗi trong `boom-re/ca_strings.txt`
**[VERIFIED]** (2026-10-19, `grep -ci`):

| Chế độ KR (`docs/vn-vs-kr.md` §3) | Có code trong client VN? | Nguồn đo |
|---|---|---|
| Battle thường, item, map | ✅ | Oracle VN (`REWRITE_PLAN.md` §4) |
| HiddenCatch | ✅ (749 tên, `CHiddenCatchGameStage`) | Oracle VN + asset `FxHiddenCatch` |
| AIBattle | ✅ (178 tên) | Oracle VN, asset `FxContents/AIBattle` |
| Airplane | ✅ (12 tên) | Oracle VN, asset `FxMap/Bonus/Airplane*` |
| SuperStar | ✅ (15 tên) | Oracle VN |
| Monster | ✅ (164 tên) | Oracle VN + asset `FxMonster` |
| **Alchemist** | ❌ (0) | Không có oracle: **[ARCHIVE]** (video, wiki KR) + **[DESIGN]** |
| **Baseball** | ❌ (0) | Không có oracle: **[ARCHIVE]** + **[DESIGN]** |

Lưu ý: chế độ có tên trong VN chưa chắc có đủ asset và map VN để chạy được. Cách xử lý: chạy client VN
với asset VN nếu đủ; nếu thiếu thì chỉ đọc tĩnh luật trong IDA (bản VN không bị chặn như KR).
Bản KR có thể đã chỉnh luật so với VN. Chênh lệch nào phát hiện qua [ARCHIVE] thì ghi lại và gắn tag.

## 4. Thay đổi cần làm trong engine chung

| ID | Việc | Ở đâu |
|---|---|---|
| KR-R0-1 | `boom-assets` đọc **13 pack** bằng khoá tự thân; port từ `rust/krpack` (đã byte-identical với Python) | `boom-re` → `crates/boom-assets` |
| KR-R0-2 | Thêm inflate zlib cho entry `_mm` (TODO của `krpack`), decode tên cp949 đúng chuẩn | như trên |
| KR-R0-3 | Bảng **content profile** (`vn`, `kr`): danh sách pack, map, chế độ bật | `crates/boom-assets` + config |
| KR-R0-4 | `boom-convert --profile kr`: xuất 51.467 entry KR sang PNG/OGG/RON | như trên |
| KR-W3-1 | Chế độ đã có trong VN (HiddenCatch, AIBattle, Airplane, SuperStar, Monster): làm ở đợt W3 với oracle VN | `REWRITE_PLAN.md` §9.5 |
| KR-W3-2 | Alchemist, Baseball: viết spec từ [ARCHIVE], designer duyệt [DESIGN] | `docs/spec/features/` |
| KR-UI | UI mới của KR (`FxDesktop`, `FxContents`): dựng bằng `boom-ui`; golden ảnh lấy từ video/ảnh chụp [ARCHIVE], vì không chạy được client KR | `crates/boom-ui` |

Chi phí cộng thêm cho KR so với chỉ VN (ước lượng thô, đội 3–5 người):
- asset/profile: **2–3 tuần**;
- UI KR mới: **1–2 tháng**;
- Alchemist và Baseball: **1–2 tháng**.

Các chế độ còn lại nằm trong W3 đã có.

## 5. Vai trò của repo này sau quyết định

- Giữ: RE build KR (`docs/kr.md`), manifest, so sánh VN–KR, danh mục nội dung KR.
- **Code engine không nằm ở đây.** Theo `SYNC.md` ("không copy code"), mọi crate engine nằm trong `boom-re`.
  `rust/krpack` sẽ được gộp vào `boom-assets`, rồi giữ lại ở đây làm tool tham chiếu hoặc xoá.
- Việc vượt `ProcessPatch`/GameGuard (`docs/kr.md` §25, §33): **hạ ưu tiên**. Chỉ làm nếu cần đo một luật mà KR đã đổi so với VN.

## 6. Rủi ro

| Rủi ro | Giảm thiểu |
|---|---|
| Asset KR thuộc Nexon, chưa chắc thuộc quyền công ty | **Cần xác nhận bằng văn bản** trước khi phát hành build dùng asset KR. Trong lúc chờ: build phát hành dùng profile `vn`, profile `kr` chỉ dùng nội bộ |
| KR đã chỉnh luật so với VN; oracle VN đo sai cho KR | Đối chiếu [ARCHIVE] (video KR) ở các tình huống chuẩn; ghi chênh lệch thành tham số riêng của profile `kr` |
| Alchemist/Baseball không có nguồn đo | Spec từ video + designer; không tính vào chuẩn "~100%" của Fidelity Spec |
| UI KR không có golden ảnh từ client chạy được | Ảnh/video [ARCHIVE]; nếu sau này vượt được `ProcessPatch` thì chụp lại |
