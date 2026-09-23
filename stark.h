// Reading the game's own archives, so the remake has no build step between it
// and the shipped data.
//
//   .DKX  index: "DKBT", 64 byte header, then 2048 byte nodes.  Each node has
//                a 16 byte header and then records walked by their own stride.
//   .DKD  payload, addressed by (offset, size) from the index.
//
// A record is 32 bytes plus its name:
//
//   +4   offset into the .DKD        +13  compression method
//   +8   type                        +14  size after decompression
//   +9   size stored                 +18  w, h        +30  name length
//
// Method 1 is FUN_007519d0 in ST.exe: LZSS over a 16 bit little endian bit
// stream, read LSB first.  Method 2 (FUN_007516f0) uses a 64K window and is
// not implemented - nothing the menu touches uses it.
#pragma once
#include <algorithm>
#include <array>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace ark {

struct Record {
    uint32_t offset = 0, size = 0, unpacked = 0;
    uint8_t  type = 0, method = 0;
    uint16_t w = 0, h = 0;
    std::array<uint8_t, 8> extra{}; // index +22..29, type-specific metadata
};

inline uint16_t rd16(const uint8_t *p) { return uint16_t(p[0] | (p[1] << 8)); }
inline uint32_t rd32(const uint8_t *p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

// LZSS, method 1.  Tokens:
//   1        one literal byte
//   0 1      u16: bits 15..11 displacement high, 10..8 length-2, 7..0 low.
//            The 13 bit displacement is sign extended, so always backwards.
//            Length 2 escapes: next byte 0 ends the stream, 1 is a no-op,
//            anything else is length = byte + 1.
//   0 0      two bits give length 2..5, one byte gives displacement -256..-1
inline std::vector<uint8_t> unpack1(const uint8_t *src, size_t n, uint32_t want)
{
    std::vector<uint8_t> out;
    out.reserve(want);
    size_t p = 0;
    uint32_t flags = rd16(src);
    int left = 16;
    p = 2;
    auto bit = [&]() -> uint32_t {
        uint32_t b = flags & 1;
        flags >>= 1;
        if (--left == 0) {                 // refill only once the last bit is spent
            left = 16;
            flags = rd16(src + p);
            p += 2;
        }
        return b;
    };
    for (;;) {
        while (bit()) {
            if (p >= n) return out;
            out.push_back(src[p++]);
        }
        int len;
        int disp;
        size_t next;
        if (bit()) {
            uint32_t tok = rd16(src + p);
            len  = int((tok >> 8) & 7) + 2;
            disp = int(((tok & 0xF800) >> 3) | (tok & 0xFF)) - 0x2000;
            next = p + 2;
            if (len == 2) {
                uint8_t ext = src[p + 2];
                p += 3;
                if (ext == 0) return out;
                if (ext == 1) continue;
                len = ext + 1;
                next = p;
            }
        } else {
            len  = int(bit()) * 2 + 2 + int(bit());
            disp = int(src[p]) - 0x100;
            next = p + 1;
        }
        size_t base = out.size();
        if (disp >= 0 || size_t(-disp) > base) return out;   // never read before the start
        for (int i = 0; i < len; ++i)
            out.push_back(out[base + i + disp]);                   // byte at a time: runs overlap
        p = next;
    }
}

// A packed DIB - BITMAPINFOHEADER, palette, then 8 bit indices - as ARGB.
// Backgrounds want index 0 opaque; cut-outs like the map thumbnails want it
// clear, so which it is stays the caller's decision.
// Sama paleta spakowanego DIB-a. Pasek typu 6 z tego samego archiwum
// rozpakowuje sie WLASNIE nia - `STATS_<n>` idzie w palecie `REPORT_*`.
inline bool dibPalette(const uint8_t *p, size_t n, uint32_t pal[256])
{
    if (n < 40 || rd32(p) != 40) return false;
    uint32_t used = rd32(p + 32);
    if (used == 0) used = 256;
    if (n < 40 + size_t(used) * 4) return false;
    for (uint32_t i = 0; i < 256; ++i) {
        if (i >= used) { pal[i] = 0xFF000000u; continue; }
        const uint8_t *c = p + 40 + size_t(i) * 4;       // BGRA
        pal[i] = 0xFF000000u | (uint32_t(c[2]) << 16)
               | (uint32_t(c[1]) << 8) | c[0];
    }
    return true;
}

inline bool decodeDib(const uint8_t *p, size_t n, int &w, int &h,
                      std::vector<uint32_t> &out, bool zeroClear)
{
    if (n < 40 || rd32(p) != 40) return false;
    int iw = int(rd32(p + 4)), ih = int(rd32(p + 8));
    if (rd16(p + 14) != 8 || iw <= 0 || ih == 0) return false;
    bool bottomUp = ih > 0;
    if (ih < 0) ih = -ih;
    uint32_t used = rd32(p + 32);
    if (used == 0) used = 256;
    size_t stride = (size_t(iw) * 8 + 31) / 32 * 4;
    if (n < 40 + size_t(used) * 4 + stride * size_t(ih)) return false;
    const uint8_t *pal = p + 40, *px = pal + size_t(used) * 4;
    w = iw;
    h = ih;
    out.assign(size_t(iw) * ih, 0);
    for (int y = 0; y < ih; ++y) {
        const uint8_t *row = px + stride * size_t(bottomUp ? (ih - 1 - y) : y);
        for (int x = 0; x < iw; ++x) {
            uint8_t i = row[x];
            if (!i && zeroClear) continue;
            const uint8_t *c = pal + size_t(i) * 4;          // palette is BGRA
            out[size_t(y) * iw + x] = 0xFF000000u | (uint32_t(c[2]) << 16)
                                    | (uint32_t(c[1]) << 8) | c[0];
        }
    }
    return true;
}

class Archive {
public:
    bool open(const std::string &dkxPath, const std::string &dkdPath, bool mapIndex = false)
    {
        if (!slurp(dkxPath, idx) || !slurp(dkdPath, dkd)) return false;
        // **Drugie otwarcie musi zaczac od pustego spisu.** `parse()` tylko
        // DOKLADA rekordy, a przy duplikacie nazwy zostaje wiekszy - wiec bez
        // tego wpis ze starego archiwum wygrywal z nowym i jego `offset`
        // wskazywal w **nowy** bufor danych. Wychodzily z tego smieci albo
        // odczyt poza zakresem, i to tym czesciej, im bardziej archiwa sie
        // roznily. Widac to dopiero przy dwoch mapach o roznym terenie
        // w jednym procesie - ta sama rodzina, co `aiSt` i `civ`.
        recs.clear();
        if (mapIndex && dkd.size() >= 4 && std::memcmp(dkd.data(), "DKFM", 4) == 0) {
            if (!parseTree()) { recs.clear(); return false; }
        } else parse(); // recovery for old remake exports without DKFM
        return !recs.empty();
    }

    // Nazwy wszystkich rekordow. Edytor potrzebuje ich, zeby przepisac
    // archiwum w calosci - z podmieniona jedna pozycja i **niczym wiecej
    // zmienionym**.
    std::vector<std::string> names() const
    {
        std::vector<std::string> out;
        out.reserve(recs.size());
        for (const auto &kv : recs) out.push_back(kv.first);
        std::sort(out.begin(), out.end());
        return out;
    }

    const Record *find(const std::string &name) const
    {
        auto it = recs.find(name);
        return it == recs.end() ? nullptr : &it->second;
    }

    // Decompressed payload, or empty when the record is missing.
    std::vector<uint8_t> read(const std::string &name) const
    {
        const Record *r = find(name);
        if (!r) return {};
        const uint8_t *p = dkd.data() + r->offset;
        if (r->method == 0) return std::vector<uint8_t>(p, p + r->size);
        if (r->method != 1) return {};
        return unpack1(p, r->size, r->unpacked);
    }

    size_t count() const { return recs.size(); }

    // Some things are addressed by a raw .DKD offset rather than by record:
    // SYS_FONT holds six such pointers, one per colour, each a plain DIB.
    const uint8_t *dkdAt(uint32_t off, uint32_t need) const
    {
        if (size_t(off) + need > dkd.size()) return nullptr;
        return dkd.data() + off;
    }

private:
    std::vector<uint8_t> idx, dkd;
    std::unordered_map<std::string, Record> recs;

public:
    static bool slurp(const std::string &path, std::vector<uint8_t> &into)
    {
        FILE *f = std::fopen(path.c_str(), "rb");
        if (!f) return false;
        std::fseek(f, 0, SEEK_END);
        long n = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        into.resize(size_t(n < 0 ? 0 : n));
        size_t got = into.empty() ? 0 : std::fread(into.data(), 1, into.size(), f);
        std::fclose(f);
        into.resize(got);
        return got > 0;
    }

    // One record, or false when the bytes do not check out as one.
    bool readRec(size_t i, size_t limit, size_t &next, bool tree = false)
    {
        if (i + 32 > limit) return false;
        uint16_t nameLen = rd16(&idx[i + 30]);
        if (nameLen == 0 || nameLen > (tree ? 512 : 32) || i + 32 + nameLen > limit) return false;
        for (size_t k = 0; k < nameLen; ++k) {
            uint8_t c = idx[i + 32 + k];
            if (c < (tree ? 32 : 33) || c >= 127) return false;
        }
        Record r;
        r.offset   = rd32(&idx[i + 4]);
        r.type     = idx[i + 8];
        r.size     = rd32(&idx[i + 9]);
        r.method   = idx[i + 13];
        r.unpacked = rd32(&idx[i + 14]);
        r.w        = rd16(&idx[i + 18]);
        r.h        = rd16(&idx[i + 20]);
        std::copy_n(&idx[i + 22], 8, r.extra.begin());
        if (r.size == 0 || size_t(r.offset) + r.size > dkd.size()) return false;
        // A name can appear more than once with different types: BOAT holds
        // "crui" both as a 92 byte descriptor (type 29) and as the 69 KB
        // sprite strip (type 6). Keep the larger one - the art, not the label.
        std::string name(reinterpret_cast<const char *>(&idx[i + 32]), nameLen);
        auto ins = recs.emplace(std::move(name), r);
        if (!ins.second && r.unpacked > ins.first->second.unpacked)
            ins.first->second = r;
        next = i + 32 + nameLen;
        return true;
    }

    bool parseTree()
    {
        if (idx.size() < 64 || std::memcmp(idx.data(), "DKBT", 4) != 0) return false;
        std::vector<uint8_t> seen((idx.size()-64)/2048, 0);
        std::function<bool(uint32_t)> visit = [&](uint32_t at) {
            if (at == 0xffffffffu) return true;
            if (at < 64 || (at-64)%2048 || size_t(at)+2048 > idx.size()) return false;
            size_t n = (at-64)/2048;
            if (seen[n]) return false;
            seen[n] = 1;
            size_t used = rd16(&idx[at+8]);
            if (used > 2032 || !visit(rd32(&idx[at+4]))) return false;
            size_t p = at+16, end = p+used, next;
            while (p < end) {
                if (!readRec(p,end,next,true) || !visit(rd32(&idx[p]))) return false;
                p = next;
            }
            return p == end;
        };
        return visit(rd32(&idx[36]));
    }

    void parse()
    {
        const size_t HDR = 64, NODE = 2048;
        if (idx.size() > HDR && std::memcmp(idx.data(), "DKBT", 4) == 0
                && (idx.size() - HDR) % NODE == 0) {
            for (size_t base = HDR; base < idx.size(); base += NODE) {
                size_t i = base + 16, limit = base + NODE, next;
                while (readRec(i, limit, next)) i = next;
            }
            return;
        }
        size_t i = 16, next;                     // archive without the node layout
        while (i + 34 <= idx.size()) {
            if (readRec(i, idx.size(), next)) i = next;
            else ++i;
        }
    }
};

// ------------------------------------------------------- zapis archiwum
//
// Original archive format confirmed in ST.exe, not inferred from a linear scan:
// DKX: 64-byte DKBT header; root offset +36, height u16 +42; 2048-byte nodes.
// Node +4 is the left child, +8 is a u16 used-byte count, keys start at +16.
// Record +0 is its RIGHT child, +4 payload offset, +8 type, +9 stored size,
// +13 compression, +14 unpacked size, +18..29 type-specific metadata,
// +30 name length, +32 name. Compare (type, name length, name bytes).
// DKD: 32-byte DKFM allocator header, size-prefixed aligned payload blocks,
// then a terminal free block. Raw concatenated payloads are NOT compatible.
// See ST_map_format.md and tools/map_original_search.py for executable checks.
struct OutRec {
    std::string name;
    uint8_t     type = 0;
    uint16_t    w = 0, h = 0;
    std::vector<uint8_t> data;
    std::array<uint8_t, 8> extra{};
};

inline void wr16(uint8_t *p, uint16_t v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); }
inline void wr32(uint8_t *p, uint32_t v)
{
    p[0] = uint8_t(v); p[1] = uint8_t(v >> 8);
    p[2] = uint8_t(v >> 16); p[3] = uint8_t(v >> 24);
}

// Ile bajtow indeksu zajma te rekordy. Wolajacy moze zapytac ZANIM cokolwiek
// zapisze - lepiej odmowic przed otwarciem pliku niz zostawic polowe.
inline size_t indexBytes(const std::vector<OutRec> &recs)
{
    size_t n = 0;
    for (const OutRec &r : recs) n += 32 + r.name.size();
    return n;
}

constexpr size_t NODE_CAPACITY = 2048 - 16;

// Zwraca pusty napis przy powodzeniu, inaczej powod odmowy.
//
// **Nie nadpisuje.** Istniejacy plik o tej nazwie jest bledem, a nie
// zaproszeniem - edytor ma tworzyc mapy, nie kasowac cudze.
inline std::string writeArchive(const std::string &dkxPath,
                                const std::string &dkdPath,
                                const std::vector<OutRec> &recs)
{
    if (recs.empty()) return "brak rekordow";
    for (const OutRec &r : recs)
        if (32 + r.name.size() > NODE_CAPACITY)
            return "nazwa rekordu za dluga: " + r.name;
    for (const std::string &p : { dkxPath, dkdPath }) {
        FILE *t = std::fopen(p.c_str(), "rb");
        if (t) { std::fclose(t); return "plik juz istnieje: " + p; }
    }

    // cMf32 comparator (006f0e30): section, name length, then raw name.
    std::vector<size_t> order;
    for (size_t i = 0; i < recs.size(); ++i) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        const auto &x = recs[a]; const auto &y = recs[b];
        if (x.type != y.type) return x.type < y.type;
        if (x.name.size() != y.name.size()) return x.name.size() < y.name.size();
        return x.name < y.name;
    });
    for (size_t k = 0; k < order.size(); ++k) {
        const auto &r = recs[order[k]];
        if (r.name.empty() || r.name.size() > 512 || r.data.empty())
            return "pusty lub nieprawidlowy rekord: " + r.name;
        if (k && r.type == recs[order[k-1]].type && r.name == recs[order[k-1]].name)
            return "powtorzony klucz: " + r.name;
    }
    // DKFM is an allocator, not a naked concatenation of payloads. Each live
    // block starts with aligned size|1; offsets address the following payload.
    // 00753170 checks the magic, version and final free-block marker.
    std::vector<uint8_t> dkd(32, 0);
    std::vector<uint32_t> offsets(recs.size());
    for (size_t i : order) {
        size_t start = dkd.size(), bytes = (recs[i].data.size() + 7) & ~size_t(3);
        if (start + bytes + 8 >= 0x40000000u) return "archiwum za duze";
        dkd.resize(start + bytes, 0);
        wr32(&dkd[start], uint32_t(bytes) | 1u);
        offsets[i] = uint32_t(start + 4);
        std::copy(recs[i].data.begin(), recs[i].data.end(), dkd.begin() + start + 4);
    }
    uint32_t freeAt = uint32_t(dkd.size());
    dkd.resize(dkd.size() + 8, 0);
    std::memcpy(dkd.data(), "DKFM", 4);
    wr16(&dkd[4], 0x101);
    wr32(&dkd[8], uint32_t(dkd.size()));
    wr32(&dkd[12], freeAt);
    wr32(&dkd[freeAt], 0x40000000u - freeAt);

    // A balanced B-tree. Insert sorted keys on the rightmost branch, splitting
    // full nodes and promoting the median. Node+4 is its first child; each
    // record+0 is the child to the RIGHT of that key (00755970).
    struct Node { std::vector<size_t> keys; std::vector<int> children{-1}; };
    std::vector<Node> nodes(1);
    int root = 0, height = 1;
    std::function<int(int, size_t, size_t &)> insert = [&](int n, size_t key, size_t &promoted) -> int {
        int child = nodes[n].children.back();
        if (child < 0) {
            nodes[n].keys.push_back(key); nodes[n].children.push_back(-1);
        } else {
            size_t up = 0;
            int right = insert(child, key, up);
            if (right < 0) return -1;
            nodes[n].keys.push_back(up); nodes[n].children.push_back(right);
        }
        size_t bytes = 0;
        for (size_t k : nodes[n].keys) bytes += 32 + recs[k].name.size();
        if (bytes <= NODE_CAPACITY) return -1;
        size_t mid = nodes[n].keys.size() / 2;
        promoted = nodes[n].keys[mid];
        Node right;
        right.keys.assign(nodes[n].keys.begin() + mid + 1, nodes[n].keys.end());
        right.children.assign(nodes[n].children.begin() + mid + 1, nodes[n].children.end());
        nodes[n].keys.resize(mid); nodes[n].children.resize(mid + 1);
        int ri = int(nodes.size()); nodes.push_back(std::move(right));
        return ri;
    };
    for (size_t k : order) {
        size_t up = 0; int right = insert(root, k, up);
        if (right >= 0) {
            Node n; n.keys = {up}; n.children = {root, right};
            root = int(nodes.size()); nodes.push_back(std::move(n)); ++height;
        }
    }
    auto address = [](int n) -> uint32_t { return n < 0 ? 0xffffffffu : 64u + 2048u * uint32_t(n); };
    std::vector<uint8_t> idx(64 + 2048 * nodes.size(), 0);
    std::memcpy(idx.data(), "DKBT", 4);
    wr32(&idx[4], 0x08000101u); wr32(&idx[8], uint32_t(idx.size()));
    wr32(&idx[12], 0xffffffffu); wr32(&idx[16], 0x00200001u);
    wr32(&idx[32], 0xffffffffu); wr32(&idx[36], address(root));
    wr32(&idx[40], uint32_t(height) << 16);
    idx[48] = 0xff; std::memcpy(&idx[54], "00000000", 8);
    for (size_t n = 0; n < nodes.size(); ++n) {
        uint8_t *node = &idx[address(int(n))];
        wr32(node, int(n) == root ? 0xffffffffu : address(int(n)));
        wr32(node + 4, address(nodes[n].children[0]));
        size_t at = 16;
        for (size_t k = 0; k < nodes[n].keys.size(); ++k) {
            size_t i = nodes[n].keys[k]; const OutRec &r = recs[i];
            uint8_t *p = node + at;
            wr32(p, address(nodes[n].children[k+1])); wr32(p + 4, offsets[i]);
            p[8] = r.type; wr32(p + 9, uint32_t(r.data.size()));
            p[13] = 0; wr32(p + 14, uint32_t(r.data.size()));
            wr16(p + 18, r.w); wr16(p + 20, r.h);
            std::copy(r.extra.begin(), r.extra.end(), p + 22);
            wr16(p + 30, uint16_t(r.name.size()));
            std::memcpy(p + 32, r.name.data(), r.name.size());
            at += 32 + r.name.size();
        }
        wr16(node + 8, uint16_t(at - 16));
    }

    FILE *fd = std::fopen(dkdPath.c_str(), "wb");
    if (!fd) return "nie moge zapisac " + dkdPath;
    if (!dkd.empty()) std::fwrite(dkd.data(), 1, dkd.size(), fd);
    std::fclose(fd);
    FILE *fx = std::fopen(dkxPath.c_str(), "wb");
    if (!fx) return "nie moge zapisac " + dkxPath;
    std::fwrite(idx.data(), 1, idx.size(), fx);
    std::fclose(fx);
    return std::string();
}

// Kopia archiwum z podmieniona zawartoscia JEDNEGO rekordu, o tym samym
// rozmiarze. Indeks nie musi sie wtedy zmienic ani o bajt, wiec zapis jest
// bezpieczny z definicji: wszystko, czego nie rozumiemy, przechodzi
// nietkniete.
//
// Dziala, bo `3D_MAP` jest **nieskompresowany na wszystkich 58 mapach**
// (metoda 0, rozmiar zapisany rowny rozpakowanemu) - wiec teren da sie
// przepisac w miejscu.
inline std::string patchCopy(const std::string &srcDkx, const std::string &srcDkd,
                             const std::string &dstDkx, const std::string &dstDkd,
                             const std::string &recName,
                             const std::vector<uint8_t> &data)
{
    for (const std::string &p : { dstDkx, dstDkd }) {
        FILE *t = std::fopen(p.c_str(), "rb");
        if (t) { std::fclose(t); return "plik juz istnieje: " + p; }
    }
    Archive a;
    if (!a.open(srcDkx, srcDkd)) return "nie moge otworzyc " + srcDkx;
    const Record *r = a.find(recName);
    if (!r) return "brak rekordu " + recName;
    if (r->method != 0) return recName + " jest skompresowany";
    if (r->size != data.size()) return recName + ": inny rozmiar";

    std::vector<uint8_t> dkd, dkx;
    if (!Archive::slurp(srcDkd, dkd) || !Archive::slurp(srcDkx, dkx))
        return "nie moge wczytac zrodla";
    if (r->offset + data.size() > dkd.size()) return "rekord poza plikiem";
    std::memcpy(&dkd[r->offset], data.data(), data.size());

    FILE *fd = std::fopen(dstDkd.c_str(), "wb");
    if (!fd) return "nie moge zapisac " + dstDkd;
    std::fwrite(dkd.data(), 1, dkd.size(), fd);
    std::fclose(fd);
    FILE *fx = std::fopen(dstDkx.c_str(), "wb");
    if (!fx) return "nie moge zapisac " + dstDkx;
    std::fwrite(dkx.data(), 1, dkx.size(), fx);
    std::fclose(fx);
    return std::string();
}

}  // namespace ark
