// Autodesk FLC playback for the menu's background animations.
//
// Once ark::Archive has expanded them the MM_FLC records are ordinary .FLC
// files: a 128 byte header, then one chunk per frame, each holding sub-chunks
// that either replace the palette or patch the previous frame. Only the
// sub-chunk types the game's own files use are handled; the rest are skipped
// rather than guessed at.
//
// Frames are decoded on demand into a single buffer, so an animation costs its
// own frame size rather than every frame at once - MM_FLC03 is 267x141 across
// 560 frames, which is 37 KB held instead of 21 MB.
#pragma once
#include <cstdint>
#include <vector>

#include "stark.h"

namespace flc {

enum : uint16_t { COLOR_256 = 4, DELTA_FLC = 7, COLOR_64 = 11, DELTA_FLI = 12,
                  BLACK = 13, BYTE_RUN = 15, FLI_COPY = 16 };

class Anim {
public:
    bool load(std::vector<uint8_t> blob)
    {
        if (blob.size() < 128) return false;
        data = std::move(blob);
        uint16_t magic = ark::rd16(&data[4]);
        if (magic != 0xAF12 && magic != 0xAF11) return false;
        declared = ark::rd16(&data[6]);
        w = ark::rd16(&data[8]);
        h = ark::rd16(&data[10]);
        if (!w || !h) return false;
        px.assign(size_t(w) * h, 0);
        pal.assign(256 * 3, 0);
        // Index the frame chunks once so wrapping does not have to rescan.
        for (size_t p = 128; p + 6 <= data.size();) {
            uint32_t csize = ark::rd32(&data[p]);
            if (csize < 6) break;
            if (ark::rd16(&data[p + 4]) == 0xF1FA) chunks.push_back(p);
            p += csize + (csize & 1);
        }
        // The trailing ring frame just restores frame 0, and chunk 0 is a full
        // keyframe, so looping the declared count is seamless without it.
        if (declared && declared < chunks.size()) chunks.resize(declared);
        cur = -1;
        step();
        return !chunks.empty();
    }

    // Advance one frame, wrapping at the end.
    void step()
    {
        if (chunks.empty()) return;
        int nxt = cur + 1;
        if (nxt >= int(chunks.size())) nxt = 0;
        if (nxt == 0) px.assign(px.size(), 0);      // chunk 0 repaints in full
        apply(chunks[size_t(nxt)]);
        cur = nxt;
    }

    int width()  const { return w; }
    int height() const { return h; }
    int frame()  const { return cur; }
    int frames() const { return int(chunks.size()); }
    const uint8_t *pixels() const { return px.data(); }
    const uint8_t *palette() const { return pal.data(); }
    bool ok() const { return !chunks.empty(); }

private:
    std::vector<uint8_t> data, px, pal;
    std::vector<size_t> chunks;
    int w = 0, h = 0, cur = -1;
    uint16_t declared = 0;

    void colors(const uint8_t *b, size_t n, bool six)
    {
        if (n < 2) return;
        uint16_t packets = ark::rd16(b);
        size_t p = 2;
        int idx = 0;
        for (uint16_t k = 0; k < packets && p + 2 <= n; ++k) {
            idx += b[p];
            int cnt = b[p + 1] ? b[p + 1] : 256;
            p += 2;
            for (int i = 0; i < cnt && p + 3 <= n; ++i, p += 3) {
                if (idx + i >= 256) continue;
                uint8_t r = b[p], g = b[p + 1], bl = b[p + 2];
                if (six) {                       // 6 bit channels, scaled up
                    r = uint8_t((r << 2) | (r >> 4));
                    g = uint8_t((g << 2) | (g >> 4));
                    bl = uint8_t((bl << 2) | (bl >> 4));
                }
                pal[size_t(idx + i) * 3 + 0] = r;
                pal[size_t(idx + i) * 3 + 1] = g;
                pal[size_t(idx + i) * 3 + 2] = bl;
            }
            idx += cnt;
        }
    }

    void byteRun(const uint8_t *b, size_t n)
    {
        size_t p = 0;
        for (int y = 0; y < h; ++y) {
            if (p >= n) return;
            ++p;                                  // packet count, unreliable in FLC
            int x = 0;
            size_t row = size_t(y) * w;
            while (x < w && p < n) {
                int8_t c = int8_t(b[p++]);
                if (c < 0) {                      // literal run
                    int cnt = -c;
                    for (int i = 0; i < cnt && x < w && p < n; ++i) px[row + x++] = b[p++];
                } else {
                    if (p >= n) return;
                    uint8_t v = b[p++];
                    for (int i = 0; i < c && x < w; ++i) px[row + x++] = v;
                }
            }
        }
    }

    void deltaFlc(const uint8_t *b, size_t n)
    {
        if (n < 2) return;
        int lines = ark::rd16(b);
        size_t p = 2;
        int y = 0;
        while (lines > 0 && p + 2 <= n) {
            uint16_t op = ark::rd16(b + p);
            p += 2;
            uint16_t top = op & 0xC000;
            if (top == 0xC000) { y += int(0x10000 - op); continue; }   // line skip
            if (top == 0x8000) {                                       // odd width tail
                if (y >= 0 && y < h) px[size_t(y) * w + w - 1] = uint8_t(op & 0xFF);
                continue;
            }
            --lines;
            int x = 0;
            for (int k = 0; k < op && p + 2 <= n; ++k) {
                x += b[p];
                int8_t t = int8_t(b[p + 1]);
                p += 2;
                if (y < 0 || y >= h) break;
                size_t row = size_t(y) * w;
                if (t < 0) {                       // repeat one word
                    int cnt = -t;
                    if (p + 2 > n) break;
                    uint8_t a = b[p], c = b[p + 1];
                    p += 2;
                    for (int i = 0; i < cnt && x + 1 < w; ++i) {
                        px[row + x] = a;
                        px[row + x + 1] = c;
                        x += 2;
                    }
                } else {
                    for (int i = 0; i < t && x + 1 < w && p + 2 <= n; ++i) {
                        px[row + x] = b[p];
                        px[row + x + 1] = b[p + 1];
                        p += 2;
                        x += 2;
                    }
                }
            }
            ++y;
        }
    }

    void deltaFli(const uint8_t *b, size_t n)
    {
        if (n < 4) return;
        int y0 = ark::rd16(b), lines = ark::rd16(b + 2);
        size_t p = 4;
        for (int i = 0; i < lines && p < n; ++i) {
            int y = y0 + i;
            int packets = b[p++];
            int x = 0;
            for (int k = 0; k < packets && p + 2 <= n; ++k) {
                x += b[p];
                int8_t t = int8_t(b[p + 1]);
                p += 2;
                if (y < 0 || y >= h) break;
                size_t row = size_t(y) * w;
                if (t < 0) {                       // repeat one byte
                    int cnt = -t;
                    if (p >= n) break;
                    uint8_t v = b[p++];
                    for (int j = 0; j < cnt && x < w; ++j) px[row + x++] = v;
                } else {
                    for (int j = 0; j < t && x < w && p < n; ++j) px[row + x++] = b[p++];
                }
            }
        }
    }

    void apply(size_t p)
    {
        size_t n = data.size();
        if (p + 16 > n) return;
        uint16_t subs = ark::rd16(&data[p + 6]);
        size_t q = p + 16;
        for (uint16_t i = 0; i < subs && q + 6 <= n; ++i) {
            uint32_t ssize = ark::rd32(&data[q]);
            if (ssize < 6) break;
            uint16_t stype = ark::rd16(&data[q + 4]);
            const uint8_t *body = &data[q + 6];
            size_t blen = (q + ssize <= n ? ssize : n - q) - 6;
            switch (stype) {
            case COLOR_256: colors(body, blen, false); break;
            case COLOR_64:  colors(body, blen, true);  break;
            case BYTE_RUN:  byteRun(body, blen);       break;
            case DELTA_FLC: deltaFlc(body, blen);      break;
            case DELTA_FLI: deltaFli(body, blen);      break;
            case BLACK:     px.assign(px.size(), 0);   break;
            case FLI_COPY:
                for (size_t k = 0; k < px.size() && k < blen; ++k) px[k] = body[k];
                break;
            default: break;
            }
            q += ssize + (ssize & 1);
        }
    }
};

}  // namespace flc
