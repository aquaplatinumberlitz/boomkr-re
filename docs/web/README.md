# Nguồn web tham khảo (luật chơi + thông số)

Thư mục này lưu **ghi chú có nguồn** từ web để dùng khi viết lại engine Rust. Nó không phải spec
chuẩn của engine; spec/evidence vẫn nằm ở `docs/` theo quy ước dự án.

## Nguồn và trạng thái truy cập (2026-10-19)

| Nguồn | URL gốc | Truy cập tự động | Ghi chú |
|---|---|---|---|
| namu.wiki (Hàn) | `https://namu.wiki/w/크레이지 아케이드/...` | ❌ Cloudflare 403 | **mirror `namu.moe` trả 200** → dùng mirror để cào |
| Nexon KR chính thức | `https://ca.nexon.com/...` | ❌ DNS trả IP nội bộ | phải mở tay |
| baike.so.com / bnb.web.sdo.com (TQ) | — | ❌ DNS nội bộ | phải mở tay |
| Wikipedia (en/ja/zh/vi/ru) | `https://*.wikipedia.org` | ✅ | xem `docs/game-spec-web.md` |

## Cấu trúc

- `raw/` — **bản cào đầy đủ** từ namu.moe (8 trang), **đã gitignore** (`docs/web/raw/`) vì nội dung
  namu là CC BY-NC-SA 2.0 KR (cấm dùng thương mại) và repo `boomkr-re` là public.
  Dùng nội bộ để tra cứu; không commit, không phát hành lại.
- `namu-digest.md` — bản trích lược có cấu trúc (được commit): danh mục item, mode, monster, EXP, luật.
- `nexon-official.md` — 3 trang Nexon bị chặn: nội dung dự kiến + link mở tay.

## Cào lại (khi cần cập nhật)

```sh
python3 tools/namu2md.py "https://namu.moe/w/<tên trang>" docs/web/raw/<slug>.md
```

`tools/namu2md.py` chỉ lấy phần `<article>`, bỏ khung điều hướng/TOC/footer, giữ markup wiki
(`||` bảng, `==` tiêu đề) và chèn header nguồn/ngày/license.

## Giới hạn (phải ghi khi trích dẫn)

- Nội dung namu là **wiki cộng đồng**, không phải tài liệu phát hành của Nexon → mức tin cậy
  thấp hơn nguồn chính thức; dùng để **đối chiếu**, không dùng làm oracle duy nhất.
- Namu phản ánh **build KR cuối** (server đóng 2026-08-13); VN có thể khác.
- Các con số "giây" cho **thời gian bị nhốt** và **quy đổi tốc độ → ô/giây** vẫn **không có** ở mọi nguồn
  đã đọc; xem `docs/game-spec-web.md` §8.
