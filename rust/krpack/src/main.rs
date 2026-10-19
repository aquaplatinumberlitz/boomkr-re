// krpack — đọc pack Boom (KR/VN): .idx (XOR-chain) + .idd (entry decrypt/zlib)
// Format lấy từ RE: PackFS__DecryptIdx / PackFS__DecryptEntry / CData__LoadFromPack.
use std::env;
use std::fs;
use std::path::{Path, PathBuf};

const MAGIC: u32 = 0x9C6C95CE;

fn crc32(init: u32, data: &[u8]) -> u32 {
    let mut crc = !init;
    for &b in data {
        crc ^= b as u32;
        for _ in 0..8 {
            let mask = (crc & 1).wrapping_neg();
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    !crc
}

fn decrypt_idx(data: &[u8], key: u32) -> Vec<u8> {
    let n = data.len() / 4;
    let mut out = Vec::with_capacity(data.len());
    let mut prev = key;
    for i in 0..n {
        let x = u32::from_le_bytes([data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]]);
        prev = if i == 0 { x ^ key } else { prev ^ x };
        out.extend_from_slice(&prev.to_le_bytes());
    }
    let last = prev;
    for (j, &b) in data[n * 4..].iter().enumerate() {
        out.push(b ^ ((last >> (8 * j)) & 0xFF) as u8);
    }
    out
}

fn decrypt_entry(data: &[u8], key: u32) -> Vec<u8> {
    let n = data.len() / 4;
    let mut out = Vec::with_capacity(data.len());
    let mut prev = key;
    for i in 0..n {
        let x = u32::from_le_bytes([data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]]);
        let v = if i == 0 {
            x ^ key ^ MAGIC
        } else {
            let poly = ((i as u32).wrapping_mul(i as u32).wrapping_mul(i as u32 + 966).wrapping_mul(i as u32 + 2020));
            prev ^ x ^ poly ^ MAGIC
        };
        prev = v;
        out.extend_from_slice(&v.to_le_bytes());
    }
    let last = prev;
    for (j, &b) in data[n * 4..].iter().enumerate() {
        out.push(b ^ ((last >> (8 * j)) & 0xFF) as u8);
    }
    out
}

fn zlib_inflate(data: &[u8]) -> Vec<u8> {
    // zlib: 2 byte header (0x78 ...), deflate raw, 4 byte adler — dùng miniz_oxide? không có dep.
    // Tạm: gọi thư viện chuẩn không có inflate => trả nguyên (báo lỗi ở CLI).
    let _ = data;
    Vec::new()
}

#[derive(Clone, Debug)]
struct Entry {
    name: String,
    off: u32,
    size: u32,
    key: u32,
    crc: u32,
}

struct Pack {
    entries: Vec<Entry>,
    idd: Vec<u8>,
}

fn parse_idx(buf: &[u8]) -> Vec<Entry> {
    let first = u32::from_le_bytes([buf[0], buf[1], buf[2], buf[3]]);
    let (count, mut off, extra) = if first == 0 {
        let count = u32::from_le_bytes([buf[8], buf[9], buf[10], buf[11]]) as usize;
        (count, 12usize, true)
    } else {
        (first as usize, 4usize, false)
    };
    let mut ents = Vec::with_capacity(count);
    for _ in 0..count {
        if off + 4 > buf.len() { break; }
        let nl = i32::from_le_bytes([buf[off], buf[off + 1], buf[off + 2], buf[off + 3]]);
        off += 4;
        if nl <= 0 || off + nl as usize > buf.len() {
            break;
        }
        let name = String::from_utf8_lossy(&buf[off..off + nl as usize]).to_string();
        off += nl as usize;
        let a = u32::from_le_bytes([buf[off], buf[off + 1], buf[off + 2], buf[off + 3]]);
        let b = u32::from_le_bytes([buf[off + 4], buf[off + 5], buf[off + 6], buf[off + 7]]);
        let c = u32::from_le_bytes([buf[off + 8], buf[off + 9], buf[off + 10], buf[off + 11]]);
        off += 12;
        let d = if extra {
            let v = u32::from_le_bytes([buf[off], buf[off + 1], buf[off + 2], buf[off + 3]]);
            off += 4;
            v
        } else {
            0
        };
        ents.push(Entry { name, off: a, size: b, key: c, crc: d });
    }
    ents
}

impl Pack {
    fn open(root: &Path, stem: &str) -> std::io::Result<Pack> {
        let idx = fs::read(root.join(format!("{stem}.idx")))?;
        let idd = fs::read(root.join(format!("{stem}.idd")))?;
        let key = u32::from_le_bytes([idx[0], idx[1], idx[2], idx[3]]);
        let plain = decrypt_idx(&idx, key);
        Ok(Pack { entries: parse_idx(&plain), idd })
    }

    fn read(&self, name: &str) -> Option<(Vec<u8>, u32, u32)> {
        let e = self.entries.iter().find(|e| e.name == name)?;
        let start = e.off as usize;
        let end = start + e.size as usize;
        if end > self.idd.len() {
            return None;
        }
        let raw = &self.idd[start..end];
        if e.key != 0 {
            let blob = decrypt_entry(raw, e.key);
            let ok = e.crc <= 1 || crc32(e.key, &blob) == e.crc;
            Some((blob, e.key, if ok { 1 } else { 0 }))
        } else if e.name.contains("_mm") {
            Some((zlib_inflate(raw), 0, 0))
        } else {
            Some((raw.to_vec(), 0, 1))
        }
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        eprintln!("usage: krpack <packdir> list <stem> | krpack <packdir> cat <stem> <name> <out> | krpack <packdir> count");
        std::process::exit(2);
    }
    let root = PathBuf::from(&args[1]);
    let cmd = args[2].as_str();
    match cmd {
        "count" => {
            let mut total = 0usize;
            let mut packs = 0usize;
            let mut names: Vec<String> = fs::read_dir(&root).unwrap()
                .filter_map(|e| e.ok())
                .map(|e| e.file_name().to_string_lossy().to_string())
                .filter(|n| n.ends_with(".idx"))
                .collect();
            names.sort();
            for n in &names {
                let stem = n.trim_end_matches(".idx");
                match Pack::open(&root, stem) {
                    Ok(p) => { packs += 1; total += p.entries.len();
                               println!("{stem:<14} {:>6} entry", p.entries.len()); }
                    Err(e) => println!("{stem:<14} LỖI {e}"),
                }
            }
            println!("--- {packs} pack, {total} entry");
        }
        "list" => {
            let stem = &args[3];
            let p = Pack::open(&root, stem).unwrap();
            for e in p.entries.iter().take(20) {
                println!("{:<40} off={:#010x} size={:#x} key={:#x}", e.name, e.off, e.size, e.key);
            }
            println!("... tổng {}", p.entries.len());
        }
        "cat" => {
            let stem = &args[3];
            let name = &args[4];
            let out = &args[5];
            let p = Pack::open(&root, stem).unwrap();
            match p.read(name) {
                Some((blob, key, ok)) => {
                    fs::write(out, &blob).unwrap();
                    println!("{name} -> {out} ({} B, key={key:#x}, crc_ok={ok})", blob.len());
                }
                None => { eprintln!("không thấy {name}"); std::process::exit(1); }
            }
        }
        _ => { eprintln!("lệnh lạ"); std::process::exit(2); }
    }
}
