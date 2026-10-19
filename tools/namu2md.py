#!/usr/bin/env python3
"""Chuyển 1 trang namu.moe (mirror của namu.wiki) thành Markdown sạch.

Dùng: python3 tools/namu2md.py <url> <out.md> [--title T]
Chỉ trích phần <article>, bỏ khung điều hướng/TOC/footer; giữ nguyên markup wiki
ở dạng đọc được (bảng ||, tiêu đề ==, chữ '''đậm''').
"""
import sys, re, html, urllib.request, urllib.parse, datetime

def fetch(url):
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0 (research)"})
    with urllib.request.urlopen(req, timeout=60) as r:
        return r.read().decode("utf-8", "replace")

def article(h):
    m = re.search(r"<article[^>]*>(.*?)</article>", h, re.S)
    body = m.group(1) if m else h
    body = re.sub(r"<script.*?</script>", "", body, flags=re.S)
    body = re.sub(r"<style.*?</style>", "", body, flags=re.S)
    body = re.sub(r"<br\s*/?>", "\n", body)
    body = re.sub(r"</(p|div|li|tr|h1|h2|h3|pre)>", "\n", body)
    body = re.sub(r"<[^>]+>", "", body)
    return html.unescape(body)

def clean(t):
    t = t.replace("\u00a0", " ")
    # wiki link [[a|b]] -> b ; [[a]] -> a
    t = re.sub(r"\[\[[^\]|]*\|([^\]]*)\]\]", r"\1", t)
    t = re.sub(r"\[\[([^\]]*)\]\]", r"\1", t)
    # '''bold''' -> **bold**
    t = re.sub(r"'''([^']+)'''", r"**\1**", t)
    # macro lines
    lines = []
    for ln in t.split("\n"):
        s = ln.rstrip()
        if s.lstrip().startswith(("#!", "{{{", "}}}", "||<")):
            continue
        if s.lstrip().startswith("[[파일:"):
            continue
        lines.append(s)
    t = "\n".join(lines)
    t = re.sub(r"[ \t]+\n", "\n", t)
    t = re.sub(r"\n{3,}", "\n\n", t)
    return t.strip()

def main():
    url = sys.argv[1]; out = sys.argv[2]
    title = None
    if "--title" in sys.argv:
        title = sys.argv[sys.argv.index("--title") + 1]
    raw = fetch(url)
    title = title or (re.search(r'<h1 class="wikiTitle"[^>]*>\s*<a[^>]*>(.*?)</a>', raw, re.S) or [None, "?"])[1]
    title = re.sub(r"<[^>]+>", "", title).strip()
    body = clean(article(raw))
    # cắt đầu: bỏ khung "상위 문서"/TOC, lấy từ tiêu đề mục 1 thật
    import re as _re
    m = _re.search(r"(?m)^1\.\s*개요\s*$", body)
    if m:
        body = body[m.start():]
    # cắt đuôi: bỏ block boilerplate/footer
    for marker in ("에서 가져왔습니다", "uuid not found", "이 저작물은"):
        i = body.find(marker)
        if i > 2000:
            body = body[:i]
            break
    body = body.strip()
    now = datetime.date.today().isoformat()
    hdr = (f"# {title}\n\n"
           f"> Nguồn: {urllib.parse.unquote(url)}\n"
           f"> Ngày cào: {now}\n"
           f"> Bản quyền: CC BY-NC-SA 2.0 KR (namu.wiki) — chỉ dùng tham khảo nội bộ, KHÔNG phát hành lại.\n"
           f"> Ghi chú: bản cào tự động, đã bỏ khung điều hướng/TOC/footer; markup wiki giữ nguyên dạng đọc được.\n\n---\n\n")
    open(out, "w", encoding="utf-8").write(hdr + body + "\n")
    print(f"{out}: {len(body)} ký tự")

if __name__ == "__main__":
    main()
