# SYNC — quan hệ với `boom-re`

Project này **không** chứa bản sao code của `boom-re`. Mọi tool dùng lại đều gọi trực tiếp từ đó.

| Thứ dùng lại | Đường dẫn trong `boom-re` | Cách dùng | Ghi chú |
|---|---|---|---|
| đọc pack `.idx/.idd` | `tools/idf.py` | `import idf` (thêm `sys.path`) | dùng `decrypt_idx`, `parse_idx`; **chưa sửa** |
| doctor chặn file packed | `tools/vmdevirt.py` | `subprocess doctor` | profile-driven, không hardcode VN |
| phương pháp devirt | `docs/devirt-themida-huong-dan.md` | đọc | không phụ thuộc bản nào |
| bảng pack/khoá của VN | `tools/idf.py` (`KEYS`) | tham chiếu | 2 khoá VN dùng lại được cho 4 pack KR |
| kế hoạch & phân vai | `docs/MASTER_PLAN.md`, `docs/kr-re-plan.md` | đọc | nguồn chuẩn |

## Mốc đồng bộ

- `boom-re` @ `2b650e7` (2026-10-18) — lúc tạo project này.
- Nếu `boom-re` đổi `tools/idf.py` (ví dụ thêm stem KR vào `KEYS`), project này **hưởng ngay** vì
  không copy. Ngược lại, mọi thay đổi riêng cho KR phải nằm ở project này (`tools/krctl.py`), trừ khi
  chủ dự án quyết định đưa vào `boom-re`.

## Quy ước file

| Thư mục | Nội dung | Git |
|---|---|---|
| `tools/` | `krctl.py` (chỉ phần KR) | ✅ |
| `docs/` | phát hiện, có tag bằng chứng | ✅ |
| `out/` | JSON/listing sinh ra | ❌ git-ignored |
| build KR | `/home/admin/boom-re/boomKR/` | ❌ ngoài repo |
