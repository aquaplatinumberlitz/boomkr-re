# Nguồn web — đã chuyển về docs chuẩn

Theo `AGENTS.md` (một nội dung — một nguồn chuẩn), phần **ghi chú web** đã chuyển sang worktree docs:

| Nội dung | Vị trí chuẩn |
|---|---|
| Finding trích lược (item/mode/monster/EXP/map/strategy) | `boom-re-l10n/docs/research/findings/WEB_RULES_NAMU.md` |
| Register nguồn (WEB-KR-002…008, ngày/truy cập/giới hạn) | `boom-re-l10n/docs/research/sources/README.md` |
| Bản cào đầy đủ 8 trang (local, gitignored) | `boom-re-l10n/docs/research/sources/raw-namu/` |
| Bảng EXP parse | `boomkr-re/data/kr/exp_table.json` |

Repo này chỉ giữ **công cụ cào** để tái lập: `tools/namu2md.py`.

```sh
python3 tools/namu2md.py "https://namu.moe/w/<tên trang>" <out.md>
```

Lưu ý license: nội dung namu là CC BY-NC-SA 2.0 KR → chỉ dùng nội bộ, không commit raw vào repo public.
